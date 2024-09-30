/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Intel Corporation.
 * Copyright (c) 2024 Andes Technology Corporation.
 */

#include <sbi/sbi_domain.h>

struct domain_mpxy_state_priv {
	/** MPXY state for possible HARTs indexed by hartindex */
	struct hart_mpxy_state *hartindex_to_mpxy_state_table[SBI_HARTMASK_MAX_BITS];
};

static struct sbi_domain_data dmspriv = {
	.data_size = sizeof(struct domain_mpxy_state_priv),
};

struct hart_mpxy_state *hart_mpxy_state_get(struct sbi_domain *dom,
					    u32 hartindex)
{
	struct domain_mpxy_state_priv *dmsp = sbi_domain_data_ptr(dom, &dmspriv);

	return (dmsp && hartindex < SBI_HARTMASK_MAX_BITS) ?
		dmsp->hartindex_to_mpxy_state_table[hartindex] : NULL;
}

void hart_mpxy_state_set(struct sbi_domain *dom, u32 hartindex,
			 struct hart_mpxy_state *hms)
{
	struct domain_mpxy_state_priv *dmsp = sbi_domain_data_ptr(dom, &dmspriv);

	if (dmsp && hartindex < SBI_HARTMASK_MAX_BITS) {
		dmsp->hartindex_to_mpxy_state_table[hartindex] = hms;
	}
}

int sbi_domain_mpxy_state_init(void)
{
	return sbi_domain_register_data(&dmspriv);
}

void sbi_domain_mpxy_state_deinit(void)
{
	sbi_domain_unregister_data(&dmspriv);
}
