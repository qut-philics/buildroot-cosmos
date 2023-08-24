# zrproc-firmware
ZRPROC_FIRMWARE_VERSION = 1.0
ZRPROC_FIRMWARE_SITE = $(BR2_EXTERNAL_QUT_PATH)/package/zrproc-firmware/src
ZRPROC_FIRMWARE_SITE_METHOD = local

ZRPROC_FIRMWARE_DEPENDENCIES = \
	$(BR2_MAKE_HOST_DEPENDENCY) \
	host-arm-gnu-toolchain
	
ZRPROC_FIRMWARE_PROJECTS_LIST = \
	pacpwm \
	phil-phaselock \
	template

ZRPROC_FIRMWARE_MAKE_OPTS = \
	CROSS_COMPILE=$(HOST_DIR)/bin/arm-none-eabi-

define ZRPROC_FIRMWARE_BUILD_CMDS
	$(foreach project, $(ZRPROC_FIRMWARE_TEST), \
		@echo $(project)
	)
	$(foreach project, $(ZRPROC_FIRMWARE_PROJECTS_LIST), \
		$(MAKE) $(ZRPROC_FIRMWARE_MAKE_OPTS) -C $(@D)/firmware/$(project)
	)
endef

define ZRPROC_FIRMWARE_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/lib/zrproc
	$(foreach project, $(ZRPROC_FIRMWARE_PROJECTS_LIST), \
		$(INSTALL) -D -m 4755 $(@D)/firmware/$(project)/$(project) $(TARGET_DIR)/usr/lib/zrproc/$(project).elf
	)
    
endef

$(eval $(generic-package))