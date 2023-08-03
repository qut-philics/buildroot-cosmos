################################################################################
#
# bitstream-timestamp
#
################################################################################

BITSTREAM_TIMESTAMP_VERSION = 1.0
BITSTREAM_TIMESTAMP_SITE = $(BR2_EXTERNAL_QUT_PATH)/package/bitstream-timestamp/src
BITSTREAM_TIMESTAMP_SITE_METHOD = local

define BITSTREAM_TIMESTAMP_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define BITSTREAM_TIMESTAMP_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 4755 $(@D)/bitstream-timestamp $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))