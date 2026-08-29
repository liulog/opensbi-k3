/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef __SBI_ECALL_BENCH_H__
#define __SBI_ECALL_BENCH_H__

/* Private EID from the SBI experimental extension range. */
#define SBI_EXT_ECALL_BENCH		0x08000000
#define SBI_EXT_ECALL_BENCH_NOP		0
#define SBI_EXT_ECALL_BENCH_STOP	1

/* Keep each measured instruction block branch-free and approximately 4 KiB. */
#define SBI_ECALL_BENCH_BATCH_SIZE	1024
#define SBI_ECALL_BENCH_BATCHES		800

#ifndef __ASSEMBLER__
void sbi_ecall_bench_prepare(void);
#endif

#endif
