/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name : ethernetif.c
  * Description : This file provides code for the configuration
  * of the ethernetif.c MiddleWare with YT8512C PHY.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lwip/opt.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include "ethernetif.h"
/* USER CODE BEGIN Include for User BSP */
#include "yt8512c.h"
#include "stm32f4xx_hal_eth.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include <stddef.h> /* for offsetof */
/* USER CODE END Include for User BSP */
#include <string.h>
#include "cmsis_os.h"
#include "lwip/tcpip.h"

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN 0 */

/* YT8512C PHY 相关函数 */

/**
 * @brief YT8512C PHY 软复位
 * @param heth: ETH 句柄指针
 * @retval YT8512C 状态
 */
static int32_t YT8512C_SoftReset(ETH_HandleTypeDef *heth)
{
    uint32_t tickstart = 0;
    uint32_t regvalue = 0;

    /* 发送软复位命令 */
    if (HAL_ETH_WritePHYRegister(heth, YT8512C_ADDR, YT8512C_BCR, YT8512C_BCR_SOFT_RESET) != HAL_OK)
    {
        return YT8512C_STATUS_WRITE_ERROR;
    }

    /* 等待复位完成 */
    tickstart = HAL_GetTick();
    do {
        if (HAL_ETH_ReadPHYRegister(heth, YT8512C_ADDR, YT8512C_BCR, &regvalue) != HAL_OK)
        {
            return YT8512C_STATUS_READ_ERROR;
        }

        /* 检查超时 */
        if ((HAL_GetTick() - tickstart) >= 1000U)
        {
            return YT8512C_STATUS_RESET_TIMEOUT;
        }
    } while (regvalue & YT8512C_BCR_SOFT_RESET);

    return YT8512C_STATUS_OK;
}

/**
 * @brief YT8512C PHY 初始化
 * @param heth: ETH 句柄指针
 * @retval YT8512C 状态
 */
int32_t YT8512C_Init(ETH_HandleTypeDef *heth)
{
    int32_t ret = YT8512C_STATUS_OK;
    uint32_t regvalue = 0;

    /* 软复位 PHY */
    ret = YT8512C_SoftReset(heth);
    if (ret != YT8512C_STATUS_OK)
    {
        return ret;
    }

    /* 配置自动协商 */
    regvalue = YT8512C_BCR_AUTONEGO_EN | YT8512C_BCR_RESTART_AUTONEGO;
    if (HAL_ETH_WritePHYRegister(heth, YT8512C_ADDR, YT8512C_BCR, regvalue) != HAL_OK)
    {
        return YT8512C_STATUS_WRITE_ERROR;
    }

    /* 延时等待 PHY 稳定 */
    HAL_Delay(100);

    return YT8512C_STATUS_OK;
}

/**
 * @brief 获取 YT8512C PHY 链路状态 (增强错误处理)
 * @param heth: ETH 句柄指针
 * @retval YT8512C 状态
 */
int32_t YT8512C_GetLinkState(ETH_HandleTypeDef *heth)
{
    uint32_t regvalue = 0;
    uint32_t retry_count = 0;
    const uint32_t max_retries = 3;
    HAL_StatusTypeDef hal_status;

    /* 读取基本状态寄存器 - 带重试机制 */
    do {
        hal_status = HAL_ETH_ReadPHYRegister(heth, YT8512C_ADDR, YT8512C_BSR, &regvalue);
        if (hal_status == HAL_OK) {
            break;
        }

        retry_count++;
        if (retry_count < max_retries) {
            HAL_Delay(10); // 短暂延时后重试
        }
    } while (retry_count < max_retries);

    if (hal_status != HAL_OK)
    {
        return YT8512C_STATUS_READ_ERROR;
    }

    /* 检查链路状态 */
    if ((regvalue & YT8512C_BSR_LINK_STATUS) == 0)
    {
        return YT8512C_STATUS_LINK_DOWN;
    }

    /* 检查自动协商是否完成 */
    if ((regvalue & YT8512C_BSR_AUTONEGO_CPLT) == 0)
    {
        return YT8512C_STATUS_AUTONEGO_NOTDONE;
    }

    /* 读取速度和双工状态 - 带重试机制 */
    retry_count = 0;
    do {
        hal_status = HAL_ETH_ReadPHYRegister(heth, YT8512C_ADDR, YT8512C_PHYSCSR, &regvalue);
        if (hal_status == HAL_OK) {
            break;
        }

        retry_count++;
        if (retry_count < max_retries) {
            HAL_Delay(10); // 短暂延时后重试
        }
    } while (retry_count < max_retries);

    if (hal_status != HAL_OK)
    {
        return YT8512C_STATUS_READ_ERROR;
    }

    /* 解析速度和双工模式 - 使用更准确的位检测 */
    /* 检查速度 (bit 4 = 1 表示100M, bit 4 = 0 表示10M) */
    if ((regvalue & YT8512C_PHYSCSR_SPEED_100M) != 0)
    {
        /* 100Mbps - 检查双工模式 (bit 13 = 1 表示全双工) */
        if ((regvalue & YT8512C_PHYSCSR_DUPLEX_FULL) != 0)
        {
            return YT8512C_STATUS_100MBITS_FULLDUPLEX;
        }
        else
        {
            return YT8512C_STATUS_100MBITS_HALFDUPLEX;
        }
    }
    else
    {
        /* 10Mbps - 检查双工模式 */
        if ((regvalue & YT8512C_PHYSCSR_DUPLEX_FULL) != 0)
        {
            return YT8512C_STATUS_10MBITS_FULLDUPLEX;
        }
        else
        {
            return YT8512C_STATUS_10MBITS_HALFDUPLEX;
        }
    }
}

/**
 * @brief 设置 YT8512C PHY 链路状态 (当前未使用)
 * @param heth: ETH 句柄指针
 * @param LinkState: 链路状态
 * @retval YT8512C 状态
 */
__attribute__((unused))
static int32_t YT8512C_SetLinkState(ETH_HandleTypeDef *heth, uint32_t LinkState)
{
    uint32_t bcrvalue = 0;
    int32_t ret = YT8512C_STATUS_OK;

    switch (LinkState)
    {
        case YT8512C_STATUS_100MBITS_FULLDUPLEX:
            bcrvalue = YT8512C_BCR_SPEED_SELECT | YT8512C_BCR_DUPLEX_MODE;
            break;
        case YT8512C_STATUS_100MBITS_HALFDUPLEX:
            bcrvalue = YT8512C_BCR_SPEED_SELECT;
            break;
        case YT8512C_STATUS_10MBITS_FULLDUPLEX:
            bcrvalue = YT8512C_BCR_DUPLEX_MODE;
            break;
        case YT8512C_STATUS_10MBITS_HALFDUPLEX:
            bcrvalue = 0;
            break;
        default:
            ret = YT8512C_STATUS_ERROR;
            break;
    }

    if (ret == YT8512C_STATUS_OK)
    {
        /* 禁用自动协商并设置链路状态 */
        if (HAL_ETH_WritePHYRegister(heth, YT8512C_ADDR, YT8512C_BCR, bcrvalue) != HAL_OK)
        {
            ret = YT8512C_STATUS_WRITE_ERROR;
        }
    }

    return ret;
}

/* USER CODE END 0 */

/* Private define ------------------------------------------------------------*/
/* The time to block waiting for input. */
#define TIME_WAITING_FOR_INPUT ( portMAX_DELAY )
/* Time to block waiting for transmissions to finish */
#define ETHIF_TX_TIMEOUT (2000U)
/* USER CODE BEGIN OS_THREAD_STACK_SIZE_WITH_RTOS */
/* Stack size of the interface thread */
#define INTERFACE_THREAD_STACK_SIZE ( 350 )
/* USER CODE END OS_THREAD_STACK_SIZE_WITH_RTOS */
/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'

/* ETH Setting  */
#define ETH_DMA_TRANSMIT_TIMEOUT               ( 20U )
#define ETH_TX_BUFFER_MAX             ((ETH_TX_DESC_CNT) * 2U)

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/* Private variables ---------------------------------------------------------*/
/*
@Note: This interface is implemented to operate in zero-copy mode only:
        - Rx Buffers will be allocated from LwIP stack Rx memory pool,
          then passed to ETH HAL driver.
        - Tx Buffers will be allocated from LwIP stack memory heap,
          then passed to ETH HAL driver.

@Notes:
  1.a. ETH DMA Rx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_RX_DESC_CNT in ETH GUI (Rx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h
  1.b. ETH DMA Tx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_TX_DESC_CNT in ETH GUI (Tx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h

  2.a. Rx Buffers number must be between ETH_RX_DESC_CNT and 2*ETH_RX_DESC_CNT
  2.b. Rx Buffers must have the same size: ETH_RX_BUF_SIZE, this value must
       passed to ETH DMA in the init field (heth.Init.RxBuffLen)
  2.c  The RX Ruffers addresses and sizes must be properly defined to be aligned
       to L1-CACHE line size (32 bytes).
*/

/* Data Type Definitions */
typedef enum
{
  RX_ALLOC_OK       = 0x00,
  RX_ALLOC_ERROR    = 0x01
} RxAllocStatusTypeDef;

typedef struct
{
  struct pbuf_custom pbuf_custom;
  uint8_t buff[(ETH_RX_BUF_SIZE + 31) & ~31] __ALIGNED(32);
} RxBuff_t;

/* Memory Pool Declaration */
#define ETH_RX_BUFFER_CNT             12U
LWIP_MEMPOOL_DECLARE(RX_POOL, ETH_RX_BUFFER_CNT, sizeof(RxBuff_t), "Zero-copy RX PBUF pool");

/* Variable Definitions */
static uint8_t RxAllocStatus;

ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

/* USER CODE BEGIN 2 */
/* Static buffers for traditional ETH mode - compatible with new HAL API */
#define ETH_RX_BUF_SIZE_STATIC 1524 /* 1518 + 4 (VLAN) + 2 (alignment) */
#define ETH_TX_BUF_SIZE_STATIC 1524

static uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_RX_BUF_SIZE_STATIC] __ALIGNED(32); /* Ethernet Receive Buffer */
static uint8_t Tx_Buff[ETH_TX_DESC_CNT][ETH_TX_BUF_SIZE_STATIC] __ALIGNED(32); /* Ethernet Transmit Buffer */

/* USER CODE END 2 */

osSemaphoreId RxPktSemaphore = NULL;   /* Semaphore to signal incoming packets */
osSemaphoreId TxPktSemaphore = NULL;   /* Semaphore to signal transmit packet complete */

/* Global Ethernet handle */
ETH_HandleTypeDef heth;
ETH_TxPacketConfig TxConfig;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN Private function prototypes for User BSP */

/* USER CODE END Private function prototypes for User BSP */

/* USER CODE BEGIN 3 */

/* USER CODE END 3 */

/* Private functions ---------------------------------------------------------*/
void pbuf_free_custom(struct pbuf *p);

/**
  * @brief  Ethernet Rx Transfer completed callback
  * @param  handlerEth: ETH handler
  * @retval None
  */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *handlerEth)
{
  osSemaphoreRelease(RxPktSemaphore);
}
/**
  * @brief  Ethernet Tx Transfer completed callback
  * @param  handlerEth: ETH handler
  * @retval None
  */
void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *handlerEth)
{
  osSemaphoreRelease(TxPktSemaphore);
}
/**
  * @brief  Ethernet DMA transfer error callback
  * @param  handlerEth: ETH handler
  * @retval None
  */
void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *handlerEth)
{
  if((HAL_ETH_GetDMAError(handlerEth) & ETH_DMASR_RBUS) == ETH_DMASR_RBUS)
  {
     osSemaphoreRelease(RxPktSemaphore);
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH)
*******************************************************************************/
/**
 * @brief In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif)
{
  HAL_StatusTypeDef hal_eth_init_status = HAL_OK;
/* USER CODE BEGIN OS_THREAD_ATTR_CMSIS_RTOS_V2 */
  osThreadAttr_t attributes;
/* USER CODE END OS_THREAD_ATTR_CMSIS_RTOS_V2 */
/* USER CODE BEGIN low_level_init Variables Initialization for User BSP */

/* USER CODE END low_level_init Variables Initialization for User BSP */
  /* Start ETH HAL Init */

   uint8_t MACAddr[6] ;
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1536;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  hal_eth_init_status = HAL_ETH_Init(&heth);

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;

  /* End ETH HAL Init */

  /* Initialize the RX POOL */
  LWIP_MEMPOOL_INIT(RX_POOL);

#if LWIP_ARP || LWIP_ETHERNET

  /* set MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] =  heth.Init.MACAddr[0];
  netif->hwaddr[1] =  heth.Init.MACAddr[1];
  netif->hwaddr[2] =  heth.Init.MACAddr[2];
  netif->hwaddr[3] =  heth.Init.MACAddr[3];
  netif->hwaddr[4] =  heth.Init.MACAddr[4];
  netif->hwaddr[5] =  heth.Init.MACAddr[5];

  /* maximum transfer unit */
  netif->mtu = ETH_MAX_PAYLOAD;

  /* Accept broadcast address and ARP traffic */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  #if LWIP_ARP
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
  #else
    netif->flags |= NETIF_FLAG_BROADCAST;
  #endif /* LWIP_ARP */

  /* create a binary semaphore used for informing ethernetif of frame reception */
  RxPktSemaphore = osSemaphoreNew(1, 0, NULL);

  /* create a binary semaphore used for informing ethernetif of frame transmission */
  TxPktSemaphore = osSemaphoreNew(1, 0, NULL);

  /* create the task that handles the ETH_MAC */
/* USER CODE BEGIN OS_THREAD_NEW_CMSIS_RTOS_V2 */
  memset(&attributes, 0x0, sizeof(osThreadAttr_t));
  attributes.name = "EthIf";
  attributes.stack_size = INTERFACE_THREAD_STACK_SIZE;
  attributes.priority = osPriorityRealtime;
  osThreadNew(ethernetif_input, netif, &attributes);
/* USER CODE END OS_THREAD_NEW_CMSIS_RTOS_V2 */

/* USER CODE BEGIN low_level_init Code 1 for User BSP */

/* USER CODE END low_level_init Code 1 for User BSP */

  if (hal_eth_init_status == HAL_OK)
  {
/* USER CODE BEGIN low_level_init Code 2 for User BSP */
    /* 使用新版HAL API，需要设置必要的回调函数 */

    // 设置接收缓冲区分配回调函数 - 这是新版HAL API必需的
    heth.rxAllocateCallback = HAL_ETH_RxAllocateCallback;
    heth.rxLinkCallback = HAL_ETH_RxLinkCallback;
    heth.txFreeCallback = NULL; // 发送不需要特殊处理
    __HAL_ETH_DMA_ENABLE_IT(&heth, ETH_DMA_IT_NIS | ETH_DMA_IT_R);
    /* 启动ETH中断模式 */
    HAL_ETH_Start_IT(&heth);

    /* 初始化 YT8512C PHY */
    int32_t phy_status = YT8512C_Init(&heth);
    HAL_ETH_WritePHYRegister(&heth, YT8512C_ADDR, YT8512C_DBG_AOR, 0x4000);  // 扩展寄存器偏移
    HAL_ETH_WritePHYRegister(&heth, YT8512C_ADDR, YT8512C_DBG_DR, 0x0003);    // RMII 启用 (bit1=1, bit0=1)
    if (phy_status != YT8512C_STATUS_OK)
    {
        Error_Handler();
    }

    /* 等待 PHY 链路建立 */
    HAL_Delay(1000);

/* USER CODE END low_level_init Code 2 for User BSP */

  }
  else
  {
    Error_Handler();
  }
#endif /* LWIP_ARP || LWIP_ETHERNET */

/* USER CODE BEGIN LOW_LEVEL_INIT */

/* USER CODE END LOW_LEVEL_INIT */
}

/**
 * @brief This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  uint32_t i = 0U;
  struct pbuf *q = NULL;
  err_t errval = ERR_OK;
  ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT] = {0};

  memset(Txbuffer, 0 , ETH_TX_DESC_CNT*sizeof(ETH_BufferTypeDef));

  for(q = p; q != NULL; q = q->next)
  {
    if(i >= ETH_TX_DESC_CNT)
      return ERR_IF;

    Txbuffer[i].buffer = q->payload;
    Txbuffer[i].len = q->len;

    if(i>0)
    {
      Txbuffer[i-1].next = &Txbuffer[i];
    }

    if(q->next == NULL)
    {
      Txbuffer[i].next = NULL;
    }

    i++;
  }

  TxConfig.Length = p->tot_len;
  TxConfig.TxBuffer = Txbuffer;
  TxConfig.pData = p;

  pbuf_ref(p);

  do
  {
    if(HAL_ETH_Transmit_IT(&heth, &TxConfig) == HAL_OK)
    {
      errval = ERR_OK;
    }
    else
    {

      if(HAL_ETH_GetError(&heth) & HAL_ETH_ERROR_BUSY)
      {
        /* Wait for descriptors to become available */
        osSemaphoreAcquire(TxPktSemaphore, ETHIF_TX_TIMEOUT);
        HAL_ETH_ReleaseTxPacket(&heth);
        errval = ERR_BUF;
      }
      else
      {
        /* Other error */
        pbuf_free(p);
        errval =  ERR_IF;
      }
    }
  }while(errval == ERR_BUF);

  return errval;
}

/**
 * @brief Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
   */
static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL;

  if(RxAllocStatus == RX_ALLOC_OK)
  {
    HAL_ETH_ReadData(&heth, (void **)&p);
  }

  return p;
}

/**
 * @brief This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input(void* argument)
{
  struct pbuf *p = NULL;
  struct netif *netif = (struct netif *) argument;

  for( ;; )
  {
    if (osSemaphoreAcquire(RxPktSemaphore, TIME_WAITING_FOR_INPUT) == osOK)
    {
      do
      {
        p = low_level_input( netif );
        if (p != NULL)
        {
          if (netif->input( p, netif) != ERR_OK )
          {
            pbuf_free(p);
          }
        }
      } while(p!=NULL);
    }
  }
}

#if !LWIP_ARP
/**
 * This function has to be completed by user in case of ARP OFF.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if ...
 */
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
  err_t errval;
  errval = ERR_OK;

/* USER CODE BEGIN 5 */

/* USER CODE END 5 */

  return errval;

}
#endif /* LWIP_ARP */

/**
 * @brief Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  // MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
#if LWIP_ARP
  netif->output = etharp_output;
#else
  /* The user should write its own code in low_level_output_arp_off function */
  netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP */
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

/**
  * @brief  Custom Rx pbuf free callback
  * @param  pbuf: pbuf to be freed
  * @retval None
  */
void pbuf_free_custom(struct pbuf *p)
{
  struct pbuf_custom* custom_pbuf = (struct pbuf_custom*)p;
  LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);

  /* If the Rx Buffer Pool was exhausted, signal the ethernetif_input task to
   * call HAL_ETH_GetRxDataBuffer to rebuild the Rx descriptors. */

  if (RxAllocStatus == RX_ALLOC_ERROR)
  {
    RxAllocStatus = RX_ALLOC_OK;
    osSemaphoreRelease(RxPktSemaphore);
  }
}

/* USER CODE BEGIN 6 */

/**
* @brief Returns the current time in milliseconds
* when LWIP_TIMERS == 1 and NO_SYS == 1
* @param None
* @retval Current Time value
*/
u32_t sys_now(void)
{
  return HAL_GetTick();
}

/* USER CODE END 6 */

/* USER CODE BEGIN PHI IO Functions for User BSP */

/* USER CODE END PHI IO Functions for User BSP */

/**
  * @brief  Check the ETH link state then update ETH driver and netif link accordingly.
  * @retval None
  */
void ethernet_link_thread(void* argument)
{

/* USER CODE BEGIN ETH link init */
    extern struct netif gnetif;
    int32_t phy_link_state = YT8512C_STATUS_LINK_DOWN;
    int32_t prev_link_state = YT8512C_STATUS_LINK_DOWN;
    uint32_t error_count = 0;
    uint32_t consecutive_errors = 0;
    const uint32_t max_consecutive_errors = 10;
/* USER CODE END ETH link init */

  for(;;)
  {

/* USER CODE BEGIN ETH link Thread core code for User BSP */
        /* 获取 YT8512C PHY 链路状态 */
        phy_link_state = YT8512C_GetLinkState(&heth);

        /* 处理PHY通信错误 */
        if (phy_link_state == YT8512C_STATUS_READ_ERROR)
        {
            error_count++;
            consecutive_errors++;

            /* 如果连续错误过多，尝试重新初始化PHY */
            if (consecutive_errors >= max_consecutive_errors)
            {
                /* 重新初始化PHY */
                int32_t init_result = YT8512C_Init(&heth);
                if (init_result == YT8512C_STATUS_OK)
                {
                    consecutive_errors = 0;
                    prev_link_state = YT8512C_STATUS_LINK_DOWN; // 强制状态检查
                }
                else
                {
                    consecutive_errors = max_consecutive_errors / 2; // 减少连续错误计数，避免无限重试
                }
            }

            /* 跳过本次状态更新 */
            osDelay(500); // 延长等待时间
            continue;
        }

        /* 成功读取PHY状态，重置连续错误计数 */
        consecutive_errors = 0;

        /* 检查链路状态是否发生变化 */
        if (phy_link_state != prev_link_state)
        {
            switch (phy_link_state)
            {
                case YT8512C_STATUS_100MBITS_FULLDUPLEX:
                case YT8512C_STATUS_100MBITS_HALFDUPLEX:
                case YT8512C_STATUS_10MBITS_FULLDUPLEX:
                case YT8512C_STATUS_10MBITS_HALFDUPLEX:
                    /* 链路已建立 */
                    if (prev_link_state == YT8512C_STATUS_LINK_DOWN ||
                        prev_link_state == YT8512C_STATUS_AUTONEGO_NOTDONE)
                    {
                        /* 启动以太网 */
                        HAL_ETH_Start_IT(&heth);
                        netif_set_link_up(&gnetif);
                        netif_set_up(&gnetif);
                    }
                    break;

                case YT8512C_STATUS_LINK_DOWN:
                    /* 链路断开 */
                    if (prev_link_state != YT8512C_STATUS_LINK_DOWN)
                    {
                        HAL_ETH_Stop_IT(&heth);
                        netif_set_link_down(&gnetif);
                        netif_set_down(&gnetif);
                    }
                    break;

                case YT8512C_STATUS_AUTONEGO_NOTDONE:
                    /* 自动协商进行中，继续等待 */
                    break;

                default:
                    /* 其他状态或错误 */
                    break;
            }

            prev_link_state = phy_link_state;
        }
/* USER CODE END ETH link Thread core code for User BSP */

    osDelay(100);
  }
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
/* USER CODE BEGIN HAL ETH RxAllocateCallback */
  // 进入临界区保护内存池分配
  taskENTER_CRITICAL();

  struct pbuf_custom *p = LWIP_MEMPOOL_ALLOC(RX_POOL);
    if (p)
    {
      /* Get the buff from the struct pbuf address. */
      *buff = (uint8_t *)p + offsetof(RxBuff_t, buff);
      p->custom_free_function = pbuf_free_custom;
      /* Initialize the struct pbuf.
      * This must be performed whenever a buffer's allocated because it may be
      * changed by lwIP or the app, e.g., pbuf_free decrements ref. */
      pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff, ETH_RX_BUF_SIZE);
    }
    else
    {
      RxAllocStatus = RX_ALLOC_ERROR;
      *buff = NULL;
    }

  // 退出临界区
  taskEXIT_CRITICAL();
/* USER CODE END HAL ETH RxAllocateCallback */
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
/* USER CODE BEGIN HAL ETH RxLinkCallback */
  // 进入临界区保护pbuf链表操作
  taskENTER_CRITICAL();

  struct pbuf **ppStart = (struct pbuf **)pStart;
  struct pbuf **ppEnd = (struct pbuf **)pEnd;
  struct pbuf *p = NULL;

  /* Get the struct pbuf from the buff address. */
  p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
  p->next = NULL;
  p->tot_len = 0;
  p->len = Length;

  /* Chain the buffer. */
  if (!*ppStart)
  {
    /* The first buffer of the packet. */
    *ppStart = p;
  }
  else
  {
    /* Chain the buffer to the end of the packet. */
    (*ppEnd)->next = p;
  }
  *ppEnd = p;

  /* Update the total length of all the buffers of the chain. Each pbuf in the chain should have its tot_len
   * set to its own length, plus the length of all the following pbufs in the chain. */
  for (p = *ppStart; p != NULL; p = p->next)
  {
    p->tot_len += Length;
  }

  // 退出临界区
  taskEXIT_CRITICAL();
/* USER CODE END HAL ETH RxLinkCallback */
}

void HAL_ETH_TxFreeCallback(uint32_t * buff)
{
/* USER CODE BEGIN HAL ETH TxFreeCallback */
  // 进入临界区保护pbuf释放操作
  taskENTER_CRITICAL();

  pbuf_free((struct pbuf *)buff);

  // 退出临界区
  taskEXIT_CRITICAL();
/* USER CODE END HAL ETH TxFreeCallback */
}

/* USER CODE BEGIN 8 */

/* USER CODE END 8 */

