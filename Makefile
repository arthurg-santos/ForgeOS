# Toolchain
CC = gcc
CXX = g++
ASM = nasm
LD = gcc

# Flags de compilação (freestanding + sem XMM/x87 no código gerado)
CFLAGS = -ffreestanding -fno-pie -fno-pic -mno-red-zone -mcmodel=kernel -nostdlib -fno-builtin -Wall -Wextra -O2 -g -mno-mmx -mno-sse -mno-sse2 -mno-3dnow -mno-80387
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASMFLAGS = -f elf64
LDFLAGS = -ffreestanding -nostdlib -lgcc -no-pie -T linker/linker.ld

# Diretórios e Arquivos
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/iso
KERNEL_ELF = $(BUILD_DIR)/forgeos.elf
ISO_NAME = $(BUILD_DIR)/forgeos.iso

CXX_SOURCES = kernel/main.cpp kernel/io.cpp kernel/irq.cpp kernel/scheduler.cpp \
              arch/x86_64/gdt.cpp arch/x86_64/idt.cpp arch/x86_64/pic.cpp \
              arch/x86_64/serial.cpp memory/pmm.cpp memory/vmm.cpp memory/kheap.cpp \
              drivers/timer.cpp
ASM_SOURCES = boot/multiboot2_header.asm boot/entry.asm \
              arch/x86_64/interrupts.asm arch/x86_64/cpu_asm.asm \
              arch/x86_64/context.asm

CXX_OBJECTS = $(CXX_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
ASM_OBJECTS = $(ASM_SOURCES:%.asm=$(BUILD_DIR)/%.o)
OBJECTS = $(CXX_OBJECTS) $(ASM_OBJECTS)

# Caminhos de headers
INCLUDES = -Iinclude -Iarch/x86_64 -Imemory -Ikernel -Idrivers

.PHONY: all clean run debug dirs

all: dirs $(ISO_NAME)

dirs:
	@mkdir -p $(BUILD_DIR)/boot
	@mkdir -p $(BUILD_DIR)/kernel
	@mkdir -p $(BUILD_DIR)/arch/x86_64
	@mkdir -p $(BUILD_DIR)/memory
	@mkdir -p $(BUILD_DIR)/drivers
	@mkdir -p $(ISO_DIR)/boot/grub

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDES)

$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

$(KERNEL_ELF): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

$(ISO_NAME): $(KERNEL_ELF)
	@cp $(KERNEL_ELF) $(ISO_DIR)/boot/forgeos.elf
	@cp scripts/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@grub2-mkrescue -o $@ $(ISO_DIR) --modules="multiboot2" 2>/dev/null || grub-mkrescue -o $@ $(ISO_DIR) --modules="multiboot2" 2>/dev/null

run: all
	@qemu-system-x86_64 -cdrom $(ISO_NAME) -m 128M -vga std -serial stdio -no-reboot -no-shutdown -d int,cpu_reset -D $(BUILD_DIR)/qemu.log

debug: all
	@qemu-system-x86_64 -cdrom $(ISO_NAME) -m 128M -vga std -serial stdio -no-reboot -no-shutdown -s -S &
	@gdb -ex "target remote :1234" -ex "symbol-file $(KERNEL_ELF)"

clean:
	@rm -rf $(BUILD_DIR)
