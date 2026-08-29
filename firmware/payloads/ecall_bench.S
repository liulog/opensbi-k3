/* SPDX-License-Identifier: BSD-2-Clause */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_ecall_bench.h>

	.section .text
	.align 3
	.globl ecall_bench_run
ecall_bench_run:
	mv	t0, a0
	li	a6, SBI_EXT_ECALL_BENCH_NOP
	li	a7, SBI_EXT_ECALL_BENCH

	/* Warm the instruction cache and branch predictor before timing. */
	li	t3, 256
1:
	ecall
	addi	t3, t3, -1
	bnez	t3, 1b

	csrr	t1, CSR_CYCLE
2:
	ecall
	addi	t0, t0, -1
	bnez	t0, 2b
	csrr	t2, CSR_CYCLE

	sub	a0, t2, t1
	ret

	.align 3
	.globl ecall_bench_baseline
ecall_bench_baseline:
	mv	t0, a0
	csrr	t1, CSR_CYCLE
1:
	addi	t0, t0, -1
	bnez	t0, 1b
	csrr	t2, CSR_CYCLE
	sub	a0, t2, t1
	ret

	.align 3
	.globl ecall_bench_stop
ecall_bench_stop:
	li	a6, SBI_EXT_ECALL_BENCH_STOP
	li	a7, SBI_EXT_ECALL_BENCH
	ecall
	ret
