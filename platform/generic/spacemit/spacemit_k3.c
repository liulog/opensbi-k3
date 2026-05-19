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
#include <sbi/sbi_domain.h>
#include <sbi/sbi_timer.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <spacemit/spacemit_config.h>
#include <sbi_utils/cache/cache.h>
#include <sbi_utils/cci/cci.h>

static const struct fdt_match spacemit_k3_mach[] = {
	{ .compatible = "spacemit,k3" },
	{ .compatible = "riscv-spacemit" },
	{ },
};

PLAT_CCI_MAP;
extern struct sbi_platform platform;
extern void _start_warm(unsigned long);
extern void _start_warm_dummy(unsigned long);

void boot_entry_dummy(unsigned long sc)
{
	/* set the vector load instructions to bypass L1 cache,only cached in the L2 cache */
	csr_set(CSR_PERF_CTRL, VEC_L1BYPASS);
	/* Increase the L2 prefetch distance to 56 entries */
	csr_set(CSR_PREFETCH_CTRL, L2_PERF_DIST);
	/* Turn off full address correlation check to improve L2 performance */
	csr_clear(CSR_ML2HINT, CIU_CHR2_DEPD_DIS);
	csr_set(CSR_ML2HINT, CIU_CHR2_MER_DIS);

	/* devote early */
	spacemit_devote_pwrdown_cluster(current_hartid());

	/* de-vote core acpr */
	spacemit_devote_core_apcr(current_hartid());

	/* re-set the bootentry of cluster2 */
	writel((unsigned long)_start_warm & 0xffffffff, (unsigned int *)(C2_RVBADDR_LO_ADDR));
	writel((((unsigned long)_start_warm) >> 32) & 0xffffffff, (unsigned int*)(C2_RVBADDR_HI_ADDR));

	spacemit_vote_powrdown_core(8);

	/* disable local timer */
	csr_write(CSR_STIMECMP, 0xffffffffffffffff);
	/* disable all irq */
	csr_clear(CSR_MIE, MIP_SSIP | MIP_MSIP | MIP_STIP | MIP_MTIP | MIP_SEIP | MIP_MEIP);
	/* disable prefetch */
	csi_disable_data_preftch();
	asm volatile ("fence iorw, iorw");
	/* flush dcache all */
	csi_flush_dcache_all();
	asm volatile ("fence iorw, iorw");
	/* disable i/d cache */
	csi_disable_cache();
	asm volatile ("fence iorw, iorw");
	/* disable core snoop */
	csr_clear(CSR_ML2SETUP, 1 << (current_hartid() % PLATFORM_MAX_CPUS_PER_CLUSTER));
	asm volatile ("fence iorw, iorw");

	while (1)
		wfi();
}

#define CPU_TO_CLUSTER(cpu)    ((cpu) / PLATFORM_MAX_CPUS_PER_CLUSTER)

static int spacemit_k3_early_init(bool cold_boot, const void *fdt, const struct fdt_match *match)
{
	int i, rc;
	unsigned int hartid;
	unsigned long cluster_id;
	struct sbi_scratch *scratch;

	if (cold_boot) {
		/* initiaze the cci */
		cci_init(PLATFORM_CCI_ADDR, cci_map, array_size(cci_map));

		/* set the bootv of each cluster */
		for (i = 0; i < platform.hart_count; i += PLATFORM_MAX_CPUS_PER_CLUSTER) {

			hartid = platform.hart_index2id[i];
			scratch = sbi_hartid_to_scratch(hartid);

			cluster_id = CPU_TO_CLUSTER(hartid);

                        switch (cluster_id) {
                        case 0:
				writel(scratch->warmboot_addr & 0xffffffff, (unsigned int *)(C0_RVBADDR_LO_ADDR));
				writel((scratch->warmboot_addr >> 32) & 0xffffffff, (unsigned int*)(C0_RVBADDR_HI_ADDR));

				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C0_L2_FLUSH_CTRL);
				break;
                       case 1:
				writel(scratch->warmboot_addr & 0xffffffff, (unsigned int *)(C1_RVBADDR_LO_ADDR));
				writel((scratch->warmboot_addr >> 32) & 0xffffffff, (unsigned int*)(C1_RVBADDR_HI_ADDR));

				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C1_L2_FLUSH_CTRL);
				break;
                       case 2:
				writel(scratch->warmboot_addr & 0xffffffff, (unsigned int *)(C2_RVBADDR_LO_ADDR));
				writel((scratch->warmboot_addr >> 32) & 0xffffffff, (unsigned int*)(C2_RVBADDR_HI_ADDR));

				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C2_L2_FLUSH_CTRL);
				break;
                       case 3:
				writel(scratch->warmboot_addr & 0xffffffff, (unsigned int *)(C3_RVBADDR_LO_ADDR));
				writel((scratch->warmboot_addr >> 32) & 0xffffffff, (unsigned int*)(C3_RVBADDR_HI_ADDR));

				/* using hw type to flush l2 cache */
				writel(PMU_L2_FLUSH_HW_EN | PMU_L2_FLUSH_HW_TYPE, (unsigned int *)PMU_C3_L2_FLUSH_CTRL);
				break;
			default:
				break;
			}

		}

		/* enable the cci */
		cci_enable_snoop_dvm_reqs(0);
		cci_enable_snoop_dvm_reqs(1);
		cci_enable_snoop_dvm_reqs(2);
		cci_enable_snoop_dvm_reqs(3);
		cci_enable_snoop_dvm_reqs(4);
		cci_enable_snoop_dvm_reqs(5);
		cci_enable_snoop_dvm_reqs(6);

		for (i = 0; i < platform.hart_count; ++i)
			/* devote the cluster */
			spacemit_devote_pwrdown_cluster(i);

		/* then wakeup core8 which belongs cluster2 */
		writel(((unsigned long)_start_warm_dummy) & 0xffffffff, (unsigned int *)(C2_RVBADDR_LO_ADDR));
		writel((((unsigned long)_start_warm_dummy) >> 32) & 0xffffffff, (unsigned int*)(C2_RVBADDR_HI_ADDR));
		writel((1 << 8), (unsigned int *)PMU_CAP_CORE8_WAKEUP);

		/* deassert dmasys reset for cpus reach all tcm range */
		writel(1, (unsigned int *)DMASYS_RESET);
		/* enable dmasys clk for cpus reach all tcm range */
		writel(1, (unsigned int *)DMASYS_CLK_EN);

		/*
		 * Protect reserved memory regions: no R/W/X for M/S/U modes.
		 * ENF_PERMISSIONS locks the PMP entry so M-mode is also denied.
		 * Protect the rcpu runtime environment from corruption
		 */
		rc = sbi_domain_root_add_memrange(RCPU0_RUNTIME_SPACE_BASE_ADDR, RCPU0_RUNTIME_SPACE_SIZE, 0x100000UL,
						  SBI_DOMAIN_MEMREGION_ENF_PERMISSIONS);
		if (rc)
			return rc;

		rc = sbi_domain_root_add_memrange(RCPU1_RUNTIME_SPACE_BASE_ADDR, RCPU1_RUNTIME_SPACE_SIZE, 0x100000UL,
						  SBI_DOMAIN_MEMREGION_ENF_PERMISSIONS);
		if (rc)
			return rc;

		rc = sbi_domain_root_add_memrange(RCPU_DTB_SPACE_BASE_ADDR, RCPU_DTB_SPACEMI_SIZE, 0x100000UL,
						  SBI_DOMAIN_MEMREGION_ENF_PERMISSIONS);
		if (rc)
			return rc;
	} else {
		unsigned int current_hartid = current_hartid();

		cluster_id = CPU_TO_CLUSTER(current_hartid);
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

	if (hartid >= 8) {
		/* set the vector load instructions to bypass L1 cache,only cached in the L2 cache */
		csr_set(CSR_PERF_CTRL, VEC_L1BYPASS);
		/* Increase the L2 prefetch distance to 56 entries */
		csr_set(CSR_PREFETCH_CTRL, L2_PERF_DIST);
		/* Turn off full address correlation check to improve L2 performance */
		csr_clear(CSR_ML2HINT, CIU_CHR2_DEPD_DIS);
		csr_set(CSR_ML2HINT, CIU_CHR2_MER_DIS);
	}

	/* devote early */
	spacemit_devote_pwrdown_cluster(hartid);

	/* de-vote core acpr */
	spacemit_devote_core_apcr(hartid);

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
