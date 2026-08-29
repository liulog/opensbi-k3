/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_ecall_bench.h>
#include <sbi/sbi_string.h>

struct sbiret {
	unsigned long error;
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
	unsigned long total, baseline, net;
	unsigned long average, remainder, net_average, net_remainder;
	char output[384];
	char *p = output;

	extern unsigned long ecall_bench_run(unsigned long batches);
	extern unsigned long ecall_bench_baseline(unsigned long batches);
	extern void ecall_bench_stop(void);

	static const char digits[] = "0123456789";
	char tmp[3 * sizeof(unsigned long)];
	unsigned int i;

	(void)a0;
	(void)a1;

	baseline = ecall_bench_baseline(batches);
	total = ecall_bench_run(batches);
	ecall_bench_stop();

	net = total > baseline ? total - baseline : 0;
	average = total / iterations;
	remainder = total % iterations;
	net_average = net / iterations;
	net_remainder = net % iterations;

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

	APPEND_LITERAL("\nS-mode ECALL latency benchmark\n");
	APPEND_LITERAL("batches         : ");
	APPEND_ULONG(batches);
	APPEND_LITERAL("\necalls/batch    : ");
	APPEND_ULONG(batch_size);
	APPEND_LITERAL("\niterations      : ");
	APPEND_ULONG(iterations);
	APPEND_LITERAL("\nmeasured cycles : ");
	APPEND_ULONG(total);
	APPEND_LITERAL("\nnop cycles      : ");
	APPEND_ULONG(baseline);
	APPEND_LITERAL("\nnet cycles      : ");
	APPEND_ULONG(net);
	APPEND_LITERAL("\ncycles/ecall    : ");
	APPEND_ULONG(average);
	APPEND_LITERAL(".");
	remainder = (remainder * 1000) / iterations;
	*p++ = digits[(remainder / 100) % 10];
	*p++ = digits[(remainder / 10) % 10];
	*p++ = digits[remainder % 10];
	APPEND_LITERAL("\nnet cycles/ecall: ");
	APPEND_ULONG(net_average);
	APPEND_LITERAL(".");
	net_remainder = (net_remainder * 1000) / iterations;
	*p++ = digits[(net_remainder / 100) % 10];
	*p++ = digits[(net_remainder / 10) % 10];
	*p++ = digits[net_remainder % 10];
	APPEND_LITERAL("\n");
	*p = '\0';

#undef APPEND_ULONG
#undef APPEND_LITERAL

	sbi_ecall_console_puts(output);
#else
	sbi_ecall_console_puts("\nTest payload running\n");
#endif

	while (1)
		wfi();
}
