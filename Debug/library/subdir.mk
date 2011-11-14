################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../library/base64.c \
../library/send_mail.c \
../library/task.c \
../library/tool.c 

OBJS += \
./library/base64.o \
./library/send_mail.o \
./library/task.o \
./library/tool.o 

C_DEPS += \
./library/base64.d \
./library/send_mail.d \
./library/task.d \
./library/tool.d 


# Each subdirectory must supply rules for building sources it contributes
library/%.o: ../library/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


