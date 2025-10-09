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

static struct mpxy_rpmi_service_data rtc_services[] = {
{
	.id = RPMI_RTC_SRV_ENABLE_NOTIFICATION,
	.min_tx_len = sizeof(struct rpmi_enable_notification_req),
	.max_tx_len = sizeof(struct rpmi_enable_notification_req),
	.min_rx_len = sizeof(struct rpmi_enable_notification_resp),
	.max_rx_len = sizeof(struct rpmi_enable_notification_resp),
},
{
	.id = RPMI_RTC_SRV_SET_TIME,
	.min_tx_len = sizeof(struct rpmi_rtc_set_time_req),
	.max_tx_len = sizeof(struct rpmi_rtc_set_time_req),
	.min_rx_len = sizeof(struct rpmi_rtc_set_time_resp),
	.max_rx_len = sizeof(struct rpmi_rtc_set_time_resp),
},
{
	.id = RPMI_RTC_SRV_GET_TIME,
	.min_tx_len = sizeof(struct rpmi_rtc_get_time_req),
	.max_tx_len = sizeof(struct rpmi_rtc_get_time_req),
	.min_rx_len = sizeof(struct rpmi_rtc_get_time_resp),
	.max_rx_len = sizeof(struct rpmi_rtc_get_time_resp),
},
{
	.id = RPMI_RTC_SRV_SET_ALARM,
	.min_tx_len = sizeof(struct rpmi_rtc_set_alarm_req),
	.max_tx_len = sizeof(struct rpmi_rtc_set_alarm_req),
	.min_rx_len = sizeof(struct rpmi_rtc_set_alarm_resp),
	.max_rx_len = sizeof(struct rpmi_rtc_set_alarm_resp),
},
{
	.id = RPMI_RTC_SRV_GET_ALARM,
	.min_tx_len = sizeof(struct rpmi_rtc_get_alarm_req),
	.max_tx_len = sizeof(struct rpmi_rtc_get_alarm_req),
	.min_rx_len = sizeof(struct rpmi_rtc_get_alarm_resp),
	.max_rx_len = sizeof(struct rpmi_rtc_get_alarm_resp),
},
{
	.id = RPMI_RTC_SRV_ALARM_GET_EN,
	.min_tx_len = sizeof(struct rpmi_rtc_get_alarm_en_req),
	.max_tx_len = sizeof(struct rpmi_rtc_get_alarm_en_req),
	.min_rx_len = sizeof(struct rpmi_rtc_get_alarm_en_resp),
	.max_rx_len = sizeof(struct rpmi_rtc_get_alarm_en_resp),
},
{
	.id = RPMI_RTC_SRV_ALARM_SET_EN,
	.min_tx_len = sizeof(struct rpmi_rtc_set_alarm_en_req),
	.max_tx_len = sizeof(struct rpmi_rtc_set_alarm_en_req),
	.min_rx_len = sizeof(struct rpmi_rtc_set_alarm_en_resp),
	.max_rx_len = sizeof(struct rpmi_rtc_set_alarm_en_resp),
},
{
	.id = RPMI_RTC_SRV_QUERY_PENDING,
	.min_tx_len = sizeof(struct rpmi_rtc_query_pending_req),
	.max_tx_len = sizeof(struct rpmi_rtc_query_pending_req),
	.min_rx_len = sizeof(struct rpmi_rtc_query_pending_resp),
	.max_rx_len = sizeof(struct rpmi_rtc_query_pending_resp),
},
{
	.id = RPMI_RTC_SRV_CLR_PENDING,
	.min_tx_len = sizeof(struct rpmi_rtc_clear_pending_req),
	.max_tx_len = sizeof(struct rpmi_rtc_clear_pending_req),
	.min_rx_len = sizeof(struct rpmi_rtc_clear_pending_resp),
	.max_rx_len = sizeof(struct rpmi_rtc_clear_pending_resp),
},
};

static const struct mpxy_rpmi_mbox_data rtc_data = {
	.servicegrp_id = RPMI_SRVGRP_RTC,
	.num_services = RPMI_RTC_SRV_MAX_COUNT,
	.service_data = rtc_services,
};

static const struct fdt_match rtc_match[] = {
	{ .compatible = "riscv,rpmi-mpxy-rtc", .data = &rtc_data },
	{ },
};

const struct fdt_driver fdt_mpxy_rpmi_rtc = {
	.match_table = rtc_match,
	.init = mpxy_rpmi_mbox_init,
	.experimental = true,
};
