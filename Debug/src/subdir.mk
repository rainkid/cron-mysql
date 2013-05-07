################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/base64.c \
../src/config.c \
../src/list.c \
../src/mail.c \
../src/main.c \
../src/util.c 

OBJS += \
./src/base64.o \
./src/config.o \
./src/list.o \
./src/mail.o \
./src/main.o \
./src/util.o 

C_DEPS += \
./src/base64.d \
./src/config.d \
./src/list.d \
./src/mail.d \
./src/main.d \
./src/util.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


