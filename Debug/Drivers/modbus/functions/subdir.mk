################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/modbus/functions/mbfunccoils.c \
../Drivers/modbus/functions/mbfuncdiag.c \
../Drivers/modbus/functions/mbfuncdisc.c \
../Drivers/modbus/functions/mbfuncfile.c \
../Drivers/modbus/functions/mbfuncholding.c \
../Drivers/modbus/functions/mbfuncinput.c \
../Drivers/modbus/functions/mbfuncother.c \
../Drivers/modbus/functions/mbutils.c 

OBJS += \
./Drivers/modbus/functions/mbfunccoils.o \
./Drivers/modbus/functions/mbfuncdiag.o \
./Drivers/modbus/functions/mbfuncdisc.o \
./Drivers/modbus/functions/mbfuncfile.o \
./Drivers/modbus/functions/mbfuncholding.o \
./Drivers/modbus/functions/mbfuncinput.o \
./Drivers/modbus/functions/mbfuncother.o \
./Drivers/modbus/functions/mbutils.o 

C_DEPS += \
./Drivers/modbus/functions/mbfunccoils.d \
./Drivers/modbus/functions/mbfuncdiag.d \
./Drivers/modbus/functions/mbfuncdisc.d \
./Drivers/modbus/functions/mbfuncfile.d \
./Drivers/modbus/functions/mbfuncholding.d \
./Drivers/modbus/functions/mbfuncinput.d \
./Drivers/modbus/functions/mbfuncother.d \
./Drivers/modbus/functions/mbutils.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/modbus/functions/%.o Drivers/modbus/functions/%.su Drivers/modbus/functions/%.cyclo: ../Drivers/modbus/functions/%.c Drivers/modbus/functions/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Drivers/BSP/Components/lan8742 -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -I"C:/Users/13016/STM32CubeIDE/workspace_1.19.0/dispenserBeneth/Drivers/modbus/port" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Drivers/modbus/functions/mbfuncdisc.o: ../Drivers/modbus/functions/mbfuncdisc.c Drivers/modbus/functions/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Drivers/BSP/Components/lan8742 -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -I"C:/Users/13016/STM32CubeIDE/workspace_1.19.0/dispenserBeneth/Drivers/modbus/port" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-modbus-2f-functions

clean-Drivers-2f-modbus-2f-functions:
	-$(RM) ./Drivers/modbus/functions/mbfunccoils.cyclo ./Drivers/modbus/functions/mbfunccoils.d ./Drivers/modbus/functions/mbfunccoils.o ./Drivers/modbus/functions/mbfunccoils.su ./Drivers/modbus/functions/mbfuncdiag.cyclo ./Drivers/modbus/functions/mbfuncdiag.d ./Drivers/modbus/functions/mbfuncdiag.o ./Drivers/modbus/functions/mbfuncdiag.su ./Drivers/modbus/functions/mbfuncdisc.cyclo ./Drivers/modbus/functions/mbfuncdisc.d ./Drivers/modbus/functions/mbfuncdisc.o ./Drivers/modbus/functions/mbfuncdisc.su ./Drivers/modbus/functions/mbfuncfile.cyclo ./Drivers/modbus/functions/mbfuncfile.d ./Drivers/modbus/functions/mbfuncfile.o ./Drivers/modbus/functions/mbfuncfile.su ./Drivers/modbus/functions/mbfuncholding.cyclo ./Drivers/modbus/functions/mbfuncholding.d ./Drivers/modbus/functions/mbfuncholding.o ./Drivers/modbus/functions/mbfuncholding.su ./Drivers/modbus/functions/mbfuncinput.cyclo ./Drivers/modbus/functions/mbfuncinput.d ./Drivers/modbus/functions/mbfuncinput.o ./Drivers/modbus/functions/mbfuncinput.su ./Drivers/modbus/functions/mbfuncother.cyclo ./Drivers/modbus/functions/mbfuncother.d ./Drivers/modbus/functions/mbfuncother.o ./Drivers/modbus/functions/mbfuncother.su ./Drivers/modbus/functions/mbutils.cyclo ./Drivers/modbus/functions/mbutils.d ./Drivers/modbus/functions/mbutils.o ./Drivers/modbus/functions/mbutils.su

.PHONY: clean-Drivers-2f-modbus-2f-functions

