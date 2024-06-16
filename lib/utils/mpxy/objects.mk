#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2024 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_MPXY) += mpxy/fdt_mpxy.o
libsbiutils-objs-$(CONFIG_FDT_MPXY) += mpxy/fdt_mpxy_drivers.carray.o

libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_MBOX) += mpxy/fdt_mpxy_rpmi_mbox.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_CLOCK) += fdt_mpxy_rpmi_clock
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_CLOCK) += mpxy/fdt_mpxy_rpmi_clock.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_VOLTAGE) += fdt_mpxy_rpmi_voltage
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_VOLTAGE) += mpxy/fdt_mpxy_rpmi_voltage.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_DOMAIN) += fdt_mpxy_rpmi_domain
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_DOMAIN) += mpxy/fdt_mpxy_rpmi_domain.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_RTC) += fdt_mpxy_rpmi_rtc
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_RTC) += mpxy/fdt_mpxy_rpmi_rtc.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_PWRKEY) += fdt_mpxy_rpmi_pwrkey
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_PWRKEY) += mpxy/fdt_mpxy_rpmi_pwrkey.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_RPMI_SYSMSI) += fdt_mpxy_rpmi_sysmsi
libsbiutils-objs-$(CONFIG_FDT_MPXY_RPMI_SYSMSI) += mpxy/fdt_mpxy_rpmi_sysmsi.o

carray-fdt_mpxy_drivers-$(CONFIG_FDT_MPXY_OPTEED) += fdt_mpxy_opteed
libsbiutils-objs-$(CONFIG_FDT_MPXY_OPTEED) += mpxy/fdt_mpxy_opteed.o
