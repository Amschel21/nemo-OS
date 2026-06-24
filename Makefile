CXX := g++
AS  := as
LD  := ld

CXXFLAGS := \
-ffreestanding \
-fno-exceptions \
-fno-rtti \
-m32 \
-std=c++20 \
-Wall \
-Wextra

CPP_SOURCES := $(shell find src -name "*.cpp")
CPP_OBJECTS := $(CPP_SOURCES:.cpp=.o)

ASM_SOURCES := $(shell find src -name "*.s")
ASM_OBJECTS := $(ASM_SOURCES:.s=.o)

OBJECTS := $(CPP_OBJECTS) $(ASM_OBJECTS)

all: kernel.iso

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.s
	$(AS) --32 $< -o $@

kernel.bin: $(OBJECTS)
	$(LD) -m elf_i386 \
	-T linker.ld \
	$(OBJECTS) \
	-o kernel.bin

kernel.iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o kernel.iso iso

run: kernel.iso
	qemu-system-i386 \
	-cdrom kernel.iso \
	-m 512M

debug: kernel.iso
	qemu-system-i386 \
	-cdrom kernel.iso \
	-s \
	-S

clean:
	find src -name "*.o" -delete
	rm -f kernel.bin
	rm -f kernel.iso
	rm -rf iso/boot

rebuild: clean all

.PHONY: all run debug clean rebuild