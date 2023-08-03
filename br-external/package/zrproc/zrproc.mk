# zrproc
ZRPROC_VERSION = 1.0
ZRPROC_SITE = $(BR2_EXTERNAL_QUT_PATH)/package/zrproc/src
ZRPROC_SITE_METHOD = local

define ZRPROC_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define ZRPROC_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 4755 $(@D)/zrproc $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))