# wspace_thin_ap_unicast
# v1.0.0-1
#
# by Yijing Zeng

include $(TOPDIR)/rules.mk

PKG_NAME:=wspace_thin_ap_unicast
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/wspace_thin_ap_unicast
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=wspace_thin_ap_unicast -- Wspace Thin AP Unicast
  DEPENDS:=+libstdcpp +libpthread
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Build/Configure
endef

TARGET_CFLAGS += $(FPIC)

define Package/wspace_thin_ap_unicast/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wspace_thin_ap_unicast $(1)/bin/
endef

$(eval $(call BuildPackage,wspace_thin_ap_unicast))


