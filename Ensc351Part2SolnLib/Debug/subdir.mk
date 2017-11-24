################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../main.cc 

CPP_SRCS += \
../Medium.cpp 

CC_DEPS += \
./main.d 

OBJS += \
./Medium.o \
./main.o 

CPP_DEPS += \
./Medium.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351x-myio" -I"/media/sf_U_ensc251/ensc351/git/ensc351lib/Ensc351" -I"/media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351xmodem" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351x-myio" -I"/media/sf_U_ensc251/ensc351/git/ensc351lib/Ensc351" -I"/media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351xmodem" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


