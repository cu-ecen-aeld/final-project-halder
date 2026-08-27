#########################################################################################
#
# sysmon
#
#########################################################################################

SYSMON_VERSION = 1.0
SYSMON_SITE = $(BR2_EXTERNAL_FINAL_PROJECT_SYSMON_PATH)/sysmon
SYSMON_SITE_METHOD = local

$(eval $(kernel-module))
$(eval $(generic-package))
