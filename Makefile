BUILDROOT_DIR=buildroot
BUILDROOT_CONFIG=$(BUILDROOT_DIR)/.config

# Final project is the Buildroot external tree
EXTERNAL_DIR=.
EXTERNAL_REL_BUILDROOT=..

DEFCONFIG=configs/raspberrypi4-64_sysmon_defconfig
DEFCONFIG_REL_BUILDROOT=../$(DEFCONFIG)


.PHONY: all config build menuconfig save-defconfig clean

# Default target
all: build


# Configure Buildroot from the project's saved defconfig
config:
	$(MAKE) -C $(BUILDROOT_DIR) defconfig \
		BR2_EXTERNAL=$(EXTERNAL_REL_BUILDROOT) \
		BR2_DEFCONFIG=$(DEFCONFIG_REL_BUILDROOT)


# Build the Buildroot image
build:
ifeq (,$(wildcard $(BUILDROOT_CONFIG)))
	@echo "MISSING BUILDROOT CONFIGURATION FILE"
	@echo "Run 'make config' first."
	@exit 1
endif
	$(MAKE) -C $(BUILDROOT_DIR) BR2_EXTERNAL=$(EXTERNAL_REL_BUILDROOT)


# Open Buildroot menuconfig
menuconfig:
	$(MAKE) -C $(BUILDROOT_DIR) menuconfig


# Save current Buildroot configuration
save-defconfig:
	$(MAKE) -C $(BUILDROOT_DIR) savedefconfig \
		BR2_DEFCONFIG=$(DEFCONFIG_REL_BUILDROOT)


# Clean the Buildroot build
clean:
	$(MAKE) -C $(BUILDROOT_DIR) distclean

