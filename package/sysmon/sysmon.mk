#########################################################################################
#
# sysmon
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
	$(INSTALL) -d -m 0755 $(TARGET_DIR)/usr/share/sysmon/dashboard/templates
	$(INSTALL) -m 0755 $(BR2_EXTERNAL_FINAL_PROJECT_SYSMON_PATH)/dashboard/app.py \
		$(TARGET_DIR)/usr/share/sysmon/dashboard/app.py
	$(INSTALL) -m 0644 $(BR2_EXTERNAL_FINAL_PROJECT_SYSMON_PATH)/dashboard/templates/index.html \
		$(TARGET_DIR)/usr/share/sysmon/dashboard/templates/index.html
endef

$(eval $(kernel-module))
$(eval $(generic-package))
