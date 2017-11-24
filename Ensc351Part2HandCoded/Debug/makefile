################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Ensc351Part2HandCoded

dependents:
	-cd /media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351Part2SolnLib/Debug && $(MAKE) all

# Tool invocations
Ensc351Part2HandCoded: $(OBJS) $(USER_OBJS) /media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351Part2SolnLib/Debug/libEnsc351Part2SolnLib.a
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L"/media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351x-myio/Debug" -L"/media/sf_U_ensc251/ensc351/git/ensc351lib/Ensc351/Debug" -L"/media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351xmodem/Debug" -L"/media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351Part2SolnLib/Debug" -o "Ensc351Part2HandCoded" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS)$(CXX_DEPS)$(OBJS)$(CPP_DEPS)$(C_DEPS) Ensc351Part2HandCoded
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/media/sf_U_ensc251/workspace-cpp-Neon3/Ensc351Part2SolnLib/Debug/libEnsc351Part2SolnLib.a:

-include ../makefile.targets
