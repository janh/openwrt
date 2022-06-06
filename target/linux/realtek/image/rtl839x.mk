# SPDX-License-Identifier: GPL-2.0-only

define Device/hpe_1920-48g
  $(Device/hpe_1920)
  SOC := rtl8393
  DEVICE_MODEL := 1920-48G (JG927A)
  H3C_DEVICE_ID := 0x0001002a
endef
TARGET_DEVICES += hpe_1920-48g

define Device/zyxel_gs1900-48
  SOC := rtl8393
  IMAGE_SIZE := 13952k
  DEVICE_VENDOR := ZyXEL
  UIMAGE_MAGIC := 0x83800000
  ZYXEL_VERS := AAHO
  DEVICE_MODEL := GS1900-48
  KERNEL_INITRAMFS := kernel-bin | append-dtb | gzip | zyxel-vers $$$$(ZYXEL_VERS) | \
	uImage gzip
endef
TARGET_DEVICES += zyxel_gs1900-48
