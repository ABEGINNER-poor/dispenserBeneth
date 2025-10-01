/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * File Name          : LWIP.c
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
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
#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#if defined ( __CC_ARM )  /* MDK ARM Compiler */
#include "lwip/sio.h"
#endif /* MDK ARM Compiler */
#include "ethernetif.h"

/* USER CODE BEGIN 0 */
#include "usbd_cdc_if.h"
#include <stdio.h>

/**
  * @brief  Send debug message via CDC
  * @param  msg: debug message string
  * @retval None
  */
static void lwip_cdc_debug_printf(const char *msg)
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
static void lwip_cdc_debug_log(const char *prefix, const char *msg)
{
  char debug_buf[128];
  uint32_t tick = HAL_GetTick();
  snprintf(debug_buf, sizeof(debug_buf), "[%lu] %s: %s\r\n", tick, prefix, msg);
  lwip_cdc_debug_printf(debug_buf);
}
/* USER CODE END 0 */
/* Private function prototypes -----------------------------------------------*/
static void ethernet_link_status_updated(struct netif *netif);
/* ETH Variables initialization ----------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN 1 */
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/raw.h"
#include "lwip/stats.h"

/**
  * @brief  Get network interface status
  * @retval None
  */
void lwip_get_interface_status(void)
{
  char status_msg[256];
  
  if (netif_is_up(&gnetif))
  {
    if (netif_is_link_up(&gnetif))
    {
      snprintf(status_msg, sizeof(status_msg), 
               "Interface: UP, Link: UP, IP: %s, Netmask: %s, Gateway: %s",
               ip4addr_ntoa(netif_ip4_addr(&gnetif)),
               ip4addr_ntoa(netif_ip4_netmask(&gnetif)),
               ip4addr_ntoa(netif_ip4_gw(&gnetif)));
    }
    else
    {
      snprintf(status_msg, sizeof(status_msg), "Interface: UP, Link: DOWN");
    }
  }
  else
  {
    snprintf(status_msg, sizeof(status_msg), "Interface: DOWN");
  }
  
  lwip_cdc_debug_log("NET_STATUS", status_msg);
}

/**
  * @brief  Send ping to specified IP address
  * @param  target_ip: target IP address string (e.g., "192.168.1.1")
  * @retval None
  */
void lwip_ping(const char* target_ip)
{
  char ping_msg[64];
  snprintf(ping_msg, sizeof(ping_msg), "Attempting to ping %s", target_ip);
  lwip_cdc_debug_log("PING", ping_msg);
  
  // Note: This is a simple notification. For actual ping implementation,
  // you would need to implement ICMP echo request/reply handling
}

/**
  * @brief  Print network statistics
  * @retval None
  */
void lwip_print_stats(void)
{
  lwip_cdc_debug_log("NET_STATS", "=== Network Interface Information ===");
  
  char stats_msg[128];
  
  // Interface basic information
  snprintf(stats_msg, sizeof(stats_msg), "Interface Name: %c%c", 
           gnetif.name[0], gnetif.name[1]);
  lwip_cdc_debug_log("NET_STATS", stats_msg);
  
  snprintf(stats_msg, sizeof(stats_msg), "MTU: %d bytes", gnetif.mtu);
  lwip_cdc_debug_log("NET_STATS", stats_msg);
  
  // Interface flags
  char flags_str[128] = "";
  if (gnetif.flags & NETIF_FLAG_UP) strcat(flags_str, "UP ");
  if (gnetif.flags & NETIF_FLAG_LINK_UP) strcat(flags_str, "LINK_UP ");
  if (gnetif.flags & NETIF_FLAG_BROADCAST) strcat(flags_str, "BROADCAST ");
  if (gnetif.flags & NETIF_FLAG_ETHARP) strcat(flags_str, "ETHARP ");
  if (gnetif.flags & NETIF_FLAG_ETHERNET) strcat(flags_str, "ETHERNET ");
  
  snprintf(stats_msg, sizeof(stats_msg), "Flags: %s", flags_str);
  lwip_cdc_debug_log("NET_STATS", stats_msg);
  
  // MAC address
  snprintf(stats_msg, sizeof(stats_msg), "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
           gnetif.hwaddr[0], gnetif.hwaddr[1], gnetif.hwaddr[2],
           gnetif.hwaddr[3], gnetif.hwaddr[4], gnetif.hwaddr[5]);
  lwip_cdc_debug_log("NET_STATS", stats_msg);
  
#if LWIP_STATS
  // If statistics are enabled, show them
  lwip_cdc_debug_log("NET_STATS", "=== LwIP Statistics ===");
  
  snprintf(stats_msg, sizeof(stats_msg), "ETH RX: %lu, TX: %lu, Drop: %lu, Err: %lu",
           lwip_stats.link.recv, lwip_stats.link.xmit, 
           lwip_stats.link.drop, lwip_stats.link.err);
  lwip_cdc_debug_log("NET_STATS", stats_msg);
  
  snprintf(stats_msg, sizeof(stats_msg), "IP RX: %lu, TX: %lu, Drop: %lu, Err: %lu",
           lwip_stats.ip.recv, lwip_stats.ip.xmit,
           lwip_stats.ip.drop, lwip_stats.ip.err);
  lwip_cdc_debug_log("NET_STATS", stats_msg);
#else
  lwip_cdc_debug_log("NET_STATS", "LwIP statistics disabled (LWIP_STATS=0)");
  lwip_cdc_debug_log("NET_STATS", "Enable LWIP_STATS in lwipopts.h for detailed stats");
#endif
}
/* USER CODE END 1 */

/* Variables Initialization */
struct netif gnetif;
ip4_addr_t ipaddr;
ip4_addr_t netmask;
ip4_addr_t gw;
uint8_t IP_ADDRESS[4];
uint8_t NETMASK_ADDRESS[4];
uint8_t GATEWAY_ADDRESS[4];

/* USER CODE BEGIN 2 */
/**
  * @brief  Test network connectivity and print diagnostics
  * @retval None
  */
void lwip_network_test(void)
{
  lwip_cdc_debug_log("NET_TEST", "=== Network Connectivity Test ===");
  
  // Wait a bit for network to stabilize
  osDelay(2000);
  
  // Print interface status
  lwip_get_interface_status();
  
  // Print network statistics
  lwip_print_stats();
  
  // Test ping to common addresses
  lwip_ping("192.168.1.1");   // Common router IP
  lwip_ping("8.8.8.8");       // Google DNS
  lwip_ping("192.168.10.1");  // Local subnet gateway
  
  lwip_cdc_debug_log("NET_TEST", "Network test completed");
}

/**
  * @brief  Periodic network status check (call this in a timer or main loop)
  * @retval None
  */
void lwip_periodic_status_check(void)
{
  static uint32_t last_check = 0;
  uint32_t current_time = HAL_GetTick();
  
  // Check every 10 seconds
  if (current_time - last_check > 10000)
  {
    last_check = current_time;
    lwip_get_interface_status();
  }
}
/* USER CODE END 2 */

/**
  * LwIP initialization function
  */
void MX_LWIP_Init(void)
{
  /* IP addresses initialization */
  IP_ADDRESS[0] = 192;
  IP_ADDRESS[1] = 168;
  IP_ADDRESS[2] = 10;
  IP_ADDRESS[3] = 88;
  NETMASK_ADDRESS[0] = 255;
  NETMASK_ADDRESS[1] = 255;
  NETMASK_ADDRESS[2] = 255;
  NETMASK_ADDRESS[3] = 0;
  GATEWAY_ADDRESS[0] = 0;
  GATEWAY_ADDRESS[1] = 0;
  GATEWAY_ADDRESS[2] = 0;
  GATEWAY_ADDRESS[3] = 0;

/* USER CODE BEGIN IP_ADDRESSES */
  char ip_msg[128];
  snprintf(ip_msg, sizeof(ip_msg), "IP: %d.%d.%d.%d", IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
  lwip_cdc_debug_log("LWIP_INIT", ip_msg);
  
  snprintf(ip_msg, sizeof(ip_msg), "Netmask: %d.%d.%d.%d", NETMASK_ADDRESS[0], NETMASK_ADDRESS[1], NETMASK_ADDRESS[2], NETMASK_ADDRESS[3]);
  lwip_cdc_debug_log("LWIP_INIT", ip_msg);
  
  snprintf(ip_msg, sizeof(ip_msg), "Gateway: %d.%d.%d.%d", GATEWAY_ADDRESS[0], GATEWAY_ADDRESS[1], GATEWAY_ADDRESS[2], GATEWAY_ADDRESS[3]);
  lwip_cdc_debug_log("LWIP_INIT", ip_msg);
/* USER CODE END IP_ADDRESSES */

  /* Initialize the LwIP stack with RTOS */
  tcpip_init( NULL, NULL );

  /* IP addresses initialization without DHCP (IPv4) */
  IP4_ADDR(&ipaddr, IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
  IP4_ADDR(&netmask, NETMASK_ADDRESS[0], NETMASK_ADDRESS[1] , NETMASK_ADDRESS[2], NETMASK_ADDRESS[3]);
  IP4_ADDR(&gw, GATEWAY_ADDRESS[0], GATEWAY_ADDRESS[1], GATEWAY_ADDRESS[2], GATEWAY_ADDRESS[3]);

  /* add the network interface (IPv4/IPv6) with RTOS */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /* Registers the default network interface */
  netif_set_default(&gnetif);

  /* We must always bring the network interface up connection or not... */
  netif_set_up(&gnetif);

  /* Set the link callback function, this function is called on change of link status*/
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);

  /* Create the Ethernet link handler thread */
/* USER CODE BEGIN H7_OS_THREAD_DEF_CREATE_CMSIS_RTOS_V1 */
  lwip_cdc_debug_log("LWIP_INIT", "Creating Ethernet link handler thread");
  osThreadDef(EthLink, ethernet_link_thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE *2);
  osThreadCreate (osThread(EthLink), &gnetif);
  lwip_cdc_debug_log("LWIP_INIT", "Ethernet link handler thread created");
/* USER CODE END H7_OS_THREAD_DEF_CREATE_CMSIS_RTOS_V1 */

/* USER CODE BEGIN 3 */
  lwip_cdc_debug_log("LWIP_INIT", "LwIP initialization completed successfully");
/* USER CODE END 3 */
}

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
#endif

/**
  * @brief  Notify the User about the network interface config status
  * @param  netif: the network interface
  * @retval None
  */
static void ethernet_link_status_updated(struct netif *netif)
{
  if (netif_is_up(netif))
  {
/* USER CODE BEGIN 5 */
    lwip_cdc_debug_log("LINK_STATUS", "Network interface is UP");
    
    char status_msg[128];
    snprintf(status_msg, sizeof(status_msg), "Interface UP - IP: %s", ip4addr_ntoa(netif_ip4_addr(netif)));
    lwip_cdc_debug_log("LINK_STATUS", status_msg);
/* USER CODE END 5 */
  }
  else /* netif is down */
  {
/* USER CODE BEGIN 6 */
    lwip_cdc_debug_log("LINK_STATUS", "Network interface is DOWN");
/* USER CODE END 6 */
  }
}

#if defined ( __CC_ARM )  /* MDK ARM Compiler */
/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
  sio_fd_t sd;

/* USER CODE BEGIN 7 */
  sd = 0; // dummy code
/* USER CODE END 7 */

  return sd;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd)
{
/* USER CODE BEGIN 8 */
/* USER CODE END 8 */
}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 9 */
  recved_bytes = 0; // dummy code
/* USER CODE END 9 */
  return recved_bytes;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t *data, u32_t len)
{
  u32_t recved_bytes;

/* USER CODE BEGIN 10 */
  recved_bytes = 0; // dummy code
/* USER CODE END 10 */
  return recved_bytes;
}
#endif /* MDK ARM Compiler */

