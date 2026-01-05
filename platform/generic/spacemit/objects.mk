#
# SPDX-License-Identifier: BSD-2-Clause
#

carray-platform_override_modules-$(CONFIG_PLATFORM_SPACEMIT_K3) += spacemit_k3
platform-objs-$(CONFIG_PLATFORM_SPACEMIT_K3) += spacemit/spacemit_k3.o spacemit/k3_corepm.o

ifeq ($(CONFIG_FIT_SIGNATURE),)
firmware-its-$(CONFIG_PLATFORM_SPACEMIT_K3) += spacemit/fw_dynamic.its
else
firmware-its-$(CONFIG_PLATFORM_SPACEMIT_K3) += spacemit/fw_dynamic_sign.its
itb_build_key_para := -k $(platform_src_dir)/spacemit/key
endif
