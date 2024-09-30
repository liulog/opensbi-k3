/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Intel Corporation.
 * Copyright (c) 2024 Andes Technology Corporation.
 */

#ifndef __SBI_DOMAIN_MPXY_STATE_H__
#define __SBI_DOMAIN_MPXY_STATE_H__

#include <sbi/sbi_types.h>

struct sbi_domain;

/** Per hart shared memory */
struct mpxy_shmem {
	unsigned long shmem_size;
	unsigned long shmem_addr_lo;
	unsigned long shmem_addr_hi;
};

/** Context representation for a hart within a domain */
struct hart_mpxy_state {
	/* MSI support in MPXY */
	bool msi_avail;
	/* SSE support in MPXY */
	bool sse_avail;
	/* MPXY Shared memory details */
	struct mpxy_shmem shmem;
};

struct hart_mpxy_state *hart_mpxy_state_get(struct sbi_domain *dom,
					    u32 hartindex);
void hart_mpxy_state_set(struct sbi_domain *dom, u32 hartindex,
			     struct hart_mpxy_state *hc);
/**
 * Initialize domain mpxy state support
 *
 * @return 0 on success and negative error code on failure
 */
int sbi_domain_mpxy_state_init(void);

/* Deinitialize domain mpxy state support */
void sbi_domain_mpxy_state_deinit(void);

#endif // __SBI_DOMAIN_MPXY_STATE_H__
