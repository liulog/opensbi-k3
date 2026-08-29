/* SPDX-License-Identifier: BSD-2-Clause */

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_ecall_bench.h>

	.section .text
	.align 3
	.globl ecall_bench_run
ecall_bench_run:
	mv	t0, a0
	/* Execute this exact block once for warm-up, then a0 measured batches. */
	addi	t0, t0, 1
	li	t3, 0
	li	t4, 1
	li	a6, SBI_EXT_ECALL_BENCH_NOP
	li	a7, SBI_EXT_ECALL_BENCH

	/* No loop bookkeeping or other instruction occurs between ECALLs. */
1:
	fence	iorw, iorw
	csrr	t1, CSR_CYCLE
	fence	iorw, iorw
	.rept SBI_ECALL_BENCH_BATCH_SIZE
	ecall
	.endr
	fence	iorw, iorw
	csrr	t2, CSR_CYCLE
	fence	iorw, iorw

	sub	t2, t2, t1
	beqz	t4, 2f
	/* Discard the first pass through the exact measured code block. */
	li	t4, 0
	j	3f
2:
	add	t3, t3, t2
3:
	addi	t0, t0, -1
	beqz	t0, 4f
	j	1b
4:

	mv	a0, t3
	ret

	.align 3
	.globl ecall_bench_baseline
ecall_bench_baseline:
	mv	t0, a0
	addi	t0, t0, 1
	li	t3, 0
	li	t4, 1

	/* Match the ECALL block's 4-byte instruction footprint. */
	.option push
	.option norvc
1:
	fence	iorw, iorw
	csrr	t1, CSR_CYCLE
	fence	iorw, iorw
	.rept SBI_ECALL_BENCH_BATCH_SIZE
	nop
	.endr
	fence	iorw, iorw
	csrr	t2, CSR_CYCLE
	fence	iorw, iorw

	sub	t2, t2, t1
	beqz	t4, 2f
	li	t4, 0
	j	3f
2:
	add	t3, t3, t2
3:
	addi	t0, t0, -1
	beqz	t0, 4f
	j	1b
4:
	.option pop

	mv	a0, t3
	ret

	.align 3
	.globl ecall_bench_stop
ecall_bench_stop:
	li	a6, SBI_EXT_ECALL_BENCH_STOP
	li	a7, SBI_EXT_ECALL_BENCH
	ecall
	ret
