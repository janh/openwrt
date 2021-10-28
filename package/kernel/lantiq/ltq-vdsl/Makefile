# Copyright (C) 2012 OpenWrt.org
# Copyright (C) 2015-2016 Lantiq Beteiligungs GmbH & Co KG.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ltq-vdsl-vr9
PKG_VERSION:=4.23.1
PKG_RELEASE:=$(AUTORELEASE)

PKG_BASE_NAME:=drv_dsl_cpe_api
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://gitlab.com/prpl-foundation/intel/dsl_cpe_api.git
PKG_SOURCE_DATE:=2021-09-30
PKG_SOURCE_VERSION:=b044661e111730928e9da35d4680eebd5500d490
PKG_MIRROR_HASH:=ace62fd1cbff218a9448ba7d0daf834738d6d77ef02f2e9730429948e518e973
PKG_SOURCE_SUBDIR:=$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_LICENSE:=GPL-2.0 BSD-2-Clause
PKG_LICENSE_FILES:=LICENSE

PKG_FIXUP:=autoreconf

PKG_MAINTAINER:=John Crispin <john@phrozen.org>

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ltq-vdsl-vr9
  TITLE:=vdsl driver
  SECTION:=sys
  SUBMENU:=Network Devices
  DEPENDS:=@TARGET_lantiq_xrx200 +kmod-ltq-vdsl-vr9-mei
  FILES:=$(PKG_BUILD_DIR)/src/drv_dsl_cpe_api.ko
  AUTOLOAD:=$(call AutoLoad,51,drv_dsl_cpe_api)
endef

define Package/ltq-vdsl-vr9/description
	This package contains the Lantiq DSL CPE API driver.

	Supported Devices:
		- VRX200 Family
endef

MAKE_FLAGS += \
	$(KERNEL_MAKE_FLAGS) \
	SHELL="$(BASH)"

CONFIGURE_ARGS += --enable-kernel-include="$(LINUX_DIR)/include" \
	--enable-vrx \
	--enable-vrx-device=vr9 \
	--enable-firmware-r9 \
	--enable-dsl-filter-detection=no \
	--enable-ifxos \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos" \
	--enable-driver-include="-I$(STAGING_DIR)/usr/include/vdsl" \
	--enable-add-drv-cflags="-DDSL_DRV_ATM_PTM_INTERFACE_ENABLE=1" \
	--enable-linux-26 \
	--enable-kernelbuild="$(LINUX_DIR)" \
	--enable-debug-prints=no \
	ARCH=mips

CONFIGURE_ARGS += \
	--enable-model=full \
	--enable-dsl-ceoc=no
#CONFIGURE_ARGS += --enable-model=lite
#CONFIGURE_ARGS += --enable-model=footprint
#CONFIGURE_ARGS += --enable-model=typical
#CONFIGURE_ARGS += --enable-model=debug

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/drv_vdsl_cpe_api
	$(CP) $(PKG_BUILD_DIR)/src/include/drv_dsl_cpe*.h $(1)/usr/include/drv_vdsl_cpe_api/
endef

$(eval $(call KernelPackage,ltq-vdsl-vr9))
