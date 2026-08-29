/* SPDX-License-Identifier: BSD-2-Clause */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_ecall_bench.h>

	.section .text
	.align 3
	.globl sbi_ecall_bench_prepare
sbi_ecall_bench_prepare:
	csrr	t0, CSR_MTVEC
	lla	t1, .Lsaved_mtvec
	REG_S	t0, 0(t1)

	csrr	t0, CSR_MIE
	lla	t1, .Lsaved_mie
	REG_S	t0, 0(t1)
	csrw	CSR_MIE, zero

	csrr	t0, CSR_MCOUNTEREN
	lla	t1, .Lsaved_mcounteren
	REG_S	t0, 0(t1)
	ori	t0, t0, 1
	csrw	CSR_MCOUNTEREN, t0

	lla	t0, sbi_ecall_bench_trap
	csrw	CSR_MTVEC, t0
	ret

	.align 3
	.globl sbi_ecall_bench_trap
sbi_ecall_bench_trap:
	bnez	a6, .Lbench_stop

	/* Minimal NOP return path: only a0 and a1 are clobbered. */
	csrr	a0, CSR_MEPC
	addi	a0, a0, 4
	csrw	CSR_MEPC, a0
	li	a0, 0
	li	a1, 0
	mret

.Lbench_stop:
	/* Restore normal OpenSBI trap and counter state outside measurement. */
	lla	a0, .Lsaved_mtvec
	REG_L	a0, 0(a0)
	csrw	CSR_MTVEC, a0

	lla	a0, .Lsaved_mcounteren
	REG_L	a0, 0(a0)
	csrw	CSR_MCOUNTEREN, a0

	lla	a0, .Lsaved_mie
	REG_L	a0, 0(a0)
	csrw	CSR_MIE, a0

	csrr	a0, CSR_MEPC
	addi	a0, a0, 4
	csrw	CSR_MEPC, a0
	li	a0, 0
	li	a1, 0
	mret

	.section .data
	.align 3
.Lsaved_mtvec:
	RISCV_PTR 0
.Lsaved_mie:
	RISCV_PTR 0
.Lsaved_mcounteren:
	RISCV_PTR 0
