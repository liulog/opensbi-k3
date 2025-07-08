#
# SPDX-License-Identifier: BSD-2-Clause
#

carray-platform_override_modules-$(CONFIG_PLATFORM_SPACEMIT_K3) += spacemit_k3
platform-objs-$(CONFIG_PLATFORM_SPACEMIT_K3) += spacemit/spacemit_k3.o
firmware-its-$(CONFIG_PLATFORM_SPACEMIT_K3) += spacemit/fw_dynamic.its
