# Copyright (C) 2019 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=dsl_vr11_firmware_xdsl
PKG_VERSION:=8.13.1.5.0.7
PKG_RELEASE:=$(AUTORELEASE)

UGW_VERSION=8.5.2
UGW_BASENAME=$(PKG_NAME)-ugw-$(UGW_VERSION)

PKG_SOURCE:=$(UGW_BASENAME).tar.bz2
PKG_BUILD_DIR:=$(BUILD_DIR)/$(UGW_BASENAME)
PKG_SOURCE_URL:=https://gitlab.com/prpl-foundation/intel/$(PKG_NAME)/-/archive/ugw-$(UGW_VERSION)/
PKG_HASH:=27aac54a0ec1c1059cd2de125d29f116af998711e69cd96b27b17f174b698dbb

PKG_LICENSE:=Intel-Confidential-Information-10.24.2016
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk

ANNEX_A_VER:=8D1507_8D0901

define Package/dsl-vr11-firmware-xdsl-a
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=VRX518 / VR11 CPE xDSL Annex A firmware
  URL:=http://www.intel.com
  DEPENDS:=@TARGET_ipq40xx
endef

define Package/dsl-vr11-firmware-xdsl-a/description
  VRX518 / VR11 CPE VDSL and ADSL Annex A firmware
endef

define Build/Compile
endef

define Package/dsl-vr11-firmware-xdsl-a/install
	$(INSTALL_DIR) $(1)/lib/firmware/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/xcpe_$(ANNEX_A_VER).bin $(1)/lib/firmware/
	ln -s xcpe_$(ANNEX_A_VER).bin $(1)/lib/firmware/vdsl.bin
endef

$(eval $(call BuildPackage,dsl-vr11-firmware-xdsl-a))
