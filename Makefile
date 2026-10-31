TOOLCHAIN ?= /Volumes/disk1t/gcc/gcc-arm-none-eabi-10.3-2021.10/bin
PREFIX     = $(TOOLCHAIN)/arm-none-eabi-
CC         = $(PREFIX)gcc
AS         = $(PREFIX)gcc
OBJCOPY    = $(PREFIX)objcopy
SIZE       = $(PREFIX)size

TARGET  = OV_Watch
BUILD   = build

MCU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

C_SOURCES = $(shell grep -v "portable/RVDS" files.txt 2>/dev/null || true)
C_SOURCES += Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c

C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/core/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/draw/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/draw/sw/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/extra/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/extra/layouts/flex/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/extra/layouts/grid/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/extra/libs/*/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/extra/others/*/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/extra/themes/*/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/extra/widgets/*/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/font/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/hal/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/misc/*.c)
C_SOURCES += $(wildcard Middlewares/LVGL/GUI/lvgl/src/widgets/*.c)

ASM_SOURCES = Drivers/CMSIS/Device/ST/STM32WBxx/Source/Templates/gcc/startup_stm32wb55xx_cm4.s

C_INCLUDES = \
-ICore/Inc \
-IDrivers/STM32WBxx_HAL_Driver/Inc \
-IDrivers/STM32WBxx_HAL_Driver/Inc/Legacy \
-IDrivers/CMSIS/Device/ST/STM32WBxx/Include \
-IDrivers/CMSIS/Include \
-IMiddlewares/Third_Party/FreeRTOS/Source/include \
-IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 \
-IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F \
-ISYSTEM \
-IBSP/LCD \
-IBSP/AHT21 \
-IBSP/BL24C02 \
-IBSP/IIC \
-IBSP/LSM303DLH \
-IBSP/MPU6050 \
-IBSP/MPU6050/eMPL \
-IBSP/POWER \
-IBSP/CW2015 \
-IBSP/SPL06_001 \
-IBSP/EM7028 \
-IBSP/TOUCH \
-IBSP/KEY \
-IBSP/KT6328 \
-IUser \
-IUser/Tasks/Inc \
-IUser/Func/Inc \
-IUser/GUI_App \
-IUser/GUI_App/Screens/Inc \
-IUser/GUI_App/Fonts \
-IUser/GUI_App/IMGs \
-IMiddlewares/LVGL/GUI \
-IMiddlewares/LVGL/GUI/lvgl \
-IMiddlewares/LVGL/GUI/porting

C_DEFS = -DUSE_HAL_DRIVER -DSTM32WB55xx \
-DtraceISR_ENTER\(\)= -DtraceISR_EXIT_TO_SCHEDULER\(\)= -DtraceISR_EXIT\(\)=

OPT = -Os -g3 -Wall

LDSCRIPT = stm32wb55xx_flash_cm4.ld

LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) \
-Wl,-Map=$(BUILD)/$(TARGET).map,--cref -Wl,--gc-sections

OBJECTS  = $(addprefix $(BUILD)/, $(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD)/, $(notdir $(ASM_SOURCES:.s=.o)))

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).hex

$(BUILD)/%.o: %.c Makefile | $(BUILD)
	$(CC) -c $(C_DEFS) $(C_INCLUDES) $(OPT) $(MCU) -MMD -MP -MF $(BUILD)/$*.d -o $@ $<

$(BUILD)/%.o: %.s Makefile | $(BUILD)
	$(CC) -c $(C_DEFS) $(C_INCLUDES) $(OPT) $(MCU) -o $@ $<

$(BUILD)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/%.hex: $(BUILD)/%.elf | $(BUILD)
	$(OBJCOPY) -O ihex $< $@

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

.PHONY: all clean

-include $(wildcard $(BUILD)/*.d)