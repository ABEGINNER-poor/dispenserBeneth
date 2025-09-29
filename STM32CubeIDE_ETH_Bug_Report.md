# STM32CubeIDE ETH Configuration Bug Report

## Issue Summary
STM32CubeIDE fails to generate proper GPIO initialization code for Ethernet peripheral, resulting in non-functional MDIO/MDC communication with PHY chips.

## Environment
- **STM32CubeIDE Version**: 1.19.0
- **MCU**: STM32F407ZGT6 (LQFP144 package)
- **PHY**: YT8512C
- **Interface**: RMII mode
- **Date**: September 28, 2025

## Problem Description

### Configuration in .ioc File (Correct)
The STM32CubeIDE project configuration file (`.ioc`) shows correct pin assignments:
```
PC1.Mode=RMII
PC1.Signal=ETH_MDC
PA2.Mode=RMII  
PA2.Signal=ETH_MDIO
ETH.MediaInterface=HAL_ETH_RMII_MODE
```

### Generated Code Issue (Incorrect)
However, STM32CubeIDE **fails to generate the required `HAL_ETH_MspInit()` function** in `stm32f4xx_hal_msp.c`. This results in:

1. **Missing GPIO configuration**: PC1 and PA2 pins are not configured as alternate function AF11
2. **Default GPIO state**: Pins remain in their default configuration (typically AF0)
3. **Non-functional MDIO interface**: PHY register reads always return 0x0000
4. **Network connectivity failure**: Cannot establish Ethernet communication

### Symptoms Observed
- PHY register reads via `HAL_ETH_ReadPHYRegister()` always return 0x0000
- PHY not responding to MDIO commands
- Network interface fails to initialize properly
- Cable insertion/removal has no effect on link status

### Root Cause Analysis
Investigation revealed that the GPIO registers showed incorrect configuration:
- PC1 (MDC): Mode=0x0, AF=0x0 (should be Mode=0x2, AF=0xB)  
- PA2 (MDIO): Mode=0x0, AF=0x0 (should be Mode=0x2, AF=0xB)

The missing `HAL_ETH_MspInit()` function should contain:
```c
void HAL_ETH_MspInit(ETH_HandleTypeDef* heth)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(heth->Instance==ETH)
  {
    /* Peripheral clock enable */
    __HAL_RCC_ETH_CLK_ENABLE();
    __HAL_RCC_ETHMAC_CLK_ENABLE();
    __HAL_RCC_ETHMACTX_CLK_ENABLE();
    __HAL_RCC_ETHMACRX_CLK_ENABLE();
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /* ... other GPIO clocks ... */
    
    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA2     ------> ETH_MDIO
    /* ... other ETH pins ... */
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* ... */
  }
}
```

## Workaround Solution
Manual GPIO configuration in user code successfully resolves the issue:
```c
// Enable GPIO clocks
__HAL_RCC_GPIOA_CLK_ENABLE();
__HAL_RCC_GPIOC_CLK_ENABLE();

// Configure PA2 (MDIO) as AF11
GPIOA->MODER = (GPIOA->MODER & ~(0x3 << 4)) | (0x2 << 4);
GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xF << 8)) | (0xB << 8);

// Configure PC1 (MDC) as AF11  
GPIOC->MODER = (GPIOC->MODER & ~(0x3 << 2)) | (0x2 << 2);
GPIOC->AFR[0] = (GPIOC->AFR[0] & ~(0xF << 4)) | (0xB << 4);
```

After applying this workaround, PHY communication is immediately restored and network functionality works correctly.

## Impact Assessment
- **Severity**: High - Ethernet functionality completely non-functional
- **Scope**: Affects all STM32F4 projects using ETH peripheral with STM32CubeIDE
- **Workaround Available**: Yes, but requires manual intervention

## Expected Behavior
STM32CubeIDE should automatically generate the complete `HAL_ETH_MspInit()` function in `stm32f4xx_hal_msp.c` when ETH peripheral is enabled in the .ioc configuration, including proper GPIO alternate function configuration for all assigned ETH pins.

## Reproduction Steps
1. Create new STM32F407ZGT6 project in STM32CubeIDE 1.19.0
2. Enable ETH peripheral in RMII mode
3. Assign pins PC1→ETH_MDC, PA2→ETH_MDIO
4. Generate code
5. Observe missing `HAL_ETH_MspInit()` function in generated `stm32f4xx_hal_msp.c`
6. Compile and run - PHY communication will fail

## Request for Fix
Please investigate and fix the STM32CubeIDE code generator to ensure proper `HAL_ETH_MspInit()` function generation for ETH peripheral configurations.

---
**Reported by**: [Your Name]  
**Project**: Dispenser Control System  
**Contact**: [Your Contact Information]