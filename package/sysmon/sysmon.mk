#########################################################################################
#
# sysmon kernel module & daemon
#
#########################################################################################

SYSMON_VERSION = 1.0
SYSMON_SITE = $(BR2_EXTERNAL_FINAL_PROJECT_SYSMON_PATH)/sysmon
SYSMON_SITE_METHOD = local

SYSMON_MODULE_SUBDIRS = kernel

define SYSMON_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)/daemon CC="$(TARGET_CC)"
endef

define SYSMON_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/daemon/sysmond $(TARGET_DIR)/usr/bin/sysmond
	$(INSTALL) -d -m 0755 $(TARGET_DIR)/var/lib/sysmon
endef

$(eval $(kernel-module))
$(eval $(generic-package))
