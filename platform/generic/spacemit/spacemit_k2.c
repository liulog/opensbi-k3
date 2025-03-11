/*
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libfdt.h>
#include <platform_override.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_console.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/riscv_atomic.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_hsm.h>
#include <sbi/sbi_ipi.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_timer.h>
#include <sbi_utils/fdt/fdt_helper.h>

#include <spacemit/spacemit_config.h>

#define PLATFORM_MAX_CPUS_PER_CLUSTER 4

#define SYSREG_REG(offset)	(0x10012000 + (offset))

#define SYS_CPU_RST		SYSREG_REG(0x10)

#define CPU_TO_CLUSTER(cpu)	((cpu) / PLATFORM_MAX_CPUS_PER_CLUSTER)

static int spacemit_k2_hart_start(uint32_t hartid, unsigned long saddr)
{
	uint32_t cluster;
	uint32_t reset = __raw_readl((void *)SYS_CPU_RST);
	uint32_t _reset;

	if (hartid == 0U)
		return 0;

	cluster = CPU_TO_CLUSTER(hartid);

	reset |= 1U << (hartid - 1U + ((cluster > 0 ? cluster : 1) - 1));

	if (cluster != 0U)
		reset |= (1U << (2U + cluster * 5));

	_reset = reset & ~(1U << (hartid - 1U + ((cluster > 0 ? cluster : 1) - 1)));

	__raw_writel(_reset, (void *)SYS_CPU_RST);
	sbi_timer_mdelay(1);
	__raw_writel(reset, (void *)SYS_CPU_RST);

	return 0;
}

static int spacemit_k2_hart_stop(void)
{
	return 0;
}

static int spacemit_k2_hart_suspend(u32 suspend_type, ulong mmode_resume_addr)
{
	return 0;
}

static void spacemit_k2_hart_resume(void)
{
}

static const struct sbi_hsm_device spacemit_k2_hsm_ops = {
	.name		= "spacemit_k2-hsm",
	.hart_start	= spacemit_k2_hart_start,
	.hart_stop	= spacemit_k2_hart_stop,
	.hart_suspend	= spacemit_k2_hart_suspend,
	.hart_resume	= spacemit_k2_hart_resume,
};

static const struct fdt_match spacemit_k2_mach[] = {
	{ .compatible = "spacemit,spacemit_k2" },
	{ .compatible = "riscv-spacemit" },
	{ },
};

static int spacemit_k2_early_init(bool cold_boot, const void *fdt, const struct fdt_match *match)
{
	return 0;
}

static int spacemit_k2_final_init(bool cold_boot, void *fdt, const struct fdt_match *match)
{
	if (cold_boot) {
		sbi_hsm_set_device(&spacemit_k2_hsm_ops);
	}

	return 0;
}

static bool spacemit_k2_cold_boot_allowed(u32 hartid, const struct fdt_match *match)
{
	/* enable core snoop */
	csr_set(CSR_ML2SETUP, 1 << (hartid % PLATFORM_MAX_CPUS_PER_CLUSTER));

	/* dealing with resuming process */
	if ((__sbi_hsm_hart_get_state(hartid) == SBI_HSM_STATE_SUSPENDED) && (hartid == 0))
		return false;

	return ((hartid == 0) ? true : false);
}

const struct platform_override spacemit_k2 = {
	.match_table = spacemit_k2_mach,
	.early_init = spacemit_k2_early_init,
	.final_init = spacemit_k2_final_init,
	.cold_boot_allowed = spacemit_k2_cold_boot_allowed,
};
