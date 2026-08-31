/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef __SBI_ECALL_BENCH_H__
#define __SBI_ECALL_BENCH_H__

/* Test-only EID registered through the normal SBI extension framework. */
#define SBI_EXT_ECALL_BENCH		0x08000000
#define SBI_EXT_ECALL_BENCH_NOP		0
/* STOP is consumed only by the temporary minimal mtvec before restoration. */
#define SBI_EXT_ECALL_BENCH_STOP	1

/* Unused EID: forces sbi_ecall_find_extension() to miss and return ENOTSUPP. */
#define SBI_EXT_ECALL_BENCH_UNSUPPORTED	0x123

/* Keep each measured instruction block branch-free and approximately 4 KiB. */
#define SBI_ECALL_BENCH_BATCH_SIZE	1024
#define SBI_ECALL_BENCH_WRAPPER_BATCH_SIZE	64
#define SBI_ECALL_BENCH_BATCHES		800

#ifndef __ASSEMBLER__
void sbi_ecall_bench_prepare(void);
#endif

#endif
