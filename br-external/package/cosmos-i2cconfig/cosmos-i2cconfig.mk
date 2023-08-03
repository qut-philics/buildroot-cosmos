# cosmos-i2cconfig
COSMOS_I2CCONFIG_VERSION = 1.0
COSMOS_I2CCONFIG_SITE = $(BR2_EXTERNAL_QUT_PATH)/package/cosmos-i2cconfig/src
COSMOS_I2CCONFIG_SITE_METHOD = local

define COSMOS_I2CCONFIG_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define COSMOS_I2CCONFIG_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/cosmos-i2cconfig $(TARGET_DIR)/usr/bin
	$(INSTALL) -D -m 0755 $(@D)/S99cosmos-i2cconfig $(TARGET_DIR)/etc/init.d
endef

$(eval $(generic-package))