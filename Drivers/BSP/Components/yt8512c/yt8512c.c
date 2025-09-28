/* Includes ------------------------------------------------------------------*/
#include "yt8512c.h"


#define YT8512C_SW_RESET_TO    ((uint32_t)500U)    /* 软件复位等待时间 */
#define YT8512C_INIT_TO        ((uint32_t)2000U)   /* 初始化等待时间 */
#define YT8512C_MAX_DEV_ADDR   ((uint32_t)31U)     /* PHY地址的最大值 */

#define YT8512C_AND_RTL8201BL_PHYREGISTER2      0x0000

/**
  * @brief       将IO函数注册到组件对象
  * @param       pobj：设备对象
  * @param       ioctx：保存设备IO功能
  * @retval      YT8512C_STATUS_OK：OK
  *              YT8512C_STATUS_ERROR：缺少功能
  */
int32_t  yt8512c_regster_bus_io(yt8512c_object_t *pobj, yt8512c_ioc_tx_t *ioctx)
{
    if (!pobj || !ioctx->readreg || !ioctx->writereg || !ioctx->gettick)
    {
        return YT8512C_STATUS_ERROR;
    }

    pobj->io.init = ioctx->init;
    pobj->io.deinit = ioctx->deinit;
    pobj->io.readreg = ioctx->readreg;
    pobj->io.writereg = ioctx->writereg;
    pobj->io.gettick = ioctx->gettick;

    return YT8512C_STATUS_OK;
}


/**
  * @brief       初始化YT8512C并配置所需的硬件资源
  * @param       pobj: 设备对象
  * @retval      YT8512C_STATUS_OK：初始化YT8512C并配置所需的硬件资源成功
                 YT8512C_STATUS_ADDRESS_ERROR：找不到设备地址
                 YT8512C_STATUS_READ_ERROR：不能读取寄存器
                 YT8512C_STATUS_WRITE_ERROR：不能写入寄存器
                 YT8512C_STATUS_RESET_TIMEOUT：无法执行软件复位
  */
int32_t yt8512c_init(yt8512c_object_t *pobj)
{
    uint32_t tickstart = 0, regvalue = 0, addr = 0;
    int32_t status = YT8512C_STATUS_OK;

    if (pobj->is_initialized == 0)
    {
        if (pobj->io.init != 0)
        {
            pobj->io.init();
        }

        pobj->devaddr = YT8512C_MAX_DEV_ADDR + 1;

        for (addr = 0; addr <= YT8512C_MAX_DEV_ADDR; addr++)
        {
            if (pobj->io.readreg(addr, YT8512C_PHYSCSR, &regvalue) < 0)
            {
                status = YT8512C_STATUS_READ_ERROR;
                continue;
            }
            if ((regvalue & YT8512C_PHY_COUNT) == addr)
            {
                pobj->devaddr = addr;
                status = YT8512C_STATUS_OK;
                break;
            }
        }

        if (pobj->devaddr > YT8512C_MAX_DEV_ADDR)
        {
            status = YT8512C_STATUS_ADDRESS_ERROR;
        }

        if (status == YT8512C_STATUS_OK)
        {
            if (pobj->io.writereg(pobj->devaddr, YT8512C_BCR, YT8512C_BCR_SOFT_RESET) >= 0)
            {
                if (pobj->io.readreg(pobj->devaddr, YT8512C_BCR, &regvalue) >= 0)
                {
                    tickstart = pobj->io.gettick();
                    while (regvalue & YT8512C_BCR_SOFT_RESET)
                    {
                        if ((pobj->io.gettick() - tickstart) <= 2000)
                        {
                            if (pobj->io.readreg(pobj->devaddr, YT8512C_BCR, &regvalue) < 0)
                            {
                                status = YT8512C_STATUS_READ_ERROR;
                                break;
                            }
                        }
                        else
                        {
                            status = YT8512C_STATUS_RESET_TIMEOUT;
                            break;
                        }
                    }
                }
                else
                {
                    status = YT8512C_STATUS_READ_ERROR;
                }
            }
            else
            {
                status = YT8512C_STATUS_WRITE_ERROR;
            }
        }
    }

    if (status == YT8512C_STATUS_OK)
    {
        tickstart = pobj->io.gettick();
        while ((pobj->io.gettick() - tickstart) <= 2000)
        {
        }
        pobj->is_initialized = 1;
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "YT8512C Init OK, Addr: 0x%02X\r\n", pobj->devaddr);
        CDC_Transmit_FS((uint8_t *)buffer, strlen(buffer));
    }
    else
    {
        CDC_Transmit_FS((uint8_t *)"YT8512C Init Failed\r\n", 20);
    }
    return status;
}



/**
  * @brief       反初始化YT8512C及其硬件资源
  * @param       pobj: 设备对象
  * @retval      YT8512C_STATUS_OK：反初始化失败成功
                 YT8512C_STATUS_ERROR：反初始化失败
  */
int32_t yt8512c_deinit(yt8512c_object_t *pobj)
{
    if (pobj->is_initialized)
    {
        if (pobj->io.deinit != 0)
        {
            if (pobj->io.deinit() < 0)
            {
                return YT8512C_STATUS_ERROR;
            }
        }

        pobj->is_initialized = 0;
    }

    return YT8512C_STATUS_OK;
}


/**
  * @brief       关闭YT8512C的下电模式
  * @param       pobj: 设备对象
  * @retval      YT8512C_STATUS_OK：关闭成功
                 YT8512C_STATUS_READ_ERROR：不能读取寄存器
                 YT8512C_STATUS_WRITE_ERROR：不能写寄存器
  */
int32_t yt8512c_disable_power_down_mode(yt8512c_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = YT8512C_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, YT8512C_BCR, &readval) >= 0)
    {
        readval &= ~YT8512C_BCR_POWER_DOWN;

        /* 清除下电模式 */
        if (pobj->io.writereg(pobj->devaddr, YT8512C_BCR, readval) < 0)
        {
            status =  YT8512C_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = YT8512C_STATUS_READ_ERROR;
    }

    return status;
}



/**
  * @brief       使能YT8512C的下电模式
  * @param       pobj: 设备对象
  * @retval      YT8512C_STATUS_OK：关闭成功
                 YT8512C_STATUS_READ_ERROR：不能读取寄存器
                 YT8512C_STATUS_WRITE_ERROR：不能写寄存器
  */
int32_t yt8512c_enable_power_down_mode(yt8512c_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = YT8512C_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, YT8512C_BCR, &readval) >= 0)
    {
        readval |= YT8512C_BCR_POWER_DOWN;

        /* 使能下电模式 */
        if (pobj->io.writereg(pobj->devaddr, YT8512C_BCR, readval) < 0)
        {
            status =  YT8512C_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = YT8512C_STATUS_READ_ERROR;
    }

    return status;
}



/**
  * @brief       启动自动协商过程
  * @param       pobj: 设备对象
  * @retval      YT8512C_STATUS_OK：关闭成功
                 YT8512C_STATUS_READ_ERROR：不能读取寄存器
                 YT8512C_STATUS_WRITE_ERROR：不能写寄存器
  */
int32_t yt8512c_start_auto_nego(yt8512c_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = YT8512C_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, YT8512C_BCR, &readval) >= 0)
    {
        readval |= YT8512C_BCR_AUTONEGO_EN;

        /* 启动自动协商 */
        if (pobj->io.writereg(pobj->devaddr, YT8512C_BCR, readval) < 0)
        {
            status =  YT8512C_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = YT8512C_STATUS_READ_ERROR;
    }

    return status;
}



/**
  * @brief       获取YT8512C设备的链路状态
  * @param       pobj: 设备对象
  * @param       pLinkState: 指向链路状态的指针
  * @retval      YT8512C_STATUS_100MBITS_FULLDUPLEX：100M，全双工
                 YT8512C_STATUS_100MBITS_HALFDUPLEX ：100M，半双工
                 YT8512C_STATUS_10MBITS_FULLDUPLEX：10M，全双工
                 YT8512C_STATUS_10MBITS_HALFDUPLEX ：10M，半双工
                 YT8512C_STATUS_READ_ERROR：不能读取寄存器
  */
int32_t yt8512c_get_link_state(yt8512c_object_t *pobj)
{
    uint32_t bsr_value = 0;
    if (pobj->io.readreg(pobj->devaddr, YT8512C_BSR, &bsr_value) < 0)
    {
        CDC_Transmit_FS((uint8_t *)"BSR Read Error\r\n", 14);
        return YT8512C_STATUS_READ_ERROR;
    }
    if (!(bsr_value & YT8512C_BSR_LINK_STATUS))
    {
        return YT8512C_STATUS_LINK_DOWN;
    }

    uint32_t phycsr_value = 0;
    if (pobj->io.readreg(pobj->devaddr, YT8512C_PHYSCSR, &phycsr_value) < 0)
    {
        CDC_Transmit_FS((uint8_t *)"PHYCSR Read Error\r\n", 18);
        return YT8512C_STATUS_READ_ERROR;
    }
    if ((phycsr_value & YT8512C_SPEED_STATUS) && (phycsr_value & YT8512C_DUPLEX_STATUS))
    {
        return YT8512C_STATUS_100MBITS_FULLDUPLEX;
    }
    else if (phycsr_value & YT8512C_SPEED_STATUS)
    {
        return YT8512C_STATUS_100MBITS_HALFDUPLEX;
    }
    else if (phycsr_value & YT8512C_BCR_DUPLEX_MODE)
    {
        return YT8512C_STATUS_10MBITS_FULLDUPLEX;
    }
    else
    {
        return YT8512C_STATUS_10MBITS_HALFDUPLEX;
    }
}


/**
  * @brief       设置YT8512C设备的链路状态
  * @param       pobj: 设备对象
  * @param       pLinkState: 指向链路状态的指针
  * @retval      YT8512C_STATUS_OK：设置成功
                 YT8512C_STATUS_ERROR ：设置失败
                 YT8512C_STATUS_READ_ERROR：不能读取寄存器
                 YT8512C_STATUS_WRITE_ERROR ：不能写入寄存器
  */
int32_t yt8512c_set_link_state(yt8512c_object_t *pobj, uint32_t linkstate)
{
    uint32_t bcrvalue = 0;
    int32_t status = YT8512C_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, YT8512C_BCR, &bcrvalue) >= 0)
    {
        /* 禁用链路配置(自动协商，速度和双工) */
        bcrvalue &= ~(YT8512C_BCR_AUTONEGO_EN | YT8512C_BCR_SPEED_SELECT | YT8512C_BCR_DUPLEX_MODE);

        if (linkstate == YT8512C_STATUS_100MBITS_FULLDUPLEX)
        {
            bcrvalue |= (YT8512C_BCR_SPEED_SELECT | YT8512C_BCR_DUPLEX_MODE);
        }
        else if (linkstate == YT8512C_STATUS_100MBITS_HALFDUPLEX)
        {
            bcrvalue |= YT8512C_BCR_SPEED_SELECT;
        }
        else if (linkstate == YT8512C_STATUS_10MBITS_FULLDUPLEX)
        {
            bcrvalue |= YT8512C_BCR_DUPLEX_MODE;
        }
        else
        {
            /* 错误的链路状态参数 */
            status = YT8512C_STATUS_ERROR;
        }
    }
    else
    {
        status = YT8512C_STATUS_READ_ERROR;
    }

    if(status == YT8512C_STATUS_OK)
    {
        /* 写入链路状态 */
        if(pobj->io.writereg(pobj->devaddr, YT8512C_BCR, bcrvalue) < 0)
        {
            status = YT8512C_STATUS_WRITE_ERROR;
        }
    }

    return status;
}

/**
  * @brief       启用环回模式
  * @param       pobj: 设备对象
  * @param       pLinkState: 指向链路状态的指针
  * @retval      YT8512C_STATUS_OK：设置成功
                 YT8512C_STATUS_READ_ERROR：不能读取寄存器
                 YT8512C_STATUS_WRITE_ERROR ：不能写入寄存器
  */
int32_t yt8512c_enable_loop_back_mode(yt8512c_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = YT8512C_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, YT8512C_BCR, &readval) >= 0)
    {
        readval |= YT8512C_BCR_LOOPBACK;

        /* 启用环回模式 */
        if (pobj->io.writereg(pobj->devaddr, YT8512C_BCR, readval) < 0)
        {
            status = YT8512C_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = YT8512C_STATUS_READ_ERROR;
    }

    return status;
}

/**
  * @brief       禁用环回模式
  * @param       pobj: 设备对象
  * @param       pLinkState: 指向链路状态的指针
  * @retval      YT8512C_STATUS_OK：设置成功
                 YT8512C_STATUS_READ_ERROR：不能读取寄存器
                 YT8512C_STATUS_WRITE_ERROR ：不能写入寄存器
  */
int32_t yt8512c_disable_loop_back_mode(yt8512c_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = YT8512C_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, YT8512C_BCR, &readval) >= 0)
    {
        readval &= ~YT8512C_BCR_LOOPBACK;

        /* 禁用环回模式 */
        if (pobj->io.writereg(pobj->devaddr, YT8512C_BCR, readval) < 0)
        {
            status =  YT8512C_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = YT8512C_STATUS_READ_ERROR;
    }

    return status;
}


