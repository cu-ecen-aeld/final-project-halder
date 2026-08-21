obj-m += sysmon.o

# Build against the Linux kernel configured by Buildroot
KDIR := $(abspath ../buildroot/output/build/linux-custom)

# Raspberry Pi 4 64-bit
ARCH := arm64

# AArch64 cross-compiler
CROSS_COMPILE := $(abspath ../buildroot/output/host/bin/aarch64-linux-)

all:
	$(MAKE) -C $(KDIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(CURDIR) \
		modules

clean:
	$(MAKE) -C $(KDIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(CURDIR) \
		clean
