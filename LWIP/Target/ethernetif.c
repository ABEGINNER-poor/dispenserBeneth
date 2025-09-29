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
#include "usbd_cdc_if.h"
/* USER CODE END Include for User BSP */
#include <string.h>
#include "cmsis_os.h"
#include "lwip/tcpip.h"

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN 0 */
extern ETH_HandleTypeDef heth;  // 声明heth以解决undeclared错误

/* 调试输出开关 - 设为0以减少调试信息 */
#define ETHERNETIF_DEBUG_ENABLED 0

/* YT8512C PHY IO函数声明 */
static int32_t YT8512C_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pData);
static int32_t YT8512C_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t Data);
static int32_t YT8512C_GetTick(void);
static int32_t YT8512C_Init(void);
static int32_t YT8512C_DeInit(void);

/* YT8512C PHY 对象和IO函数 */
yt8512c_object_t yt8512c;
yt8512c_ioc_tx_t yt8512c_ioctx = {YT8512C_Init, YT8512C_DeInit, YT8512C_WriteReg, YT8512C_ReadReg, YT8512C_GetTick};

/* PHY诊断函数：扫描所有可能的PHY地址 */
void PHY_Scan_All_Addresses(void) {
    char debug_msg[64];
    uint32_t reg_val = 0;
    
    CDC_Transmit_FS((uint8_t *)"=== PHY Address Scan ===\r\n", 27);
    
    for (uint32_t addr = 0; addr <= 31; addr++) {
        if (HAL_ETH_ReadPHYRegister(&heth, addr, 0x02, &reg_val) == HAL_OK) {
            if (reg_val != 0x0000 && reg_val != 0xFFFF) {
                snprintf(debug_msg, sizeof(debug_msg), "PHY Found at 0x%02lX, ID1: 0x%04lX\r\n", 
                        addr, reg_val & 0xFFFF);
                CDC_Transmit_FS((uint8_t *)debug_msg, strlen(debug_msg));
                
                // 读取ID2寄存器
                if (HAL_ETH_ReadPHYRegister(&heth, addr, 0x03, &reg_val) == HAL_OK) {
                    snprintf(debug_msg, sizeof(debug_msg), "  ID2: 0x%04lX\r\n", reg_val & 0xFFFF);
                    CDC_Transmit_FS((uint8_t *)debug_msg, strlen(debug_msg));
                }
            }
        }
        HAL_Delay(10); // 小延迟避免过快访问
    }
    CDC_Transmit_FS((uint8_t *)"=== Scan Complete ===\r\n", 24);
}
static int32_t YT8512C_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pData) {
    if (HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pData) == HAL_OK) {
        // 调试输出(可选)
        #ifdef DEBUG_PHY_REG
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Read Reg 0x%02X: 0x%04X\r\n", RegAddr, *pData & 0xFFFF);
        CDC_Transmit_FS((uint8_t *)buffer, strlen(buffer));
        #endif
        return YT8512C_STATUS_OK;
    }
    CDC_Transmit_FS((uint8_t *)"Read Error\r\n", 11);
    return YT8512C_STATUS_READ_ERROR;
}
static int32_t YT8512C_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t Data) {
  return (HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, Data) == HAL_OK) ? YT8512C_STATUS_OK : YT8512C_STATUS_WRITE_ERROR;
}
static int32_t YT8512C_GetTick(void) {  // 包装以解决类型警告
  return (int32_t)HAL_GetTick();
}
static int32_t YT8512C_Init(void) { return YT8512C_STATUS_OK; }  // 可空实现，HAL已处理
static int32_t YT8512C_DeInit(void) { return YT8512C_STATUS_OK; }  // 可空实现
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

/* USER CODE END 2 */

osSemaphoreId RxPktSemaphore = NULL;   /* Semaphore to signal incoming packets */
osSemaphoreId TxPktSemaphore = NULL;   /* Semaphore to signal transmit packet complete */

/* Global Ethernet handle */
ETH_HandleTypeDef heth;
ETH_TxPacketConfig TxConfig;

/* Private function prototypes -----------------------------------------------*/
static void ethernetif_input(void const * argument);
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
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
  #else
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_IGMP;
  #endif /* LWIP_ARP */

  /* create a binary semaphore used for informing ethernetif of frame reception */
  osSemaphoreDef(RxSem);
  RxPktSemaphore = osSemaphoreCreate(osSemaphore(RxSem), 1);

  /* create a binary semaphore used for informing ethernetif of frame transmission */
  osSemaphoreDef(TxSem);
  TxPktSemaphore = osSemaphoreCreate(osSemaphore(TxSem), 1);

  /* Decrease the semaphore's initial count from 1 to 0 */
  osSemaphoreWait(RxPktSemaphore, 0);
  osSemaphoreWait(TxPktSemaphore, 0);

  /* create the task that handles the ETH_MAC */
/* USER CODE BEGIN OS_THREAD_DEF_CREATE_CMSIS_RTOS_V1 */
  osThreadDef(EthIf, ethernetif_input, osPriorityRealtime, 0, INTERFACE_THREAD_STACK_SIZE);
  osThreadCreate (osThread(EthIf), netif);
/* USER CODE END OS_THREAD_DEF_CREATE_CMSIS_RTOS_V1 */

/* USER CODE BEGIN low_level_init Code 1 for User BSP */

/* USER CODE END low_level_init Code 1 for User BSP */

  if (hal_eth_init_status == HAL_OK)
  {
/* USER CODE BEGIN low_level_init Code 2 for User BSP */
    // ❗️❗️❗️ 关键修复：初始化YT8512C PHY ❗️❗️❗️
    CDC_Transmit_FS((uint8_t*)"🔧 开始初始化YT8512C PHY...\r\n", 32);
    
    // 设置YT8512C对象的IO接口
    yt8512c.io.init = yt8512c_ioctx.init;
    yt8512c.io.deinit = yt8512c_ioctx.deinit;
    yt8512c.io.readreg = yt8512c_ioctx.readreg;
    yt8512c.io.writereg = yt8512c_ioctx.writereg;
    yt8512c.io.gettick = yt8512c_ioctx.gettick;
    
    // 注册YT8512C对象到ETH句柄
    if (yt8512c_regster_bus_io(&yt8512c, &yt8512c_ioctx) != YT8512C_STATUS_OK) {
        CDC_Transmit_FS((uint8_t*)"❌ YT8512C注册IO失败\r\n", 24);
        Error_Handler();
    }
    
    // PHY地址通常是0或1，尝试自动检测
    uint32_t phyaddr = 0;
    for (uint32_t addr = 0; addr <= 31; addr++) {
        uint32_t id;
        if (HAL_ETH_ReadPHYRegister(&heth, addr, 0x02, &id) == HAL_OK && id != 0xFFFF && id != 0x0000) {
            phyaddr = addr;
            char msg[60];
            sprintf(msg, "✅ 检测到PHY地址: %lu, ID: 0x%04lX\r\n", addr, id);
            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
            break;
        }
    }
    
    // 设置PHY地址到对象中(可能需要手动设置)
    // 这里简化处理，直接使用检测到的地址
    
    // 初始化YT8512C PHY (根据头文件只需要一个参数)
    int32_t init_result = yt8512c_init(&yt8512c);
    if (init_result != YT8512C_STATUS_OK) {
        char msg[60];
        sprintf(msg, "❌ YT8512C初始化失败: %ld\r\n", init_result);
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        Error_Handler();
    }
    
    CDC_Transmit_FS((uint8_t*)"✅ YT8512C PHY初始化成功\r\n", 29);
    
    // 启动自动协商
    if (yt8512c_start_auto_nego(&yt8512c) == YT8512C_STATUS_OK) {
        CDC_Transmit_FS((uint8_t*)"🔄 自动协商已启动\r\n", 22);
    }
    
    // 延时让PHY稳定
    HAL_Delay(100);
    
    // 检查PHY链路状态
    int32_t link_status = yt8512c_get_link_state(&yt8512c);
    if (link_status >= 0) {
        char msg[80];
        if (link_status == YT8512C_STATUS_LINK_DOWN) {
            sprintf(msg, "🔗 PHY链路状态: 断开 (状态=%ld)\r\n", link_status);
        } else {
            // 根据状态值判断连接状态和速度
            const char* speed_duplex = "未知";
            switch (link_status) {
                case YT8512C_STATUS_100MBITS_FULLDUPLEX:
                    speed_duplex = "100Mbps 全双工";
                    break;
                case YT8512C_STATUS_100MBITS_HALFDUPLEX:
                    speed_duplex = "100Mbps 半双工";
                    break;
                case YT8512C_STATUS_10MBITS_FULLDUPLEX:
                    speed_duplex = "10Mbps 全双工";
                    break;
                case YT8512C_STATUS_10MBITS_HALFDUPLEX:
                    speed_duplex = "10Mbps 半双工";
                    break;
                case YT8512C_STATUS_AUTONEGO_NOTDONE:
                    speed_duplex = "自动协商进行中";
                    break;
                default:
                    speed_duplex = "未知状态";
                    break;
            }
            sprintf(msg, "🔗 PHY链路状态: %s (状态=%ld)\r\n", speed_duplex, link_status);
        }
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    } else {
        char msg[60];
        sprintf(msg, "❌ 读取PHY链路状态失败: %ld\r\n", link_status);
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    }
/* USER CODE END low_level_init Code 2 for User BSP */

  }
  else
  {
    Error_Handler();
  }
#endif /* LWIP_ARP || LWIP_ETHERNET */

/* USER CODE BEGIN LOW_LEVEL_INIT */
  
  /* ❗️❗️❗️ 关键修复 - 启动ETH接收中断 ❗️❗️❗️ */
  HAL_StatusTypeDef status = HAL_ETH_Start_IT(&heth);
  if (status != HAL_OK) {
    char msg[50];
    snprintf(msg, sizeof(msg), "❌ ETH_Start_IT失败: %d\r\n", status);
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
    Error_Handler();
  } else {
    CDC_Transmit_FS((uint8_t*)"✅ ETH接收中断已启动\r\n", 24);
  }
  
  /* 启用必要的ETH DMA中断：接收、发送、正常中断摘要 */
  __HAL_ETH_DMA_ENABLE_IT(&heth, ETH_DMA_IT_NIS | ETH_DMA_IT_R | ETH_DMA_IT_T);
  
  CDC_Transmit_FS((uint8_t*)"✅ ETH DMA接收和发送中断已启用\r\n", 35);

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
        osSemaphoreWait(TxPktSemaphore, ETHIF_TX_TIMEOUT);
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
static void ethernetif_input(void const * argument)
{
  struct pbuf *p = NULL;
  struct netif *netif = (struct netif *) argument;

  for( ;; )
  {
    if (osSemaphoreWait(RxPktSemaphore, TIME_WAITING_FOR_INPUT) == osOK)
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
* @brief  Returns the current time in milliseconds
*         when LWIP_TIMERS == 1 and NO_SYS == 1
* @param  None
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

void ethernet_link_thread(void const * argument)
{

/* USER CODE BEGIN ETH link init */

/* USER CODE END ETH link init */

  for(;;)
  {

/* USER CODE BEGIN ETH link Thread core code for User BSP */
    struct netif *netif = (struct netif *) argument;
    
    /* 简化的PHY状态检测 - 使用标准HAL函数 */
    uint32_t regval = 0;
    
    /* 读取PHY基本状态寄存器(BSR, Register 1) */
    if (HAL_ETH_ReadPHYRegister(&heth, 0x00, PHY_BSR, &regval) == HAL_OK) {
        /* 检查Link Status bit (bit 2) */
        if (regval & PHY_LINKED_STATUS) {
            /* Link is up */
            if (!netif_is_link_up(netif)) {
                netif_set_link_up(netif);
            }
        } else {
            /* Link is down */
            if (netif_is_link_up(netif)) {
                netif_set_link_down(netif);
            }
        }
    } else {
        /* PHY read failed - assume link down */
        if (netif_is_link_up(netif)) {
            netif_set_link_down(netif);
        }
    }
/* USER CODE END ETH link Thread core code for User BSP */

    osDelay(100);
  }
}

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
/* USER CODE BEGIN HAL ETH RxAllocateCallback */
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
/* USER CODE END HAL ETH RxAllocateCallback */
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
/* USER CODE BEGIN HAL ETH RxLinkCallback */

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
  *ppEnd  = p;

  /* Update the total length of all the buffers of the chain. Each pbuf in the chain should have its tot_len
   * set to its own length, plus the length of all the following pbufs in the chain. */
  for (p = *ppStart; p != NULL; p = p->next)
  {
    p->tot_len += Length;
  }

/* USER CODE END HAL ETH RxLinkCallback */
}

void HAL_ETH_TxFreeCallback(uint32_t * buff)
{
/* USER CODE BEGIN HAL ETH TxFreeCallback */

  pbuf_free((struct pbuf *)buff);

/* USER CODE END HAL ETH TxFreeCallback */
}

/* USER CODE BEGIN 8 */

/* USER CODE END 8 */

