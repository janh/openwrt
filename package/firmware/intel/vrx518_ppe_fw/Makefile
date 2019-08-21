# Copyright (C) 2020 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=vrx518_ppe_fw
PKG_VERSION:=1.3.7
PKG_RELEASE:=$(AUTORELEASE)

UGW_VERSION=8.5.2.10
UGW_BASENAME=$(PKG_NAME)-ugw_$(UGW_VERSION)

PKG_SOURCE:=$(UGW_BASENAME).tar.bz2
PKG_BUILD_DIR:=$(BUILD_DIR)/$(UGW_BASENAME)
PKG_SOURCE_URL:=https://gitlab.com/prpl-foundation/intel/$(PKG_NAME)/-/archive/ugw_$(UGW_VERSION)/
PKG_HASH:=8c59af606138a1dc159976a7528b7e8ce35a7b0fa182f0297b61f1830ee4af6f

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=VRX518 PPE firmware
  URL:=http://www.intel.com
  DEPENDS:=@TARGET_ipq40xx
endef

define Package/$(PKG_NAME)/description
  VRX518 PPE firmware
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib/firmware

# ifeq ($(CONFIG_TARGET_lantiq_xrx330),y)
# 	$(INSTALL_BIN) $(PKG_BUILD_DIR)/platform/xrx330/ppe_fw.bin $(1)/lib/firmware/
# endif
# ifeq ($(CONFIG_TARGET_lantiq_xrx500),y)
# 	$(INSTALL_BIN) $(PKG_BUILD_DIR)/platform/xrx500/ppe_fw.bin $(1)/lib/firmware/
# endif
# ifeq ($(CONFIG_TARGET_intel_mips_xrx500),y)
# 	$(INSTALL_BIN) $(PKG_BUILD_DIR)/platform/xrx500/ppe_fw.bin $(1)/lib/firmware/
# endif
ifeq ($(CONFIG_TARGET_ipq40xx),y)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/platform/xrx500/ppe_fw.bin $(1)/lib/firmware/
endif
# ifeq ($(CONFIG_TARGET_x86_puma),y)
# 	$(INSTALL_BIN) $(PKG_BUILD_DIR)/platform/xrx750/ppe_fw.bin $(1)/lib/firmware/
# endif
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
