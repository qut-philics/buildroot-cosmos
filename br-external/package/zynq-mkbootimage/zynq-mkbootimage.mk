################################################################################
#
# zynq-mkbootimage
#
################################################################################

ZYNQ_MKBOOTIMAGE_VERSION = 872363ce32c249f8278cf107bc6d3bdeb38d849f
ZYNQ_MKBOOTIMAGE_SITE = $(call github,antmicro,zynq-mkbootimage,$(ZYNQ_MKBOOTIMAGE_VERSION))
HOST_ZYNQ_MKBOOTIMAGE_DEPENDENCIES = host-elfutils
ZYNQ_MKBOOTIMAGE_LICENSE = BSD-2-Clause
ZYNQ_MKBOOTIMAGE_LICENSE_FILES = LICENSE

define HOST_ZYNQ_MKBOOTIMAGE_BUILD_CMDS
    $(MAKE) -C $(@D) CC="$(HOSTCC)" CFLAGS="-I$(HOST_DIR)/include" LDFLAGS="$(HOST_LDFLAGS)"
endef

define HOST_ZYNQ_MKBOOTIMAGE_INSTALL_CMDS
	$(INSTALL) -m 0755 -D $(@D)/mkbootimage $(HOST_DIR)/bin/mkbootimage
	$(INSTALL) -m 0755 -D $(@D)/exbootimage $(HOST_DIR)/bin/exbootimage
endef

$(eval $(host-generic-package))