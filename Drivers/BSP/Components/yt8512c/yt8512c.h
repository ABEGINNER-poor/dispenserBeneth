/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef YT8512C_H
#define YT8512C_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>


/* PHY芯片寄存器映射表 */
#define YT8512C_BCR                            ((uint16_t)0x0000U)                   /*!< Basic Control Register */
#define YT8512C_BSR                            ((uint16_t)0x0001U)                   /*!< Basic Status Register */
#define YT8512C_PHYID1                         ((uint16_t)0x0002U)                   /*!< PHY Identifier Register 1 */
#define YT8512C_PHYID2                         ((uint16_t)0x0003U)                   /*!< PHY Identifier Register 2 */
#define YT8512C_ANAR                           ((uint16_t)0x0004U)                   /*!< Auto-Negotiation Advertisement Register */
#define YT8512C_ANLPAR                         ((uint16_t)0x0005U)                   /*!< Auto-Negotiation Link Partner Ability Register */
#define YT8512C_ANER                           ((uint16_t)0x0006U)                   /*!< Auto-Negotiation Expansion Register */

/* YT8512C Specific Registers */
#define YT8512C_SMI_PHY                        ((uint16_t)0x0010U)                   /*!< SMI PHY Register */
#define YT8512C_PHYSCSR                        ((uint16_t)0x0011U)                   /*!< PHY Specific Control/Status Register */
#define YT8512C_INTERRUPT                      ((uint16_t)0x0012U)                   /*!< Interrupt Indicators Register */
#define YT8512C_INTR_MASK                      ((uint16_t)0x0013U)                   /*!< Interrupt Mask Register */
#define YT8512C_EXT_REG                        ((uint16_t)0x001EU)                   /*!< Extended Register */

/* Expected PHY ID for YT8512C */
#define YT8512C_PHY_ID                         ((uint32_t)0x0000011A)                /*!< YT8512C PHY ID */

/* YT8512C PHYSCSR Register (0x11) Bit Definitions */
#define YT8512C_PHYSCSR_SPEED_MODE_MASK ((uint16_t)0xC000)  /*!< Speed Mode mask (bit15:14) */
#define YT8512C_PHYSCSR_SPEED_100M     ((uint16_t)0x4000)  /*!< Speed 100Mbps (01) */
#define YT8512C_PHYSCSR_SPEED_10M      ((uint16_t)0x0000)  /*!< Speed 10Mbps (00) */
#define YT8512C_PHYSCSR_DUPLEX_FULL    ((uint16_t)0x2000)  /*!< Full duplex (bit13=1) */
#define YT8512C_PHYSCSR_RESOLVED       ((uint16_t)0x0800)  /*!< Speed/Duplex Resolved (bit11=1) */
#define YT8512C_PHYSCSR_LINK_UP        ((uint16_t)0x0400)  /*!< Link up (bit10=1) */                  /*!< Link up indication */

/* PHY Address */
#define YT8512C_ADDR                           ((uint16_t)0x0000U)
/* PHY Register Count */
#define YT8512C_PHY_COUNT                      ((uint16_t)0x001FU)

/* 操作SCR寄存器的值（一般不需要修改） */
#define YT8512C_BCR_SOFT_RESET                 ((uint16_t)0x8000U)
#define YT8512C_BCR_LOOPBACK                   ((uint16_t)0x4000U)
#define YT8512C_BCR_SPEED_SELECT               ((uint16_t)0x2000U)
#define YT8512C_BCR_AUTONEGO_EN                ((uint16_t)0x1000U)
#define YT8512C_BCR_POWER_DOWN                 ((uint16_t)0x0800U)
#define YT8512C_BCR_ISOLATE                    ((uint16_t)0x0400U)
#define YT8512C_BCR_RESTART_AUTONEGO           ((uint16_t)0x0200U)
#define YT8512C_BCR_DUPLEX_MODE                ((uint16_t)0x0100U)

/* 操作BSR寄存器的值（一般不需要修改） */
#define YT8512C_BSR_100BASE_T4                 ((uint16_t)0x8000U)
#define YT8512C_BSR_100BASE_TX_FD              ((uint16_t)0x4000U)
#define YT8512C_BSR_100BASE_TX_HD              ((uint16_t)0x2000U)
#define YT8512C_BSR_10BASE_T_FD                ((uint16_t)0x1000U)
#define YT8512C_BSR_10BASE_T_HD                ((uint16_t)0x0800U)
#define YT8512C_BSR_100BASE_T2_FD              ((uint16_t)0x0400U)
#define YT8512C_BSR_100BASE_T2_HD              ((uint16_t)0x0200U)
#define YT8512C_BSR_EXTENDED_STATUS            ((uint16_t)0x0100U)
#define YT8512C_BSR_AUTONEGO_CPLT              ((uint16_t)0x0020U)
#define YT8512C_BSR_REMOTE_FAULT               ((uint16_t)0x0010U)
#define YT8512C_BSR_AUTONEGO_ABILITY           ((uint16_t)0x0008U)
#define YT8512C_BSR_LINK_STATUS                ((uint16_t)0x0004U)
#define YT8512C_BSR_JABBER_DETECT              ((uint16_t)0x0002U)
#define YT8512C_BSR_EXTENDED_CAP               ((uint16_t)0x0001U)


/* PHY芯片进程状态 */
#define  YT8512C_STATUS_READ_ERROR             ((int32_t)-5)
#define  YT8512C_STATUS_WRITE_ERROR            ((int32_t)-4)
#define  YT8512C_STATUS_ADDRESS_ERROR          ((int32_t)-3)
#define  YT8512C_STATUS_RESET_TIMEOUT          ((int32_t)-2)
#define  YT8512C_STATUS_ERROR                  ((int32_t)-1)
#define  YT8512C_STATUS_OK                     ((int32_t) 0)
#define  YT8512C_STATUS_LINK_DOWN              ((int32_t) 1)
#define  YT8512C_STATUS_100MBITS_FULLDUPLEX    ((int32_t) 2)
#define  YT8512C_STATUS_100MBITS_HALFDUPLEX    ((int32_t) 3)
#define  YT8512C_STATUS_10MBITS_FULLDUPLEX     ((int32_t) 4)
#define  YT8512C_STATUS_10MBITS_HALFDUPLEX     ((int32_t) 5)
#define  YT8512C_STATUS_AUTONEGO_NOTDONE       ((int32_t) 6)

/* PHY地址 ---- 由用户设置 */
#define YT8512C_ADDR                           ((uint16_t)0x0000U)
/* PHY寄存器的数量 */
#define YT8512C_PHY_COUNT                      ((uint16_t)0x001FU)

/* Legacy definitions for compatibility - these should be updated */
#define YT8512C_SPEED_STATUS                   YT8512C_PHYSCSR_SPEED_100M              /*!< Legacy: configured information of speed: 100Mbit/s */
#define YT8512C_DUPLEX_STATUS                  YT8512C_PHYSCSR_DUPLEX_FULL             /*!< Legacy: configured information of duplex: full-duplex */

/* RMII 配置示例 (EXT 4000h) */
#define EXT_4000_RMII_EN               ((uint16_t)0x0002)  /*!< bit1=1 Enable RMII */
#define EXT_4000_CLK_SEL               ((uint16_t)0x0001)  /*!< bit0=1 Input TXC/RXC */

/* 扩展寄存器访问 */
#define YT8512C_DBG_AOR                ((uint16_t)0x001E)  /*!< Debug Address Offset Register */
#define YT8512C_DBG_DR                 ((uint16_t)0x001F)  /*!< Debug Data Register */

#ifdef __cplusplus
}
#endif
#endif /* YT8512C_H */
