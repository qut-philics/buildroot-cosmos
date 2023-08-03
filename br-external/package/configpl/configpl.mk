################################################################################
#
# configpl
#
################################################################################

CONFIGPL_VERSION = 1.0
CONFIGPL_SITE = $(BR2_EXTERNAL_QUT_PATH)/package/configpl/src
CONFIGPL_SITE_METHOD = local

define CONFIGPL_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define CONFIGPL_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 4755 $(@D)/configpl $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))