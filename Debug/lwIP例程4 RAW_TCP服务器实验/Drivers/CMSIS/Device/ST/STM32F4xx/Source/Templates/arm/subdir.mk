################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../lwIP例程4\ RAW_TCP服务器实验/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/arm/startup_stm32f407xx.s 

OBJS += \
./lwIP例程4\ RAW_TCP服务器实验/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/arm/startup_stm32f407xx.o 

S_DEPS += \
./lwIP例程4\ RAW_TCP服务器实验/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/arm/startup_stm32f407xx.d 


# Each subdirectory must supply rules for building sources it contributes
lwIP例程4\ RAW_TCP服务器实验/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/arm/startup_stm32f407xx.o: ../lwIP例程4\ RAW_TCP服务器实验/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/arm/startup_stm32f407xx.s lwIP例程4\ RAW_TCP服务器实验/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/arm/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Drivers/BSP/Components/lan8742 -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -x assembler-with-cpp -MMD -MP -MF"lwIP例程4 RAW_TCP服务器实验/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/arm/startup_stm32f407xx.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-lwIP-4f8b--7a0b-4-20-RAW_TCP-670d--52a1--5668--5b9e--9a8c--2f-Drivers-2f-CMSIS-2f-Device-2f-ST-2f-STM32F4xx-2f-Source-2f-Templates-2f-arm

clean-lwIP-4f8b--7a0b-4-20-RAW_TCP-670d--52a1--5668--5b9e--9a8c--2f-Drivers-2f-CMSIS-2f-Device-2f-ST-2f-STM32F4xx-2f-Source-2f-Templates-2f-arm:
	-$(RM) ./lwIP例程4\ RAW_TCP服务器实验/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/arm/startup_stm32f407xx.d ./lwIP例程4\ RAW_TCP服务器实验/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/arm/startup_stm32f407xx.o

.PHONY: clean-lwIP-4f8b--7a0b-4-20-RAW_TCP-670d--52a1--5668--5b9e--9a8c--2f-Drivers-2f-CMSIS-2f-Device-2f-ST-2f-STM32F4xx-2f-Source-2f-Templates-2f-arm

