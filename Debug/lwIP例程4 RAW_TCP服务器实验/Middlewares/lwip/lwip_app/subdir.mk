################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.c 

OBJS += \
./lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.o 

C_DEPS += \
./lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.d 


# Each subdirectory must supply rules for building sources it contributes
lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.o: ../lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.c lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I"C:/Users/13016/STM32CubeIDE/workspace_1.19.0/dispenserBeneth/modbus/include" -I"C:/Users/13016/STM32CubeIDE/workspace_1.19.0/dispenserBeneth/Drivers/BSP/Components/yt8512c" -I"C:/Users/13016/STM32CubeIDE/workspace_1.19.0/dispenserBeneth/modbus/port" -I"C:/Users/13016/STM32CubeIDE/workspace_1.19.0/dispenserBeneth/modbus/rtu" -I"C:/Users/13016/STM32CubeIDE/workspace_1.19.0/dispenserBeneth/modbus/tcp" -I../Core/Inc -I"C:/Users/13016/STM32CubeIDE/workspace_1.19.0/dispenserBeneth/modbus/ascii" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"lwIP例程4 RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.d" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-lwIP-4f8b--7a0b-4-20-RAW_TCP-670d--52a1--5668--5b9e--9a8c--2f-Middlewares-2f-lwip-2f-lwip_app

clean-lwIP-4f8b--7a0b-4-20-RAW_TCP-670d--52a1--5668--5b9e--9a8c--2f-Middlewares-2f-lwip-2f-lwip_app:
	-$(RM) ./lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.cyclo ./lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.d ./lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.o ./lwIP例程4\ RAW_TCP服务器实验/Middlewares/lwip/lwip_app/lwip_demo.su

.PHONY: clean-lwIP-4f8b--7a0b-4-20-RAW_TCP-670d--52a1--5668--5b9e--9a8c--2f-Middlewares-2f-lwip-2f-lwip_app

