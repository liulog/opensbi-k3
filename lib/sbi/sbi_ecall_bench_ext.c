/* SPDX-License-Identifier: BSD-2-Clause */

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_bench.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap.h>

static int sbi_ecall_bench_handler(unsigned long extid, unsigned long funcid,
				   struct sbi_trap_regs *regs,
				   struct sbi_ecall_return *out)
{
	(void)extid;
	(void)regs;
	(void)out;

	if (funcid != SBI_EXT_ECALL_BENCH_NOP)
		return SBI_ENOTSUPP;

	return SBI_SUCCESS;
}

struct sbi_ecall_extension ecall_bench;

static int sbi_ecall_bench_register_extensions(void)
{
	return sbi_ecall_register_extension(&ecall_bench);
}

struct sbi_ecall_extension ecall_bench = {
	.name			= "bench",
	.extid_start		= SBI_EXT_ECALL_BENCH,
	.extid_end		= SBI_EXT_ECALL_BENCH,
	.experimental		= true,
	.register_extensions	= sbi_ecall_bench_register_extensions,
	.handle			= sbi_ecall_bench_handler,
};
