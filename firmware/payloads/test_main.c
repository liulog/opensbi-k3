/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_ecall_interface.h>
#ifdef CONFIG_SBI_ECALL_BENCH
#include <sbi/sbi_ecall_bench.h>
#endif
#include <sbi/sbi_string.h>

struct sbiret {
	long error;
	unsigned long value;
};

struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
			unsigned long arg1, unsigned long arg2,
			unsigned long arg3, unsigned long arg4,
			unsigned long arg5)
{
	struct sbiret ret;

	register unsigned long a0 asm ("a0") = (unsigned long)(arg0);
	register unsigned long a1 asm ("a1") = (unsigned long)(arg1);
	register unsigned long a2 asm ("a2") = (unsigned long)(arg2);
	register unsigned long a3 asm ("a3") = (unsigned long)(arg3);
	register unsigned long a4 asm ("a4") = (unsigned long)(arg4);
	register unsigned long a5 asm ("a5") = (unsigned long)(arg5);
	register unsigned long a6 asm ("a6") = (unsigned long)(fid);
	register unsigned long a7 asm ("a7") = (unsigned long)(ext);
	asm volatile ("ecall"
		      : "+r" (a0), "+r" (a1)
		      : "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a6), "r" (a7)
		      : "memory");
	ret.error = a0;
	ret.value = a1;

	return ret;
}

static inline void sbi_ecall_console_puts(const char *str)
{
	sbi_ecall(SBI_EXT_DBCN, SBI_EXT_DBCN_CONSOLE_WRITE,
		  sbi_strlen(str), (unsigned long)str, 0, 0, 0, 0);
}

#define wfi()                                             \
	do {                                              \
		__asm__ __volatile__("wfi" ::: "memory"); \
	} while (0)

void test_main(unsigned long a0, unsigned long a1)
{
#ifdef CONFIG_SBI_ECALL_BENCH
	const unsigned long batches = SBI_ECALL_BENCH_BATCHES;
	const unsigned long batch_size = SBI_ECALL_BENCH_BATCH_SIZE;
	const unsigned long iterations = batches * batch_size;
	unsigned long total_min, total_early, total_sbi, total_unsupp;
	struct sbiret spec, unsupp;
	char output[320];
	char *p;

	extern unsigned long ecall_bench_run(unsigned long batches);
	extern unsigned long ecall_bench_run_early(unsigned long batches);
	extern unsigned long ecall_bench_run_sbi_spec(unsigned long batches);
	extern unsigned long ecall_bench_run_sbi_unsupp(unsigned long batches);
	extern void ecall_bench_stop(void);

	static const char digits[] = "0123456789";
	char tmp[3 * sizeof(unsigned long)];
	unsigned int i;

	(void)a0;
	(void)a1;

	total_min = ecall_bench_run(batches);
	ecall_bench_stop();

	total_early = ecall_bench_run_early(batches);

	spec = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_GET_SPEC_VERSION,
			 0, 0, 0, 0, 0, 0);
	total_sbi = ecall_bench_run_sbi_spec(batches);

	unsupp = sbi_ecall(SBI_EXT_ECALL_BENCH_UNSUPPORTED, 0,
			   0, 0, 0, 0, 0, 0);
	total_unsupp = ecall_bench_run_sbi_unsupp(batches);

#define APPEND_LITERAL(str) do { \
		const char *__s = (str); \
		while (*__s) \
			*p++ = *__s++; \
	} while (0)
#define APPEND_ULONG(value) do { \
		unsigned long __v = (value); \
		i = 0; \
		do { \
			tmp[i++] = digits[__v % 10]; \
			__v /= 10; \
		} while (__v); \
		while (i) \
			*p++ = tmp[--i]; \
	} while (0)
#define APPEND_CYCLES_PER(total) do { \
		unsigned long __avg = (total) / iterations; \
		unsigned long __rem = (total) % iterations; \
		APPEND_ULONG(__avg); \
		APPEND_LITERAL("."); \
		__rem = (__rem * 1000) / iterations; \
		*p++ = digits[(__rem / 100) % 10]; \
		*p++ = digits[(__rem / 10) % 10]; \
		*p++ = digits[__rem % 10]; \
	} while (0)
#define FLUSH_REPORT() do { \
		*p = '\0'; \
		sbi_ecall_console_puts(output); \
	} while (0)

	p = output;
	APPEND_LITERAL("\nS-mode ECALL latency benchmark\n");
	APPEND_LITERAL("path            : minimal mtvec\n");
	APPEND_LITERAL("batches         : ");
	APPEND_ULONG(batches);
	APPEND_LITERAL("\necalls/batch    : ");
	APPEND_ULONG(batch_size);
	APPEND_LITERAL("\niterations      : ");
	APPEND_ULONG(iterations);
	APPEND_LITERAL("\nmeasured cycles : ");
	APPEND_ULONG(total_min);
	APPEND_LITERAL("\ncycles/ecall    : ");
	APPEND_CYCLES_PER(total_min);
	APPEND_LITERAL("\n");
	FLUSH_REPORT();

	p = output;
	APPEND_LITERAL("\npath            : original handler early a7\n");
	APPEND_LITERAL("batches         : ");
	APPEND_ULONG(batches);
	APPEND_LITERAL("\necalls/batch    : ");
	APPEND_ULONG(batch_size);
	APPEND_LITERAL("\niterations      : ");
	APPEND_ULONG(iterations);
	APPEND_LITERAL("\nmeasured cycles : ");
	APPEND_ULONG(total_early);
	APPEND_LITERAL("\ncycles/ecall    : ");
	APPEND_CYCLES_PER(total_early);
	APPEND_LITERAL("\n");
	FLUSH_REPORT();

	p = output;
	APPEND_LITERAL("\npath            : full SBI get_spec_version\n");
	APPEND_LITERAL("sbi spec        : ");
	if (spec.error) {
		APPEND_LITERAL("error ");
		APPEND_ULONG((unsigned long)spec.error);
	} else {
		APPEND_ULONG((spec.value >> SBI_SPEC_VERSION_MAJOR_OFFSET) &
			     SBI_SPEC_VERSION_MAJOR_MASK);
		APPEND_LITERAL(".");
		APPEND_ULONG(spec.value & SBI_SPEC_VERSION_MINOR_MASK);
	}
	APPEND_LITERAL("\nbatches         : ");
	APPEND_ULONG(batches);
	APPEND_LITERAL("\necalls/batch    : ");
	APPEND_ULONG(batch_size);
	APPEND_LITERAL("\niterations      : ");
	APPEND_ULONG(iterations);
	APPEND_LITERAL("\nmeasured cycles : ");
	APPEND_ULONG(total_sbi);
	APPEND_LITERAL("\ncycles/ecall    : ");
	APPEND_CYCLES_PER(total_sbi);
	APPEND_LITERAL("\n");
	FLUSH_REPORT();

	p = output;
	APPEND_LITERAL("\npath            : full SBI unsupported ext\n");
	APPEND_LITERAL("sbi error       : ");
	if (unsupp.error < 0) {
		APPEND_LITERAL("-");
		APPEND_ULONG((unsigned long)(-unsupp.error));
	} else {
		APPEND_ULONG((unsigned long)unsupp.error);
	}
	APPEND_LITERAL("\nbatches         : ");
	APPEND_ULONG(batches);
	APPEND_LITERAL("\necalls/batch    : ");
	APPEND_ULONG(batch_size);
	APPEND_LITERAL("\niterations      : ");
	APPEND_ULONG(iterations);
	APPEND_LITERAL("\nmeasured cycles : ");
	APPEND_ULONG(total_unsupp);
	APPEND_LITERAL("\ncycles/ecall    : ");
	APPEND_CYCLES_PER(total_unsupp);
	APPEND_LITERAL("\n");
	FLUSH_REPORT();

#undef FLUSH_REPORT
#undef APPEND_CYCLES_PER
#undef APPEND_ULONG
#undef APPEND_LITERAL
#else
	sbi_ecall_console_puts("\nTest payload running\n");
#endif

	while (1)
		wfi();
}
