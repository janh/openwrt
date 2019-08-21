# Copyright (C) 2020 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=vrx518_aca_fw
PKG_VERSION:=1.5.0
PKG_RELEASE:=$(AUTORELEASE)

UGW_VERSION=8.5.2.10
UGW_BASENAME=$(PKG_NAME)-ugw_$(UGW_VERSION)

PKG_SOURCE:=$(UGW_BASENAME).tar.bz2
PKG_BUILD_DIR:=$(BUILD_DIR)/$(UGW_BASENAME)
PKG_SOURCE_URL:=https://gitlab.com/prpl-foundation/intel/$(PKG_NAME)/-/archive/ugw_$(UGW_VERSION)/
PKG_HASH:=daab560f80369516f2c1cf4385843e82004ffe96c0711b1d8faec9972af7c8fa

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=VRX518 ACA firmware
  URL:=http://www.intel.com
  DEPENDS:=@TARGET_ipq40xx
endef

define Package/$(PKG_NAME)/description
  VRX518 ACA firmware
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib/firmware/09a9
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/platform/xrx500/aca_fw.bin $(1)/lib/firmware/09a9
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
