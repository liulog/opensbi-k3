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
#include <spacemit/k2/k2_pmp.h>

#define SYSREG_REG(offset)	(0x10012000 + (offset))

#define SYS_CPU_RST		SYSREG_REG(0x10)

#define CPU_TO_CLUSTER(cpu)	((cpu) / PLATFORM_MAX_CPUS_PER_CLUSTER)

void spacemit_k2_pmp_init(void)
{
	static int pmp_inited = 0;
	int i, region_num;
	PmpRegion* pRegion;

	if (pmp_inited)
		return;
	uint8_t pmpxcfg = 0;
	uint64_t addr = 0, base_addr;
	uint64_t region_size;
	mpu_region_attr_t attr = { 0 };
	int32_t region_size_id;

	region_num = sizeof(pmp_setting) / sizeof(PmpRegion);
	if (region_num > PMP_ENTRY_NUM) {
		sbi_printf("region number exceed max!\n");
		return;
	}

	/* setup pmp for each region */
	for (i = 0; i < region_num; i++) {
		pmpxcfg = 0;
		pRegion = &pmp_setting[i];
		base_addr = pRegion->start_addr;
		region_size = pRegion->end_addr - base_addr + 1;
		region_size_id = get_region_size_id(region_size);
		if (!pRegion->region_enable)
			attr.a = 0;
		if (region_size_id == REGION_SIZE_4B) {
			addr = base_addr >> 2;
			attr.a = 2;
		} else if (region_size_id != INVALID_REGION_SIZE_ID) { // use NAPOT
			attr.a = 3;
			addr = ((base_addr >> 2) & (0xFFFFFFFFFFU - ((1 << (region_size_id + 1)) - 1))) | (((uint64_t)1 << region_size_id) - 1);
		} else { // use TOR
			attr.a = 1;
			addr = (pRegion->end_addr + 1) >> 2;
			if (i > 0 && pRegion->end_addr < pmp_setting[i - 1].end_addr) {
				sbi_printf("Opps!If use TOR please make sure region address are increased!\n");
				sbi_printf("Hang here unless fix me!\n");
				while (1)
					;
			}
		}
		__set_PMPADDRx(i, addr);

		attr.l = 1; // always lock which means also take effect for machine mode!!!
		switch (pRegion->region_attr) {
		case RWX:
			attr.r = 1;
			attr.w = 1;
			attr.x = 1;
			break;
		case RWnX:
			attr.r = 1;
			attr.w = 1;
			attr.x = 0;
			break;
		case ROX:
			attr.r = 1;
			attr.w = 0;
			attr.x = 1;
			break;
		case ROnX:
			attr.r = 1;
			attr.w = 0;
			attr.x = 0;
			break;
		case nRnWnX:
			attr.r = 0;
			attr.w = 0;
			attr.x = 0;
			break;
		default:
			sbi_printf("Opps! Unsupported PMP attribute!\n");
			while (1)
				;
			break;
		}
		pmpxcfg |= (attr.r << PMP_PMPCFG_R_Pos) | (attr.w << PMP_PMPCFG_W_Pos) | (attr.x << PMP_PMPCFG_X_Pos) | (attr.a << PMP_PMPCFG_A_Pos) | (attr.l << PMP_PMPCFG_L_Pos);
		__set_PMPxCFG(i, pmpxcfg);
	}
	pmp_inited = 1;
}

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

	/* Initialize PMP only on CPU0 */
	if (hartid == 0) {
		spacemit_k2_pmp_init();
	}

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
