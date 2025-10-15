/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Rahul Pathak <rpathak@ventanamicro.com>
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi_utils/mpxy/fdt_mpxy_rpmi_mbox.h>

static struct mpxy_rpmi_service_data domain_services[] = {
{
	.id = RPMI_DOMAIN_SRV_ENABLE_NOTIFICATION,
	.min_tx_len = sizeof(struct rpmi_enable_notification_req),
	.max_tx_len = sizeof(struct rpmi_enable_notification_req),
	.min_rx_len = sizeof(struct rpmi_enable_notification_resp),
	.max_rx_len = sizeof(struct rpmi_enable_notification_resp),
},
{
	.id = RPMI_DOMAIN_SRV_GET_NUM_DOMAINS,
	.min_tx_len = 0,
	.max_tx_len = 0,
	.min_rx_len = sizeof(struct rpmi_domain_get_num_domains_resp),
	.max_rx_len = sizeof(struct rpmi_domain_get_num_domains_resp),
},
{
	.id = RPMI_DOMAIN_SRV_GET_ATTRIBUTES,
	.min_tx_len = sizeof(struct rpmi_domain_get_attributes_req),
	.max_tx_len = sizeof(struct rpmi_domain_get_attributes_req),
	.min_rx_len = sizeof(struct rpmi_domain_get_attributes_resp),
	.max_rx_len = sizeof(struct rpmi_domain_get_attributes_resp),
},
{
	.id = RPMI_DOMAIN_SRV_SET_STATE,
	.min_tx_len = sizeof(struct rpmi_domain_set_state_req),
	.max_tx_len = sizeof(struct rpmi_domain_set_state_req),
	.min_rx_len = sizeof(struct rpmi_domain_set_state_resp),
	.max_rx_len = sizeof(struct rpmi_domain_set_state_resp),
},
{
	.id = RPMI_DOMAIN_SRV_GET_STATE,
	.min_tx_len = sizeof(struct rpmi_domain_get_state_req),
	.max_tx_len = sizeof(struct rpmi_domain_get_state_req),
	.min_rx_len = sizeof(struct rpmi_domain_get_state_resp),
	.max_rx_len = sizeof(struct rpmi_domain_get_state_resp),
},
};

static const struct mpxy_rpmi_mbox_data domain_data = {
	.servicegrp_id = RPMI_SRVGRP_DOMAIN,
	.num_services = RPMI_DOMAIN_SRV_MAX_COUNT,
	.service_data = domain_services,
};

static const struct fdt_match domain_match[] = {
	{ .compatible = "riscv,rpmi-mpxy-domain", .data = &domain_data },
	{ },
};

const struct fdt_driver fdt_mpxy_rpmi_domain = {
	.match_table = domain_match,
	.init = mpxy_rpmi_mbox_init,
	.experimental = true,
};
