################################################################################
#
# pacpwm-config
#
################################################################################

PACPWM_CONFIG_VERSION = 1.0
PACPWM_CONFIG_SITE = $(BR2_EXTERNAL_QUT_PATH)/package/pacpwm-config/src
PACPWM_CONFIG_SITE_METHOD = local

define PACPWM_CONFIG_BUILD_CMDS
	cd $(PACPWM_CONFIG_SRCDIR) && \
	$(TARGET_MAKE_ENV) \
		$(TARGET_CONFIGURE_OPTS) \
		$(PKG_CARGO_ENV) \
		$(PACPWM_CONFIG_CARGO_ENV) \
		cargo build \
			--release \
			--manifest-path Cargo.toml \
			$(PACPWM_CONFIG_CARGO_BUILD_OPTS)
endef

define PACPWM_CONFIG_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 4755 $(@D)/target/$(RUSTC_TARGET_NAME)/release/pacpwm-config $(TARGET_DIR)/usr/bin
endef

$(eval $(cargo-package))