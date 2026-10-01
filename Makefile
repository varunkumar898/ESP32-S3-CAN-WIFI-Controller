# ============================================================
# ESP32-S3 Bare-Metal Project
# ============================================================

TARGET = esp32s3_controller

BUILD_DIR = build

CC = xtensa-esp32s3-elf-gcc
OBJCOPY = xtensa-esp32s3-elf-objcopy

CFLAGS = \
	-mlongcalls \
	-mtext-section-literals \
	-O2 \
	-ffreestanding \
	-fno-builtin \
	-Wall \
	-Wextra \
	-Iinc

LDFLAGS = \
	-Tlinker/esp32s3.ld \
	-nostdlib \
	-Wl,--gc-sections

C_SOURCES = \
	src/main.c \
	src/GPIO.c \
	src/TIMER.c \
	src/TWAI.c \
	src/UART.c \
	src/SPI.c \
	src/RELAY.c \
	src/DISPLAY.c \
	src/LOAD_CELL.c \
	src/SYSTEM.c

ASM_SOURCES = \
	src/startup.S

C_OBJECTS = $(C_SOURCES:src/%.c=$(BUILD_DIR)/%.o)

ASM_OBJECTS = $(ASM_SOURCES:src/%.S=$(BUILD_DIR)/%.o)

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

ELF = $(BUILD_DIR)/$(TARGET).elf
BIN = $(BUILD_DIR)/$(TARGET).bin


# ============================================================
# Default target
# ============================================================

all: $(BIN)


# ============================================================
# Create build directory
# ============================================================

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


# ============================================================
# Compile C files
# ============================================================

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


# ============================================================
# Compile assembly
# ============================================================

$(BUILD_DIR)/%.o: src/%.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


# ============================================================
# Link
# ============================================================

$(ELF): $(OBJECTS)
	$(CC) $(OBJECTS) \
		$(LDFLAGS) \
		-o $@


# ============================================================
# Generate binary
# ============================================================

$(BIN): $(ELF)
	esptool.py \
		--chip esp32s3 \
		elf2image \
		-o $@ \
		$<


# ============================================================
# Flash
# ============================================================

flash: $(BIN)
	esptool.py \
		--chip esp32s3 \
		--port COM7 \
		write_flash \
		0x0 \
		$(BIN)


# ============================================================
# Serial monitor
# ============================================================

monitor:
	python -m serial.tools.miniterm COM7 115200


# ============================================================
# Clean
# ============================================================

clean:
	rm -rf $(BUILD_DIR)


.PHONY: all flash monitor clean