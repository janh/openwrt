# Copyright (C) 2012 OpenWrt.org
# Copyright (C) 2015-2016 Lantiq Beteiligungs GmbH & Co KG.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ltq-vdsl-vr9-mei
PKG_VERSION:=1.11.1
PKG_RELEASE:=$(AUTORELEASE)

PKG_BASE_NAME:=drv_mei_cpe
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://gitlab.com/prpl-foundation/intel/dsl_cpe_mei.git
PKG_SOURCE_DATE:=2021-09-30
PKG_SOURCE_VERSION:=73d0147a0e51216053d64eb747cc1a55581393ef
PKG_MIRROR_HASH:=31d9d0140e973d1b9f51846550854f1faaca537b52a6d86c50c473441e7a5c5f
PKG_SOURCE_SUBDIR:=$(PKG_BASE_NAME)-$(PKG_VERSION)
PKG_FIXUP:=autoreconf
PKG_FLAGS:=nonshared
PKG_MAINTAINER:=John Crispin <john@phrozen.org>
PKG_LICENSE:=GPL-2.0 BSD-2-Clause
PKG_LICENSE_FILES:=LICENSE
PKG_EXTMOD_SUBDIRS:=src

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ltq-vdsl-vr9-mei
  TITLE:=mei driver for vdsl
  SECTION:=sys
  SUBMENU:=Network Devices
  DEPENDS:=@TARGET_lantiq_xrx200 +kmod-ltq-ifxos
  FILES:=$(PKG_BUILD_DIR)/src/drv_mei_cpe.ko
  AUTOLOAD:=$(call AutoLoad,50,drv_mei_cpe)
endef

define KernelPackage/ltq-vdsl-vr9-mei/description
	Lantiq MEI CPE Kernel Module Driver
endef


define Package/ltq-vdsl-mei-test
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Lantiq mei driver test tool
  URL:=http://www.lantiq.com/
  DEPENDS:=@TARGET_lantiq_xrx200
endef

define Package/ltq-vdsl-mei-test/description
	Userland tool to directly control the mei driver, this is only needed
	for test and development purposes.
endef

MAKE_FLAGS += \
	$(KERNEL_MAKE_FLAGS) \
	SHELL="$(BASH)"

# ltq-vdsl-app uses a header provided by the MEI driver which has some
# conditionals.
# Define the conditionals here to have the same view on both sides. If you
# change them, you need to change them for the ltq-vdsl-app as well
MEI_DRV_CFLAGS = \
	-DMEI_DRV_ATM_PTM_INTERFACE_ENABLE=1 \
	-DMEI_SUPPORT_DEBUG_STREAMS=1 \
	-DMEI_SUPPORT_OPTIMIZED_FW_DL=1

CONFIGURE_ARGS += \
	--enable-kernelincl="$(LINUX_DIR)/include" \
	--enable-device=vr9 \
	--enable-debug=no \
	--enable-error-print=no \
	--enable-ifxos-include="-I$(STAGING_DIR)/usr/include/ifxos/" \
	--enable-ifxos-library="-L$(STAGING_DIR)/usr/lib" \
	--enable-add_drv_cflags="$(MEI_DRV_CFLAGS)" \
	--enable-linux-26 \
	--enable-kernelbuild="$(LINUX_DIR)" \
	--enable-drv_test_appl=yes \
	--disable-ppa-callbacks-tc \
	ARCH=$(LINUX_KARCH)

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/vdsl
	$(CP) $(PKG_BUILD_DIR)/src/drv_mei_cpe_api_intern.h $(1)/usr/include/vdsl/
	$(CP) $(PKG_BUILD_DIR)/src/drv_mei_cpe_api_atm_ptm_intern.h $(1)/usr/include/vdsl/
	$(CP) $(PKG_BUILD_DIR)/src/drv_mei_cpe_interface.h $(1)/usr/include/vdsl
	$(CP) $(PKG_BUILD_DIR)/src/drv_mei_cpe_config.h $(1)/usr/include/vdsl/
	$(CP) $(PKG_BUILD_DIR)/src/cmv_message_format.h $(1)/usr/include/vdsl/
endef

$(eval $(call KernelPackage,ltq-vdsl-vr9-mei))

define Package/ltq-vdsl-mei-test/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/mei_cpe_drv_test $(1)/bin
endef

$(eval $(call BuildPackage,ltq-vdsl-mei-test))
