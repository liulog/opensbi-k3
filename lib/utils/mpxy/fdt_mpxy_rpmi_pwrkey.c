/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Spacemit.
 */

#include <sbi/sbi_bitmap.h>
#include <sbi/sbi_domain.h>
#include <sbi/sbi_byteorder.h>
#include <sbi/sbi_heap.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>
#include <sbi_utils/mpxy/fdt_mpxy_rpmi_mbox.h>

static struct mpxy_rpmi_service_data pwrkey_services[] = {
{
	.id = RPMI_PWRKEY_SRV_ENABLE_NOTIFICATION,
	.min_tx_len = sizeof(struct rpmi_enable_notification_req),
	.max_tx_len = sizeof(struct rpmi_enable_notification_req),
	.min_rx_len = sizeof(struct rpmi_enable_notification_resp),
	.max_rx_len = sizeof(struct rpmi_enable_notification_resp),
},
{
	.id = RPMI_PWRKEY_SRV_QUERY_PENDING,
	.min_tx_len = sizeof(struct rpmi_pwrkey_query_pending_req),
	.max_tx_len = sizeof(struct rpmi_pwrkey_query_pending_req),
	.min_rx_len = sizeof(struct rpmi_pwrkey_query_pending_resp),
	.max_rx_len = sizeof(struct rpmi_pwrkey_query_pending_resp),
},
{
	.id = RPMI_PWRKEY_SRV_CLR_PENDING,
	.min_tx_len = sizeof(struct rpmi_pwrkey_clear_pending_req),
	.max_tx_len = sizeof(struct rpmi_pwrkey_clear_pending_req),
	.min_rx_len = sizeof(struct rpmi_pwrkey_clear_pending_resp),
	.max_rx_len = sizeof(struct rpmi_pwrkey_clear_pending_resp),
},
};

static const struct mpxy_rpmi_mbox_data pwrkey_data = {
	.servicegrp_id = RPMI_SRVGRP_PWRKEY,
	.num_services = RPMI_PWRKEY_SRV_ID_MAX_COUNT,
	.service_data = pwrkey_services,
};

static const struct fdt_match pwrkey_match[] = {
	{ .compatible = "riscv,rpmi-mpxy-pwrkey", .data = &pwrkey_data },
	{ },
};

const struct fdt_driver fdt_mpxy_rpmi_pwrkey = {
	.match_table = pwrkey_match,
	.init = mpxy_rpmi_mbox_init,
	.experimental = true,
};
