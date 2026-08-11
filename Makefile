CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

TARGET    = firmware
BUILD_DIR = build

CPU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS = $(CPU) \
         -DSTM32G431xx \
         -Wall \
         -O0 \
         -g3 \
         -ffunction-sections \
         -fdata-sections \
         -ICMSIS/Core \
         -ICMSIS/Device/ST/STM32G4xx/Include

LDFLAGS = $(CPU) \
          -Tlinker/STM32G431RBTX_FLASH.ld \
          -Wl,--gc-sections \
          -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

SRC_C = \
    Core/main.c \
    Core/system_stm32g4xx.c

SRC_S = \
    startup/startup_stm32g431xx.s

OBJ = \
    $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_C)) \
    $(patsubst %.s,$(BUILD_DIR)/%.o,$(SRC_S))

ELF = $(BUILD_DIR)/$(TARGET).elf
BIN = $(BUILD_DIR)/$(TARGET).bin

all: $(ELF) $(BIN)

$(ELF): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CPU) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
