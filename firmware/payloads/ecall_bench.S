/* SPDX-License-Identifier: BSD-2-Clause */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_ecall_bench.h>

	/* Keep these local: sbi_ecall_interface.h is not assembler-safe. */
#define SBI_EXT_BASE			0x10
#define SBI_EXT_BASE_GET_SPEC_VERSION	0x0

	.section .text
	.align 3
	.globl ecall_bench_run
ecall_bench_run:
	li	a6, SBI_EXT_ECALL_BENCH_NOP
	li	a7, SBI_EXT_ECALL_BENCH
	j	.Lecall_bench_measure

	.align 3
	.globl ecall_bench_run_extension
ecall_bench_run_extension:
	/*
	 * Original _trap_handler and standard SBI extension dispatch path.
	 * The registered benchmark extension handles FID 0 as an empty call.
	 * Must run after ecall_bench_stop(); otherwise the bench mtvec
	 * would take the 7-instruction shortcut.
	 */
	li	a6, SBI_EXT_ECALL_BENCH_NOP
	li	a7, SBI_EXT_ECALL_BENCH
	j	.Lecall_bench_measure

	.align 3
	.globl ecall_bench_run_sbi_spec
ecall_bench_run_sbi_spec:
	/*
	 * Full OpenSBI path: SBI_EXT_BASE / GET_SPEC_VERSION.
	 * Must run after ecall_bench_stop(); the FID is 0, so the bench
	 * mtvec would treat this as a NOP and the number would be fake.
	 */
	li	a6, SBI_EXT_BASE_GET_SPEC_VERSION
	li	a7, SBI_EXT_BASE
	j	.Lecall_bench_measure

	.align 3
	.globl ecall_bench_run_sbi_unsupp
ecall_bench_run_sbi_unsupp:
	/*
	 * Full OpenSBI path: unregistered EID, same dispatch as spec
	 * version but find_extension() misses and returns ENOTSUPP.
	 * Must run after ecall_bench_stop().
	 */
	li	a6, 0
	li	a7, SBI_EXT_ECALL_BENCH_UNSUPPORTED
	j	.Lecall_bench_measure

	/*
	 * a0 = measured batch count (SBI_ECALL_BENCH_BATCHES).
	 *
	 * t0 : remaining outer-loop trips = a0 + 1 (one extra warmup pass)
	 * t1 : rdcycle at the start of this batch
	 * t2 : rdcycle at the end, then overwritten with the batch delta
	 * t3 : accumulated cycles, warmup excluded
	 * t4 : warmup flag, 1 = discard this batch, 0 = accumulate
	 * a6/a7 : SBI args, left unchanged inside the timed window
	 */
.Lecall_bench_measure:
	mv	t0, a0
	addi	t0, t0, 1
	li	t3, 0
	li	t4, 1

	/* -------- Outer loop: label 1 -> 4, t0 batches -------- */
1:
	/* Start timing. Fences keep the cycle reads on a true boundary. */
	fence	iorw, iorw
	csrr	t1, CSR_CYCLE
	fence	iorw, iorw

	/*
	 * Inner block: .rept unrolls BATCH_SIZE consecutive ecalls.
	 * No branches or counters; nothing sits between two ecalls.
	 */
	.rept SBI_ECALL_BENCH_BATCH_SIZE
	ecall
	.endr

	/* Stop timing. */
	fence	iorw, iorw
	csrr	t2, CSR_CYCLE
	fence	iorw, iorw

	/* Outer-loop epilogue: compute this batch, discard or accumulate. */
	sub	t2, t2, t1
	beqz	t4, 2f
	/* First batch is warmup: drop it so cold I-cache / trap is excluded. */
	li	t4, 0
	j	3f
2:
	/* Later batches: add into the running total. */
	add	t3, t3, t2
3:
	/* Outer-loop back-edge: t0--, jump to 1 if more batches remain. */
	addi	t0, t0, -1
	beqz	t0, 4f
	j	1b
4:
	/* Return accumulated measured cycles (warmup already excluded). */
	mv	a0, t3
	ret

	/*
	 * Linux-like S-mode wrapper path.  Each measured operation prepares all
	 * eight SBI arguments and calls the out-of-line C wrapper.  The wrapper
	 * itself executes the ECALL and returns a0/a1 as struct sbiret.
	 */
	.align 3
	.globl ecall_bench_run_sbi_wrapper
ecall_bench_run_sbi_wrapper:
	addi	sp, sp, -(6 * __SIZEOF_POINTER__)
	REG_S	ra, (5 * __SIZEOF_POINTER__)(sp)
	REG_S	s0, (4 * __SIZEOF_POINTER__)(sp)
	REG_S	s1, (3 * __SIZEOF_POINTER__)(sp)
	REG_S	s2, (2 * __SIZEOF_POINTER__)(sp)
	REG_S	s3, (1 * __SIZEOF_POINTER__)(sp)
	REG_S	s4, (0 * __SIZEOF_POINTER__)(sp)

	mv	s0, a0
	addi	s0, s0, 1
	li	s3, 1
	li	s4, 0

5:
	fence	iorw, iorw
	csrr	s1, CSR_CYCLE
	fence	iorw, iorw

	/* Match Linux CALL_64: no loop branch occurs inside the timed body. */
	.rept SBI_ECALL_BENCH_WRAPPER_BATCH_SIZE
	li	a0, 0
	li	a1, 0
	li	a2, 0
	li	a3, 0
	li	a4, 0
	li	a5, 0
	li	a6, SBI_EXT_BASE_GET_SPEC_VERSION
	li	a7, SBI_EXT_BASE
	call	ecall_bench_sbi_wrapper
	.endr

	fence	iorw, iorw
	csrr	s2, CSR_CYCLE
	fence	iorw, iorw

	sub	s2, s2, s1
	beqz	s3, 6f
	li	s3, 0
	j	7f
6:
	add	s4, s4, s2
7:
	addi	s0, s0, -1
	bnez	s0, 5b

	mv	a0, s4
	REG_L	s4, (0 * __SIZEOF_POINTER__)(sp)
	REG_L	s3, (1 * __SIZEOF_POINTER__)(sp)
	REG_L	s2, (2 * __SIZEOF_POINTER__)(sp)
	REG_L	s1, (3 * __SIZEOF_POINTER__)(sp)
	REG_L	s0, (4 * __SIZEOF_POINTER__)(sp)
	REG_L	ra, (5 * __SIZEOF_POINTER__)(sp)
	addi	sp, sp, (6 * __SIZEOF_POINTER__)
	ret

	/* Same wrapper overhead, but exercise the extension lookup miss path. */
	.align 3
	.globl ecall_bench_run_sbi_unsupp_wrapper
ecall_bench_run_sbi_unsupp_wrapper:
	addi	sp, sp, -(6 * __SIZEOF_POINTER__)
	REG_S	ra, (5 * __SIZEOF_POINTER__)(sp)
	REG_S	s0, (4 * __SIZEOF_POINTER__)(sp)
	REG_S	s1, (3 * __SIZEOF_POINTER__)(sp)
	REG_S	s2, (2 * __SIZEOF_POINTER__)(sp)
	REG_S	s3, (1 * __SIZEOF_POINTER__)(sp)
	REG_S	s4, (0 * __SIZEOF_POINTER__)(sp)

	mv	s0, a0
	addi	s0, s0, 1
	li	s3, 1
	li	s4, 0

8:
	fence	iorw, iorw
	csrr	s1, CSR_CYCLE
	fence	iorw, iorw

	.rept SBI_ECALL_BENCH_WRAPPER_BATCH_SIZE
	li	a0, 0
	li	a1, 0
	li	a2, 0
	li	a3, 0
	li	a4, 0
	li	a5, 0
	li	a6, 0
	li	a7, SBI_EXT_ECALL_BENCH_UNSUPPORTED
	call	ecall_bench_sbi_wrapper
	.endr

	fence	iorw, iorw
	csrr	s2, CSR_CYCLE
	fence	iorw, iorw

	sub	s2, s2, s1
	beqz	s3, 9f
	li	s3, 0
	j	10f
9:
	add	s4, s4, s2
10:
	addi	s0, s0, -1
	bnez	s0, 8b

	mv	a0, s4
	REG_L	s4, (0 * __SIZEOF_POINTER__)(sp)
	REG_L	s3, (1 * __SIZEOF_POINTER__)(sp)
	REG_L	s2, (2 * __SIZEOF_POINTER__)(sp)
	REG_L	s1, (3 * __SIZEOF_POINTER__)(sp)
	REG_L	s0, (4 * __SIZEOF_POINTER__)(sp)
	REG_L	ra, (5 * __SIZEOF_POINTER__)(sp)
	addi	sp, sp, (6 * __SIZEOF_POINTER__)
	ret

	.align 3
	.globl ecall_bench_stop
ecall_bench_stop:
	li	a6, SBI_EXT_ECALL_BENCH_STOP
	li	a7, SBI_EXT_ECALL_BENCH
	ecall
	ret
