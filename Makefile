# Basalt M0 build (Limine Boot Protocol, ADR-002/005)
#
# Host: WSL2 Debian (wsl.exe -d Debian --). Tools: gcc/g++, make, xorriso,
# qemu-system-x86, python3. See README.md.

BUILD   := build
KERNEL  := $(BUILD)/basalt.elf
ISO     := $(BUILD)/basalt.iso
ISOROOT := $(BUILD)/iso_root

CC  := gcc
CXX := g++
LD  := ld

QEMU   := qemu-system-x86_64
LIMINE := third_party/limine/limine
FONT   := third_party/terminus/terminus_font.h

WARN     := -Wall -Wextra -Wno-unused-parameter
KCFLAGS  := -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector \
            -fno-asynchronous-unwind-tables -fno-plt \
            -mno-red-zone -mno-mmx -mno-sse -mno-80387 -mgeneral-regs-only \
            -mcmodel=kernel -Ithird_party/limine -Ithird_party/terminus \
            -Iarch/x86 -Ilib/kstd \
            $(WARN) -O2 -g -MMD -MP
CFLAGS   := $(KCFLAGS) -std=gnu11
CXXFLAGS := $(KCFLAGS) -std=gnu++17 -fno-exceptions -fno-rtti -fno-use-cxa-atexit
ASFLAGS  := $(KCFLAGS)

LDFLAGS := -nostdlib -z max-page-size=0x1000 -z noexecstack -T linker/x86_64.ld

OBJS := \
	$(BUILD)/requests.o \
	$(BUILD)/entry64.o \
	$(BUILD)/serial.o \
	$(BUILD)/fb.o \
	$(BUILD)/console.o \
	$(BUILD)/kmain.o \
	$(BUILD)/string.o

vpath %.c   boot/limine arch/x86/console lib/kstd
vpath %.cpp kernel kernel/console
vpath %.S   boot/x86

.PHONY: all iso run check font clean

all: $(KERNEL)

font: $(FONT)

$(FONT): third_party/terminus/ter-u16n.bdf tools/genfont.py
	python3 tools/genfont.py $< $@

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/fb.o: $(FONT)

$(KERNEL): $(OBJS) linker/x86_64.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "== linked: $@ =="

# Limine host tool (image installer), built from the vendored single source
$(LIMINE): third_party/limine/limine.c
	$(CC) -O2 -pipe -Wall -o $@ $<

iso: $(ISO)

$(ISO): $(KERNEL) $(LIMINE) limine.conf
	rm -rf $(ISOROOT)
	mkdir -p $(ISOROOT)/boot/limine
	cp $(KERNEL) $(ISOROOT)/boot/basalt.elf
	cp limine.conf $(ISOROOT)/boot/limine/limine.conf
	cp third_party/limine/limine-bios-cd.bin third_party/limine/limine-bios.sys \
	   third_party/limine/limine-uefi-cd.bin $(ISOROOT)/boot/limine/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
	    -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
	    -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
	    -efi-boot-part --efi-boot-image --protective-msdos-label \
	    $(ISOROOT) -o $@
	$(LIMINE) bios-install $@
	@echo "== iso: $@ =="

run: $(ISO)
	$(QEMU) -M pc -m 256M -cdrom $(ISO) -no-reboot -no-shutdown

# headless smoke test: pass if "hello world" appears on the COM1 log
check: $(ISO)
	@rm -f $(BUILD)/serial.log
	@timeout 15 $(QEMU) -M pc -m 256M -cdrom $(ISO) -display none \
	    -serial file:$(BUILD)/serial.log -no-reboot || true
	@grep -q "hello world" $(BUILD)/serial.log \
	    && echo "CHECK PASSED: hello world on COM1" \
	    || { echo "CHECK FAILED (see $(BUILD)/serial.log)"; exit 1; }

clean:
	rm -rf $(BUILD) $(FONT)

-include $(OBJS:.o=.d)
