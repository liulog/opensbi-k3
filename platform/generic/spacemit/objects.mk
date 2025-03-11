#
# SPDX-License-Identifier: BSD-2-Clause
#

carray-platform_override_modules-$(CONFIG_PLATFORM_SPACEMIT_K2) += spacemit_k2
platform-objs-$(CONFIG_PLATFORM_SPACEMIT_K2) += spacemit/spacemit_k2.o
firmware-its-$(CONFIG_PLATFORM_SPACEMIT_K2) += spacemit/fw_dynamic.its
