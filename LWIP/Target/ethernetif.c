/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : ethernetif.c
  * Description        : This file provides code for the configuration
  *                      of the ethernetif.c MiddleWare.
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
#include "ethernet_chip.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"
/* USER CODE END Include for User BSP */
#include <string.h>
#include "cmsis_os.h"
#include "lwip/tcpip.h"

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN 0 */
/**
  * @brief  Send debug message via CDC
  * @param  msg: debug message string
  * @retval None
  */
static void cdc_debug_printf(const char *msg)
{
  uint8_t len = strlen(msg);
  if (len > 0)
  {
    CDC_Transmit_FS((uint8_t*)msg, len);
    HAL_Delay(1); // Small delay to ensure transmission
  }
}

/**
  * @brief  Send debug message with timestamp via CDC
  * @param  prefix: message prefix
  * @param  msg: debug message string
  * @retval None
  */
static void cdc_debug_log(const char *prefix, const char *msg)
{
  char debug_buf[128];
  uint32_t tick = HAL_GetTick();
  snprintf(debug_buf, sizeof(debug_buf), "[%lu] %s: %s\r\n", tick, prefix, msg);
  cdc_debug_printf(debug_buf);
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
/* PHY chip object */
static eth_chip_object_t PHYchip;
static eth_chip_ioc_tx_t  PHYchip_io_ctx;

/* Function prototypes for PHY IO callbacks */
static int32_t PHY_IO_Init(void);
static int32_t PHY_IO_DeInit(void);
static int32_t PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal);
static int32_t PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal);
static int32_t PHY_IO_GetTick(void);
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
/**
  * @brief  Analyze ethernet frame for debugging
  * @param  p: pbuf containing the ethernet frame
  * @param  direction: "RX" or "TX"
  * @retval None
  */
static void ethernet_frame_debug(struct pbuf *p, const char *direction)
{
  if (p->len >= 14) /* Minimum ethernet header size */
  {
    struct eth_hdr *ethhdr = (struct eth_hdr *)p->payload;
    char eth_debug[256];
    
    snprintf(eth_debug, sizeof(eth_debug), 
             "%s Frame - Dst:%02X:%02X:%02X:%02X:%02X:%02X Src:%02X:%02X:%02X:%02X:%02X:%02X Type:0x%04X Len:%d",
             direction,
             ethhdr->dest.addr[0], ethhdr->dest.addr[1], ethhdr->dest.addr[2],
             ethhdr->dest.addr[3], ethhdr->dest.addr[4], ethhdr->dest.addr[5],
             ethhdr->src.addr[0], ethhdr->src.addr[1], ethhdr->src.addr[2],
             ethhdr->src.addr[3], ethhdr->src.addr[4], ethhdr->src.addr[5],
             lwip_ntohs(ethhdr->type), p->tot_len);
    
    cdc_debug_log("ETH_FRAME", eth_debug);
    
    /* Analyze frame type */
    switch (lwip_ntohs(ethhdr->type))
    {
      case ETHTYPE_IP:
        cdc_debug_log("ETH_FRAME", "IPv4 packet detected");
        break;
      case ETHTYPE_ARP:
        cdc_debug_log("ETH_FRAME", "ARP packet detected");
        break;
      case ETHTYPE_IPV6:
        cdc_debug_log("ETH_FRAME", "IPv6 packet detected");
        break;
      default:
        char unknown_msg[32];
        snprintf(unknown_msg, sizeof(unknown_msg), "Unknown protocol: 0x%04X", lwip_ntohs(ethhdr->type));
        cdc_debug_log("ETH_FRAME", unknown_msg);
        break;
    }
  }
}

/**
  * @brief  Debug ETH DMA descriptors
  * @retval None
  */
static void debug_eth_dma_descriptors(void)
{
  char debug_msg[256];
  
  cdc_debug_log("DMA_DESC", "=== RX Descriptors Status ===");
  // 检查RX描述符状态
  for(int i = 0; i < ETH_RX_DESC_CNT; i++) {
    uint32_t desc0 = DMARxDscrTab[i].DESC0;
    uint32_t desc2 = DMARxDscrTab[i].DESC2;
    
    snprintf(debug_msg, sizeof(debug_msg),
             "RX[%d]: OWN=%lu ES=%lu FS=%lu LS=%lu FL=%lu Buf1=0x%08lX",
             i, 
             (desc0 & ETH_DMARXDESC_OWN) ? 1UL : 0UL,      // Owner bit
             (desc0 & ETH_DMARXDESC_ES) ? 1UL : 0UL,       // Error Summary
             (desc0 & ETH_DMARXDESC_FS) ? 1UL : 0UL,       // First Segment
             (desc0 & ETH_DMARXDESC_LS) ? 1UL : 0UL,       // Last Segment
             (desc0 & ETH_DMARXDESC_FL) >> 16,             // Frame Length
             desc2);                                        // Buffer1 Address
    cdc_debug_log("DMA_DESC", debug_msg);
  }
  
  cdc_debug_log("DMA_DESC", "=== TX Descriptors Status ===");
  // 检查TX描述符状态
  for(int i = 0; i < ETH_TX_DESC_CNT; i++) {
    uint32_t desc0 = DMATxDscrTab[i].DESC0;
    uint32_t desc1 = DMATxDscrTab[i].DESC1;
    uint32_t desc2 = DMATxDscrTab[i].DESC2;
    
    snprintf(debug_msg, sizeof(debug_msg),
             "TX[%d]: OWN=%lu ES=%lu FS=%lu LS=%lu TBS1=%lu Buf1=0x%08lX",
             i,
             (desc0 & ETH_DMATXDESC_OWN) ? 1UL : 0UL,      // Owner bit
             (desc0 & ETH_DMATXDESC_ES) ? 1UL : 0UL,       // Error Summary
             (desc0 & ETH_DMATXDESC_FS) ? 1UL : 0UL,       // First Segment
             (desc0 & ETH_DMATXDESC_LS) ? 1UL : 0UL,       // Last Segment
             desc1 & ETH_DMATXDESC_TBS1,                   // Transmit Buffer1 Size
             desc2);                                        // Buffer1 Address
    cdc_debug_log("DMA_DESC", debug_msg);
  }
}

/**
  * @brief  Debug ETH DMA registers
  * @retval None
  */
static void debug_eth_dma_registers(void)
{
  char debug_msg[128];
  
  snprintf(debug_msg, sizeof(debug_msg), "DMA_BMR=0x%08lX", ETH->DMABMR);
  cdc_debug_log("DMA_REG", debug_msg);
  
  snprintf(debug_msg, sizeof(debug_msg), "DMA_SR=0x%08lX", ETH->DMASR);
  cdc_debug_log("DMA_REG", debug_msg);
  
  snprintf(debug_msg, sizeof(debug_msg), "DMA_IER=0x%08lX", ETH->DMAIER);
  cdc_debug_log("DMA_REG", debug_msg);
  
  snprintf(debug_msg, sizeof(debug_msg), "DMA_RDLAR=0x%08lX", ETH->DMARDLAR);
  cdc_debug_log("DMA_REG", debug_msg);
  
  snprintf(debug_msg, sizeof(debug_msg), "DMA_TDLAR=0x%08lX", ETH->DMATDLAR);
  cdc_debug_log("DMA_REG", debug_msg);
}

/**
  * @brief  Debug ETH configuration and status
  * @retval None
  */
static void debug_eth_config(void)
{
  char debug_msg[128];
  
  // MAC configuration
  snprintf(debug_msg, sizeof(debug_msg), "MAC_CR=0x%08lX", ETH->MACCR);
  cdc_debug_log("ETH_CFG", debug_msg);
  
  snprintf(debug_msg, sizeof(debug_msg), "MAC_FCR=0x%08lX", ETH->MACFCR);
  cdc_debug_log("ETH_CFG", debug_msg);
  
  // DMA configuration  
  snprintf(debug_msg, sizeof(debug_msg), "DMA_OMR=0x%08lX", ETH->DMAOMR);
  cdc_debug_log("ETH_CFG", debug_msg);
  
  // Debug descriptor counts
  snprintf(debug_msg, sizeof(debug_msg), "RX_DESC_CNT=%d, TX_DESC_CNT=%d, RX_BUF_CNT=%d", 
           ETH_RX_DESC_CNT, ETH_TX_DESC_CNT, ETH_RX_BUFFER_CNT);
  cdc_debug_log("ETH_CFG", debug_msg);
}
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
  int32_t phy_link_state;
  ETH_MACConfigTypeDef macConfig;
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
    cdc_debug_log("PHY_INIT", "Starting PHY chip initialization");
    
    /* Set up PHY chip IO context */
    PHYchip_io_ctx.init = PHY_IO_Init;
    PHYchip_io_ctx.deinit = PHY_IO_DeInit;
    PHYchip_io_ctx.readreg = PHY_IO_ReadReg;
    PHYchip_io_ctx.writereg = PHY_IO_WriteReg;
    PHYchip_io_ctx.gettick = PHY_IO_GetTick;

    /* Register PHY chip IO functions */
    if (eth_chip_regster_bus_io(&PHYchip, &PHYchip_io_ctx) != ETH_CHIP_STATUS_OK)
    {
      cdc_debug_log("PHY_INIT", "Failed to register PHY IO functions");
      Error_Handler();
    }
    cdc_debug_log("PHY_INIT", "PHY IO functions registered successfully");

    /* Initialize PHY chip */
    if (eth_chip_init(&PHYchip) != ETH_CHIP_STATUS_OK)
    {
      cdc_debug_log("PHY_INIT", "PHY chip initialization failed");
      Error_Handler();
    }
    
    char phy_addr_msg[64];
    snprintf(phy_addr_msg, sizeof(phy_addr_msg), "PHY chip initialized, address: %lu", PHYchip.devaddr);
    cdc_debug_log("PHY_INIT", phy_addr_msg);

    /* Disable PHY power down mode */
    if (eth_chip_disable_power_down_mode(&PHYchip) != ETH_CHIP_STATUS_OK)
    {
      cdc_debug_log("PHY_INIT", "Failed to disable PHY power down mode");
      Error_Handler();
    }
    cdc_debug_log("PHY_INIT", "PHY power down mode disabled");

    /* 启动自动协商 */
    if (eth_chip_start_auto_nego(&PHYchip) != ETH_CHIP_STATUS_OK)
    {
      cdc_debug_log("PHY_INIT", "Failed to start auto-negotiation");
      Error_Handler();
    }
    cdc_debug_log("PHY_INIT", "Auto-negotiation started successfully");

    /* 等待自动协商完成 */
    uint32_t autoneg_timeout = 0;
    int32_t nego_state;
    do {
      HAL_Delay(100);
      nego_state = eth_chip_get_link_state(&PHYchip);
      autoneg_timeout++;
      
      if (autoneg_timeout % 10 == 0) {
        char timeout_msg[64];
        snprintf(timeout_msg, sizeof(timeout_msg), "Auto-nego timeout: %lu, state: %ld", autoneg_timeout, nego_state);
        cdc_debug_log("PHY_INIT", timeout_msg);
      }
      
      /* 超时保护 */
      if (autoneg_timeout > 50) { // 5秒超时
        cdc_debug_log("PHY_INIT", "Auto-negotiation timeout, using current state");
        break;
      }
    } while (nego_state == ETH_CHIP_STATUS_AUTONEGO_NOTDONE || nego_state == ETH_CHIP_STATUS_READ_ERROR);

    /* Get PHY link state and configure ETH accordingly */
    phy_link_state = eth_chip_get_link_state(&PHYchip);
    
    char link_msg[64];
    snprintf(link_msg, sizeof(link_msg), "Initial PHY link state: %ld", phy_link_state);
    cdc_debug_log("PHY_INIT", link_msg);
    
    if (phy_link_state != ETH_CHIP_STATUS_READ_ERROR)
    {
      /* Get current MAC configuration */
      HAL_ETH_GetMACConfig(&heth, &macConfig);
      
      switch (phy_link_state)
      {
        case ETH_CHIP_STATUS_100MBITS_FULLDUPLEX:
          macConfig.DuplexMode = ETH_FULLDUPLEX_MODE;
          macConfig.Speed = ETH_SPEED_100M;
          cdc_debug_log("PHY_INIT", "Configured: 100M Full Duplex");
          break;
        case ETH_CHIP_STATUS_100MBITS_HALFDUPLEX:
          macConfig.DuplexMode = ETH_HALFDUPLEX_MODE;
          macConfig.Speed = ETH_SPEED_100M;
          cdc_debug_log("PHY_INIT", "Configured: 100M Half Duplex");
          break;
        case ETH_CHIP_STATUS_10MBITS_FULLDUPLEX:
          macConfig.DuplexMode = ETH_FULLDUPLEX_MODE;
          macConfig.Speed = ETH_SPEED_10M;
          cdc_debug_log("PHY_INIT", "Configured: 10M Full Duplex");
          break;
        case ETH_CHIP_STATUS_10MBITS_HALFDUPLEX:
          macConfig.DuplexMode = ETH_HALFDUPLEX_MODE;
          macConfig.Speed = ETH_SPEED_10M;
          cdc_debug_log("PHY_INIT", "Configured: 10M Half Duplex");
          break;
        default:
          macConfig.DuplexMode = ETH_FULLDUPLEX_MODE;
          macConfig.Speed = ETH_SPEED_100M;
          cdc_debug_log("PHY_INIT", "Default: 100M Full Duplex");
          break;
      }
      

    }

    /* 重要：在启动ETH中断之前，不要调用HAL_ETH_Start_IT！
     * 信号量必须先创建，否则会在中断回调中造成HardFault */
    cdc_debug_log("PHY_INIT", "PHY configuration completed, deferring ETH start until after semaphore creation");
    
    /* Debug ETH configuration after initialization */
    cdc_debug_log("ETH_INIT", "=== ETH Initialization Debug ===");
    debug_eth_config();
    debug_eth_dma_registers();
    debug_eth_dma_descriptors();
    cdc_debug_log("ETH_INIT", "=== ETH Debug Complete ===");
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

/**
  * @brief  Initialize the PHY interface
  * @retval 0 if OK, -1 if ERROR
  */
static int32_t PHY_IO_Init(void)
{
  /* 基本的PHY硬件初始化已经由HAL_ETH_Init处理 */
  /* 这里进行PHY芯片特定的初始化 */
  cdc_debug_log("PHY_IO", "PHY_IO_Init called - performing PHY-specific initialization");
  
  /* 等待PHY准备就绪 */
  HAL_Delay(10);
  
  cdc_debug_log("PHY_IO", "PHY_IO_Init completed successfully");
  return 0;
}

/**
  * @brief  De-Initialize the PHY interface
  * @retval 0 if OK, -1 if ERROR
  */
static int32_t PHY_IO_DeInit(void)
{
  cdc_debug_log("PHY_IO", "PHY_IO_DeInit called - performing PHY-specific cleanup");
  
  /* 可以在这里添加特定的PHY去初始化操作 */
  /* 例如：使能省电模式 */
  
  cdc_debug_log("PHY_IO", "PHY_IO_DeInit completed");
  return 0;
}

/**
  * @brief  Read PHY register through SMI interface
  * @param  DevAddr: PHY device address
  * @param  RegAddr: PHY register address
  * @param  pRegVal: Pointer to register value
  * @retval 0 if OK, -1 if ERROR
  */
static int32_t PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if (HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    char error_msg[64];
    snprintf(error_msg, sizeof(error_msg), "Read failed - Addr:0x%02lX Reg:0x%02lX", DevAddr, RegAddr);
    cdc_debug_log("PHY_IO", error_msg);
    return -1;
  }
  return 0;
}

/**
  * @brief  Write PHY register through SMI interface
  * @param  DevAddr: PHY device address
  * @param  RegAddr: PHY register address
  * @param  RegVal: Register value to write
  * @retval 0 if OK, -1 if ERROR
  */
static int32_t PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if (HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    char error_msg[64];
    snprintf(error_msg, sizeof(error_msg), "Write failed - Addr:0x%02lX Reg:0x%02lX Val:0x%04lX", DevAddr, RegAddr, RegVal);
    cdc_debug_log("PHY_IO", error_msg);
    return -1;
  }
  return 0;
}

/**
  * @brief  Get system tick for timing
  * @retval Current tick value
  */
static int32_t PHY_IO_GetTick(void)
{
  return HAL_GetTick();
}

/* USER CODE END PHI IO Functions for User BSP */

/**
  * @brief  Check the ETH link state then update ETH driver and netif link accordingly.
  * @retval None
  */

void ethernet_link_thread(void const * argument)
{

/* USER CODE BEGIN ETH link init */
  struct netif *netif = (struct netif *) argument;
  int32_t phy_link_state;
  ETH_MACConfigTypeDef macConfig;
  uint32_t linkchanged = 0, linkup = 0;
  uint32_t link_check_counter = 0;
  
  cdc_debug_log("LINK_THREAD", "Ethernet link monitoring started");
/* USER CODE END ETH link init */

  for(;;)
  {

/* USER CODE BEGIN ETH link Thread core code for User BSP */
    link_check_counter++;
    
    /* Only log every 50th check to avoid flooding (approximately every 5 seconds) */
    if (link_check_counter % 50 == 0)
    {
      char counter_msg[32];
      snprintf(counter_msg, sizeof(counter_msg), "Link check #%lu", link_check_counter);
      cdc_debug_log("LINK_THREAD", counter_msg);
    }
    
    phy_link_state = eth_chip_get_link_state(&PHYchip);
    
    if (phy_link_state != ETH_CHIP_STATUS_READ_ERROR && phy_link_state != ETH_CHIP_STATUS_LINK_DOWN)
    {
      /* PHY link is up */
      switch (phy_link_state)
      {
        case ETH_CHIP_STATUS_100MBITS_FULLDUPLEX:
          if (link_check_counter % 50 == 0) cdc_debug_log("LINK_THREAD", "Link: 100M Full Duplex");
          linkup = 1;
          break;
        case ETH_CHIP_STATUS_100MBITS_HALFDUPLEX:
          if (link_check_counter % 50 == 0) cdc_debug_log("LINK_THREAD", "Link: 100M Half Duplex");
          linkup = 1;
          break;
        case ETH_CHIP_STATUS_10MBITS_FULLDUPLEX:
          if (link_check_counter % 50 == 0) cdc_debug_log("LINK_THREAD", "Link: 10M Full Duplex");
          linkup = 1;
          break;
        case ETH_CHIP_STATUS_10MBITS_HALFDUPLEX:
          if (link_check_counter % 50 == 0) cdc_debug_log("LINK_THREAD", "Link: 10M Half Duplex");
          linkup = 1;
          break;
        case ETH_CHIP_STATUS_AUTONEGO_NOTDONE:
          if (link_check_counter % 50 == 0) {
            cdc_debug_log("LINK_THREAD", "Auto-negotiation in progress, restarting...");
            eth_chip_start_auto_nego(&PHYchip);
          }
          linkup = 0;
          break;
        default:
          if (link_check_counter % 50 == 0) cdc_debug_log("LINK_THREAD", "Link: Unknown state");
          linkup = 0;
          break;
      }
      
      if (linkup && !netif_is_link_up(netif))
      {
        cdc_debug_log("LINK_THREAD", "Link UP detected - configuring interface");
        
        /* Get current MAC configuration */
        HAL_ETH_GetMACConfig(&heth, &macConfig);
        
        /* Configure ETH DupLex and Speed based on PHY state */
        switch (phy_link_state)
        {
          case ETH_CHIP_STATUS_100MBITS_FULLDUPLEX:
            macConfig.DuplexMode = ETH_FULLDUPLEX_MODE;
            macConfig.Speed = ETH_SPEED_100M;
            cdc_debug_log("LINK_THREAD", "Configuring: 100M Full Duplex");
            break;
          case ETH_CHIP_STATUS_100MBITS_HALFDUPLEX:
            macConfig.DuplexMode = ETH_HALFDUPLEX_MODE;
            macConfig.Speed = ETH_SPEED_100M;
            cdc_debug_log("LINK_THREAD", "Configuring: 100M Half Duplex");
            break;
          case ETH_CHIP_STATUS_10MBITS_FULLDUPLEX:
            macConfig.DuplexMode = ETH_FULLDUPLEX_MODE;
            macConfig.Speed = ETH_SPEED_10M;
            cdc_debug_log("LINK_THREAD", "Configuring: 10M Full Duplex");
            break;
          case ETH_CHIP_STATUS_10MBITS_HALFDUPLEX:
            macConfig.DuplexMode = ETH_HALFDUPLEX_MODE;
            macConfig.Speed = ETH_SPEED_10M;
            cdc_debug_log("LINK_THREAD", "Configuring: 10M Half Duplex");
            break;
        }
        
        /* Apply the new MAC configuration */
        if (HAL_ETH_SetMACConfig(&heth, &macConfig) != HAL_OK)
        {
          cdc_debug_log("LINK_THREAD", "Failed to set MAC configuration");
        }
        else
        {
          cdc_debug_log("LINK_THREAD", "MAC configuration updated successfully");
        }
        
        /* Start ETH if not already started */
        if (HAL_ETH_Start_IT(&heth) != HAL_OK)
        {
          cdc_debug_log("LINK_THREAD", "Failed to start ETH");
        }
        else
        {
          cdc_debug_log("LINK_THREAD", "ETH started successfully");
        }
        
        /* Notify link up */
        netif_set_link_up(netif);
        cdc_debug_log("LINK_THREAD", "Network interface link set to UP");
        linkchanged = 1;
      }
    }
    else
    {
      /* PHY link is down */
      if (netif_is_link_up(netif))
      {
        cdc_debug_log("LINK_THREAD", "Link DOWN detected - stopping interface");
        
        /* Stop ETH */
        if (HAL_ETH_Stop_IT(&heth) != HAL_OK)
        {
          cdc_debug_log("LINK_THREAD", "Failed to stop ETH");
        }
        else
        {
          cdc_debug_log("LINK_THREAD", "ETH stopped successfully");
        }
        
        /* Notify link down */
        netif_set_link_down(netif);
        cdc_debug_log("LINK_THREAD", "Network interface link set to DOWN");
        linkchanged = 1;
      }
      else if (link_check_counter % 50 == 0)
      {
        if (phy_link_state == ETH_CHIP_STATUS_READ_ERROR)
        {
          cdc_debug_log("LINK_THREAD", "PHY read error detected");
        }
        else if (phy_link_state == ETH_CHIP_STATUS_LINK_DOWN)
        {
          cdc_debug_log("LINK_THREAD", "Link is down, restarting auto-negotiation");
          /* 尝试重新启动自动协商 */
          if (eth_chip_start_auto_nego(&PHYchip) == ETH_CHIP_STATUS_OK)
          {
            cdc_debug_log("LINK_THREAD", "Auto-negotiation restarted");
          }
        }
        else
        {
          char state_msg[64];
          snprintf(state_msg, sizeof(state_msg), "Unknown PHY state: %ld", phy_link_state);
          cdc_debug_log("LINK_THREAD", state_msg);
        }
      }
      linkup = 0;
    }
    
    /* Reset link change flag */
    if (linkchanged)
    {
      linkchanged = 0;
      /* Debug DMA status after link change */
      cdc_debug_log("LINK_THREAD", "=== DMA Status After Link Change ===");
      debug_eth_dma_registers();
      cdc_debug_log("LINK_THREAD", "=== DMA Debug Complete ===");
    }
    
    /* Periodic DMA debug every 100 cycles (approximately 10 seconds) */
    if (link_check_counter % 100 == 0)
    {
      cdc_debug_log("LINK_THREAD", "=== Periodic DMA Status Check ===");
      debug_eth_dma_registers();
      cdc_debug_log("LINK_THREAD", "=== Periodic Debug Complete ===");
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

