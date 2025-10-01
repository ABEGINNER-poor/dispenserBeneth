/**
 ****************************************************************************************************
 * @file        ethernet_chip.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2022-08-01
 * @brief       PHYоƬ��������
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� ������ F429������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20220420
 * ��һ�η���
 *
 ****************************************************************************************************
 */
#include "ethernet_chip.h"


#define ETH_CHIP_SW_RESET_TO    ((uint32_t)500U)    /* ������λ�ȴ�ʱ�� */
#define ETH_CHIP_INIT_TO        ((uint32_t)2000U)   /* ��ʼ���ȴ�ʱ�� */
#define ETH_CHIP_MAX_DEV_ADDR   ((uint32_t)31U)     /* PHY��ַ�����ֵ */

#if PHY_AUTO_SELECT

#else
#define YT8512C_AND_RTL8201BL_PHYREGISTER2      0x0000
#define SR8201F_PHYREGISTER2                    0x001C
#define LAN8720A_PHYREGISTER2                   0x0007
int PHY_TYPE;
uint16_t ETH_CHIP_PHYSCSR;
uint16_t ETH_CHIP_SPEED_STATUS;
uint16_t ETH_CHIP_DUPLEX_STATUS;
#endif

/**
  * @brief       ��IO����ע�ᵽ�������
  * @param       pobj���豸����
  * @param       ioctx�������豸IO����
  * @retval      ETH_CHIP_STATUS_OK��OK
  *              ETH_CHIP_STATUS_ERROR��ȱ�ٹ���
  */
int32_t  eth_chip_regster_bus_io(eth_chip_object_t *pobj, eth_chip_ioc_tx_t *ioctx)
{
    if (!pobj || !ioctx->readreg || !ioctx->writereg || !ioctx->gettick)
    {
        return ETH_CHIP_STATUS_ERROR;
    }

    pobj->io.init = ioctx->init;
    pobj->io.deinit = ioctx->deinit;
    pobj->io.readreg = ioctx->readreg;
    pobj->io.writereg = ioctx->writereg;
    pobj->io.gettick = ioctx->gettick;

    return ETH_CHIP_STATUS_OK;
}

/**
  * @brief       ��ʼ��ETH_CHIP�����������Ӳ����Դ
  * @param       pobj: �豸����
  * @retval      ETH_CHIP_STATUS_OK����ʼ��ETH_CHIP�����������Ӳ����Դ�ɹ�
                 ETH_CHIP_STATUS_ADDRESS_ERROR���Ҳ����豸��ַ
                 ETH_CHIP_STATUS_READ_ERROR�����ܶ�ȡ�Ĵ���
                 ETH_CHIP_STATUS_WRITE_ERROR������д��Ĵ���
                 ETH_CHIP_STATUS_RESET_TIMEOUT���޷�ִ��������λ
  */
int32_t eth_chip_init(eth_chip_object_t *pobj)
{
    uint32_t tickstart = 0, regvalue = 0, addr = 0;
    int32_t status = ETH_CHIP_STATUS_OK;

#if PHY_AUTO_SELECT

#else
    /*  SR8201F     Register 2    0x001C
                    Register 3    0xC016

        YT8512C     Register 2    0x0000
                    Register 3    0x0128

        LAN8720A    Register 2    0x0007
                    Register 3    0xC0F0

        RTL8201BL   Register 2    0x0000
                    Register 3    0x8201 */
    pobj->io.readreg(addr, PHY_REGISTER2, &regvalue);

    switch (regvalue)
    {
        case YT8512C_AND_RTL8201BL_PHYREGISTER2:
            pobj->io.readreg(addr, PHY_REGISTER3, &regvalue);

            if (regvalue == 0x128)
            {
                ETH_CHIP_PHYSCSR  = ((uint16_t)0x11);
                ETH_CHIP_SPEED_STATUS = ((uint16_t)0x4010);
                ETH_CHIP_DUPLEX_STATUS = ((uint16_t)0x2000);
                PHY_TYPE = YT8512C;
            }
            else
            {
                ETH_CHIP_PHYSCSR  = ((uint16_t)0x10);
                ETH_CHIP_SPEED_STATUS = ((uint16_t)0x0022);
                ETH_CHIP_DUPLEX_STATUS = ((uint16_t)0x0004);
                PHY_TYPE = RTL8201;
            }
            break;
        case SR8201F_PHYREGISTER2:
            ETH_CHIP_PHYSCSR  = ((uint16_t)0x00);
            ETH_CHIP_SPEED_STATUS = ((uint16_t)0x2020);
            ETH_CHIP_DUPLEX_STATUS = ((uint16_t)0x0100);
            PHY_TYPE = SR8201F;
            break;
        case LAN8720A_PHYREGISTER2:
            ETH_CHIP_PHYSCSR  = ((uint16_t)0x1F);
            ETH_CHIP_SPEED_STATUS = ((uint16_t)0x0004);
            ETH_CHIP_DUPLEX_STATUS = ((uint16_t)0x0010);
            PHY_TYPE = LAN8720;
            break;
    }
#endif

    if (pobj->is_initialized == 0)
    {
        if (pobj->io.init != 0)
        {
            /* MDCʱ�� */
            pobj->io.init();
        }

        /* ����PHY��ַΪ32 */
        pobj->devaddr = ETH_CHIP_MAX_DEV_ADDR + 1;

        /* ��ҪΪ�˲���PHY��ַ */
        for (addr = 0; addr <= ETH_CHIP_MAX_DEV_ADDR; addr ++)
        {
            if (pobj->io.readreg(addr, ETH_CHIP_PHYSCSR, &regvalue) < 0)
            {
                status = ETH_CHIP_STATUS_READ_ERROR;
                /* �޷���ȡ����豸��ַ������һ����ַ */
                continue;
            }
            /* �Ѿ��ҵ�PHY��ַ�� */
            if ((regvalue & ETH_CHIP_PHY_COUNT) == addr)
            {
                pobj->devaddr = addr;
                status = ETH_CHIP_STATUS_OK;
                break;
            }
        }

        /* �ж����PHY��ַ�Ƿ����32��2^5��*/
        if (pobj->devaddr > ETH_CHIP_MAX_DEV_ADDR)
        {
            status = ETH_CHIP_STATUS_ADDRESS_ERROR;
        }

        /* ���PHY��ַ��Ч */
        if (status == ETH_CHIP_STATUS_OK)
        {
            /* ����������λ  */
            if (pobj->io.writereg(pobj->devaddr, ETH_CHIP_BCR, ETH_CHIP_BCR_SOFT_RESET) >= 0)
            {
                /* ��ȡ��������״̬ */
                if (pobj->io.readreg(pobj->devaddr, ETH_CHIP_BCR, &regvalue) >= 0)
                {
                    tickstart = pobj->io.gettick();

                    /* �ȴ�������λ��ɻ�ʱ  */
                    while (regvalue & ETH_CHIP_BCR_SOFT_RESET)
                    {
                        if ((pobj->io.gettick() - tickstart) <= ETH_CHIP_SW_RESET_TO)
                        {
                            if (pobj->io.readreg(pobj->devaddr, ETH_CHIP_BCR, &regvalue) < 0)
                            {
                                status = ETH_CHIP_STATUS_READ_ERROR;
                                break;
                            }
                        }
                        else
                        {
                            status = ETH_CHIP_STATUS_RESET_TIMEOUT;
                            break;
                        }
                    }
                }
                else
                {
                    status = ETH_CHIP_STATUS_READ_ERROR;
                }
            }
            else
            {
                status = ETH_CHIP_STATUS_WRITE_ERROR;
            }
        }
    }

    /* ���������ʼ����ɣ����� */
    if (status == ETH_CHIP_STATUS_OK)
    {
        tickstart =  pobj->io.gettick();

        /* �ȴ�2s���г�ʼ�� */
        while ((pobj->io.gettick() - tickstart) <= ETH_CHIP_INIT_TO)
        {
        }
        pobj->is_initialized = 1;
    }

    return status;
}

/**
  * @brief       ����ʼ��ETH_CHIP����Ӳ����Դ
  * @param       pobj: �豸����
  * @retval      ETH_CHIP_STATUS_OK������ʼ��ʧ�ܳɹ�
                 ETH_CHIP_STATUS_ERROR������ʼ��ʧ��
  */
int32_t eth_chip_deinit(eth_chip_object_t *pobj)
{
    if (pobj->is_initialized)
    {
        if (pobj->io.deinit != 0)
        {
            if (pobj->io.deinit() < 0)
            {
                return ETH_CHIP_STATUS_ERROR;
            }
        }

        pobj->is_initialized = 0;
    }

    return ETH_CHIP_STATUS_OK;
}

/**
  * @brief       �ر�ETH_CHIP���µ�ģʽ
  * @param       pobj: �豸����
  * @retval      ETH_CHIP_STATUS_OK���رճɹ�
                 ETH_CHIP_STATUS_READ_ERROR�����ܶ�ȡ�Ĵ���
                 ETH_CHIP_STATUS_WRITE_ERROR������д�Ĵ���
  */
int32_t eth_chip_disable_power_down_mode(eth_chip_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = ETH_CHIP_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, ETH_CHIP_BCR, &readval) >= 0)
    {
        readval &= ~ETH_CHIP_BCR_POWER_DOWN;

        /* ����µ�ģʽ */
        if (pobj->io.writereg(pobj->devaddr, ETH_CHIP_BCR, readval) < 0)
        {
            status =  ETH_CHIP_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = ETH_CHIP_STATUS_READ_ERROR;
    }

    return status;
}

/**
  * @brief       ʹ��ETH_CHIP���µ�ģʽ
  * @param       pobj: �豸����
  * @retval      ETH_CHIP_STATUS_OK���رճɹ�
                 ETH_CHIP_STATUS_READ_ERROR�����ܶ�ȡ�Ĵ���
                 ETH_CHIP_STATUS_WRITE_ERROR������д�Ĵ���
  */
int32_t eth_chip_enable_power_down_mode(eth_chip_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = ETH_CHIP_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, ETH_CHIP_BCR, &readval) >= 0)
    {
        readval |= ETH_CHIP_BCR_POWER_DOWN;

        /* ʹ���µ�ģʽ */
        if (pobj->io.writereg(pobj->devaddr, ETH_CHIP_BCR, readval) < 0)
        {
            status =  ETH_CHIP_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = ETH_CHIP_STATUS_READ_ERROR;
    }

    return status;
}

/**
  * @brief       �����Զ�Э�̹���
  * @param       pobj: �豸����
  * @retval      ETH_CHIP_STATUS_OK���رճɹ�
                 ETH_CHIP_STATUS_READ_ERROR�����ܶ�ȡ�Ĵ���
                 ETH_CHIP_STATUS_WRITE_ERROR������д�Ĵ���
  */
int32_t eth_chip_start_auto_nego(eth_chip_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = ETH_CHIP_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, ETH_CHIP_BCR, &readval) >= 0)
    {
        readval |= ETH_CHIP_BCR_AUTONEGO_EN;

        /* �����Զ�Э�� */
        if (pobj->io.writereg(pobj->devaddr, ETH_CHIP_BCR, readval) < 0)
        {
            status =  ETH_CHIP_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = ETH_CHIP_STATUS_READ_ERROR;
    }

    return status;
}

/**
  * @brief       ��ȡETH_CHIP�豸����·״̬
  * @param       pobj: �豸����
  * @param       pLinkState: ָ����·״̬��ָ��
  * @retval      ETH_CHIP_STATUS_100MBITS_FULLDUPLEX��100M��ȫ˫��
                 ETH_CHIP_STATUS_100MBITS_HALFDUPLEX ��100M����˫��
                 ETH_CHIP_STATUS_10MBITS_FULLDUPLEX��10M��ȫ˫��
                 ETH_CHIP_STATUS_10MBITS_HALFDUPLEX ��10M����˫��
                 ETH_CHIP_STATUS_READ_ERROR�����ܶ�ȡ�Ĵ���
  */
int32_t eth_chip_get_link_state(eth_chip_object_t *pobj)
{
    uint32_t readval = 0;

    /* ������⹦�ܼĴ�������ֵ */
    if (pobj->io.readreg(pobj->devaddr, ETH_CHIP_PHYSCSR, &readval) < 0)
    {
        return ETH_CHIP_STATUS_READ_ERROR;
    }

    if (((readval & ETH_CHIP_SPEED_STATUS) != ETH_CHIP_SPEED_STATUS) && ((readval & ETH_CHIP_DUPLEX_STATUS) != 0))
    {
        return ETH_CHIP_STATUS_100MBITS_FULLDUPLEX;
    }
    else if (((readval & ETH_CHIP_SPEED_STATUS) != ETH_CHIP_SPEED_STATUS))
    {
        return ETH_CHIP_STATUS_100MBITS_HALFDUPLEX;
    }
    else if (((readval & ETH_CHIP_BCR_DUPLEX_MODE) != ETH_CHIP_BCR_DUPLEX_MODE))
    {
        return ETH_CHIP_STATUS_10MBITS_FULLDUPLEX;
    }
    else
    {
        return ETH_CHIP_STATUS_10MBITS_HALFDUPLEX;
    }
}

/**
  * @brief       ����ETH_CHIP�豸����·״̬
  * @param       pobj: �豸����
  * @param       pLinkState: ָ����·״̬��ָ��
  * @retval      ETH_CHIP_STATUS_OK�����óɹ�
                 ETH_CHIP_STATUS_ERROR ������ʧ��
                 ETH_CHIP_STATUS_READ_ERROR�����ܶ�ȡ�Ĵ���
                 ETH_CHIP_STATUS_WRITE_ERROR ������д��Ĵ���
  */
int32_t eth_chip_set_link_state(eth_chip_object_t *pobj, uint32_t linkstate)
{
    uint32_t bcrvalue = 0;
    int32_t status = ETH_CHIP_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, ETH_CHIP_BCR, &bcrvalue) >= 0)
    {
        /* ������·����(�Զ�Э�̣��ٶȺ�˫��) */
        bcrvalue &= ~(ETH_CHIP_BCR_AUTONEGO_EN | ETH_CHIP_BCR_SPEED_SELECT | ETH_CHIP_BCR_DUPLEX_MODE);

        if (linkstate == ETH_CHIP_STATUS_100MBITS_FULLDUPLEX)
        {
            bcrvalue |= (ETH_CHIP_BCR_SPEED_SELECT | ETH_CHIP_BCR_DUPLEX_MODE);
        }
        else if (linkstate == ETH_CHIP_STATUS_100MBITS_HALFDUPLEX)
        {
            bcrvalue |= ETH_CHIP_BCR_SPEED_SELECT;
        }
        else if (linkstate == ETH_CHIP_STATUS_10MBITS_FULLDUPLEX)
        {
            bcrvalue |= ETH_CHIP_BCR_DUPLEX_MODE;
        }
        else
        {
            /* �������·״̬���� */
            status = ETH_CHIP_STATUS_ERROR;
        }
    }
    else
    {
        status = ETH_CHIP_STATUS_READ_ERROR;
    }

    if(status == ETH_CHIP_STATUS_OK)
    {
        /* д����·״̬ */
        if(pobj->io.writereg(pobj->devaddr, ETH_CHIP_BCR, bcrvalue) < 0)
        {
            status = ETH_CHIP_STATUS_WRITE_ERROR;
        }
    }

    return status;
}

/**
  * @brief       ���û���ģʽ
  * @param       pobj: �豸����
  * @param       pLinkState: ָ����·״̬��ָ��
  * @retval      ETH_CHIP_STATUS_OK�����óɹ�
                 ETH_CHIP_STATUS_READ_ERROR�����ܶ�ȡ�Ĵ���
                 ETH_CHIP_STATUS_WRITE_ERROR ������д��Ĵ���
  */
int32_t eth_chip_enable_loop_back_mode(eth_chip_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = ETH_CHIP_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, ETH_CHIP_BCR, &readval) >= 0)
    {
        readval |= ETH_CHIP_BCR_LOOPBACK;

        /* ���û���ģʽ */
        if (pobj->io.writereg(pobj->devaddr, ETH_CHIP_BCR, readval) < 0)
        {
            status = ETH_CHIP_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = ETH_CHIP_STATUS_READ_ERROR;
    }

    return status;
}

/**
  * @brief       ���û���ģʽ
  * @param       pobj: �豸����
  * @param       pLinkState: ָ����·״̬��ָ��
  * @retval      ETH_CHIP_STATUS_OK�����óɹ�
                 ETH_CHIP_STATUS_READ_ERROR�����ܶ�ȡ�Ĵ���
                 ETH_CHIP_STATUS_WRITE_ERROR ������д��Ĵ���
  */
int32_t eth_chip_disable_loop_back_mode(eth_chip_object_t *pobj)
{
    uint32_t readval = 0;
    int32_t status = ETH_CHIP_STATUS_OK;

    if (pobj->io.readreg(pobj->devaddr, ETH_CHIP_BCR, &readval) >= 0)
    {
        readval &= ~ETH_CHIP_BCR_LOOPBACK;

        /* ���û���ģʽ */
        if (pobj->io.writereg(pobj->devaddr, ETH_CHIP_BCR, readval) < 0)
        {
            status =  ETH_CHIP_STATUS_WRITE_ERROR;
        }
    }
    else
    {
        status = ETH_CHIP_STATUS_READ_ERROR;
    }

    return status;
}
