include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ltq-vectoring
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ltq-vectoring
  SECTION:=sys
  CATEGORY:=Kernel modules
  SUBMENU:=Network Devices
  TITLE:=driver for sending vectoring error samples with L2 encapsulation
  DEPENDS:=@TARGET_lantiq_xrx200
  FILES:=$(PKG_BUILD_DIR)/ltq_vectoring.ko
  AUTOLOAD:=$(call AutoLoad,49,ltq_vectoring)
endef

define Build/Configure
endef

define Build/Compile
	cd $(LINUX_DIR); \
		ARCH=mips CROSS_COMPILE="$(KERNEL_CROSS)" \
		$(MAKE) $(KERNEL_MAKE_FLAGS) M=$(PKG_BUILD_DIR) V=1 modules
endef

$(eval $(call KernelPackage,ltq-vectoring))
