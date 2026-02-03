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
#include <spacemit/k3/k3_pmp.h>
#include <sbi_utils/cache/cache.h>
#include <sbi_utils/cci/cci.h>

void spacemit_k3_pmp_init(void)
{
	int i, region_num;
	PmpRegion* pRegion;

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
			addr = ((base_addr >> 2) & (0xFFFFFFFFFFU - ((1 << (region_size_id + 1)) - 1))) |
				(((uint64_t)1 << region_size_id) - 1);
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
		pmpxcfg |= (attr.r << PMP_PMPCFG_R_Pos) | (attr.w << PMP_PMPCFG_W_Pos) |
			(attr.x << PMP_PMPCFG_X_Pos) | (attr.a << PMP_PMPCFG_A_Pos) | (attr.l << PMP_PMPCFG_L_Pos);
		__set_PMPxCFG(i, pmpxcfg);
	}
}

static const struct fdt_match spacemit_k3_mach[] = {
	{ .compatible = "spacemit,k3" },
	{ .compatible = "riscv-spacemit" },
	{ },
};

PLAT_CCI_MAP;
extern struct sbi_platform platform;

#define CPU_TO_CLUSTER(cpu)    ((cpu) / PLATFORM_MAX_CPUS_PER_CLUSTER)

static int spacemit_k3_early_init(bool cold_boot, const void *fdt, const struct fdt_match *match)
{
	int i;
	unsigned int hartid;
	unsigned long cluster_id;

	if (cold_boot) {
		/* initiaze the cci */
		cci_init(PLATFORM_CCI_ADDR, cci_map, array_size(cci_map));

		/* set the bootv of each cluster */
		for (i = 0; i < platform.hart_count; i += PLATFORM_MAX_CPUS_PER_CLUSTER) {

			hartid = platform.hart_index2id[i];

			cluster_id = CPU_TO_CLUSTER(hartid);

			switch (cluster_id) {
			case 0:
				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C0_L2_FLUSH_CTRL);
				break;
			case 1:
				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C1_L2_FLUSH_CTRL);
				break;
			case 2:
				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C2_L2_FLUSH_CTRL);
				break;
			case 3:
				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C3_L2_FLUSH_CTRL);
				break;
			default:
				break;
			}

			/* enable the cci */
			cci_enable_snoop_dvm_reqs(cluster_id);
		}

		/* deassert dmasys reset for cpus reach all tcm range */
		writel(1, (unsigned int *)DMASYS_RESET);
		/* enable dmasys clk for cpus reach all tcm range */
		writel(1, (unsigned int *)DMASYS_CLK_EN);

	} else {
		unsigned int current_hartid = current_hartid();

		cluster_id = CPU_TO_CLUSTER(current_hartid);

		/* enable the cci */
		cci_enable_snoop_dvm_reqs(cluster_id);
	}

	return 0;
}


unsigned long hart_imisc_save_offset;

static int spacemit_k3_final_init(bool cold_boot, void *fdt, const struct fdt_match *match)
{
	if (cold_boot)
		hart_imisc_save_offset = sbi_scratch_alloc_offset(sizeof(struct imsic_config));

	return 0;
}

static bool spacemit_k3_cold_boot_allowed(u32 hartid, const struct fdt_match *match)
{
	/* enable core snoop ,iprf and tprf*/
	csr_set(CSR_ML2SETUP, 1 << (hartid % PLATFORM_MAX_CPUS_PER_CLUSTER) | IPRF | TPRF);

	// configure XIP address region with IO attribute
	csr_clear(CSR_PMACFG0, 0xFFUL << 16);
	csr_set(CSR_PMACFG0, 0x22UL << 16);
	asm("sfence.vma zero, zero");

	if (hartid >= 8) {
		/* set the vector load instructions to bypass L1 cache,only cached in the L2 cache */
		csr_set(CSR_PERF_CTRL, VEC_L1BYPASS);
		/* Increase the L2 prefetch distance to 56 entries */
		csr_set(CSR_PREFETCH_CTRL, L2_PERF_DIST);
		/* Turn off full address correlation check to improve L2 performance */
		csr_clear(CSR_ML2HINT, CIU_CHR2_DEPD_DIS);
		csr_set(CSR_ML2HINT, CIU_CHR2_MER_DIS);
	} else {
		/* only disable x100 core*/
		/* Disable the behavior of snb discarding prefetch when reaching a certain load */
		csr_set(CSR_ML2HINT, CIU_PRF_THROT_DIS);
	}

	/* set the pmp per-core */
	spacemit_k3_pmp_init();

	/* devote early */
	spacemit_devote_pwrdown_cluster(hartid);

	/* dealing with resuming process */
	if ((__sbi_hsm_hart_get_state(hartid) == SBI_HSM_STATE_SUSPENDED) && (hartid == 0))
		return false;

	return ((hartid == 0) ? true : false);
}

const struct platform_override spacemit_k3 = {
	.match_table = spacemit_k3_mach,
	.early_init = spacemit_k3_early_init,
	.final_init = spacemit_k3_final_init,
	.cold_boot_allowed = spacemit_k3_cold_boot_allowed,
};
