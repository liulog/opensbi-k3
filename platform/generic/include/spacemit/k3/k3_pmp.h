#ifndef __PMP_H__
#define __PMP_H__

#include <sbi/riscv_encoding.h>
#include <sbi/sbi_types.h>


typedef struct
{
	uint64_t start_addr;
	uint64_t end_addr;
	uint8_t region_attr;
	uint8_t region_enable;
} PmpRegion, *pPmpRegion;

#define __riscv64__
typedef enum {
	REGION_SIZE_4B = -1,
	REGION_SIZE_8B = 0,
	REGION_SIZE_16B = 1,
	REGION_SIZE_32B = 2,
	REGION_SIZE_64B = 3,
	REGION_SIZE_128B = 4,
	REGION_SIZE_256B = 5,
	REGION_SIZE_512B = 6,
	REGION_SIZE_1KB = 7,
	REGION_SIZE_2KB = 8,
	REGION_SIZE_4KB = 9,
	REGION_SIZE_8KB = 10,
	REGION_SIZE_16KB = 11,
	REGION_SIZE_32KB = 12,
	REGION_SIZE_64KB = 13,
	REGION_SIZE_128KB = 14,
	REGION_SIZE_256KB = 15,
	REGION_SIZE_512KB = 16,
	REGION_SIZE_1MB = 17,
	REGION_SIZE_2MB = 18,
	REGION_SIZE_4MB = 19,
	REGION_SIZE_8MB = 20,
	REGION_SIZE_16MB = 21,
	REGION_SIZE_32MB = 22,
	REGION_SIZE_64MB = 23,
	REGION_SIZE_128MB = 24,
	REGION_SIZE_256MB = 25,
	REGION_SIZE_512MB = 26,
	REGION_SIZE_1GB = 27,
	REGION_SIZE_2GB = 28,
	REGION_SIZE_4GB = 29,
	REGION_SIZE_8GB = 30,
	REGION_SIZE_16GB = 31,
	REGION_SIZE_32GB = 32,
	REGION_SIZE_64GB = 33,
	REGION_SIZE_128GB = 34,
	REGION_SIZE_256GB = 35,
	REGION_SIZE_512GB = 36,
	REGION_SIZE_1TB = 37
} region_size_e;

typedef enum {
	ADDRESS_MATCHING_TOR = 1,
	ADDRESS_MATCHING_NAPOT = 3
} address_matching_e;

typedef struct {
	uint32_t r : 1; /* readable enable */
	uint32_t w : 1; /* writeable enable */
	uint32_t x : 1; /* execable enable */
	address_matching_e a : 2; /* address matching mode */
	uint32_t reserved : 2; /* reserved */
	uint32_t l : 1; /* lock enable */
} mpu_region_attr_t;

#define SIZE_4B 0x4 // 4B
#define SIZE_8B 0x8 // 8B
#define SIZE_16B 0x10 // 16B
#define SIZE_32B 0x20 // 32B
#define SIZE_64B 0x40 // 64B
#define SIZE_128B 0x80 // 128B
#define SIZE_256B 0x100 // 256B
#define SIZE_512B 0x200 // 512B
#define SIZE_1KB 0x400 // 1KB
#define SIZE_2KB 0x800 // 2KB
#define SIZE_4KB 0x1000 // 4KB
#define SIZE_8KB 0x2000 // 8KB
#define SIZE_16KB 0x4000 // 16KB
#define SIZE_32KB 0x8000 // 32KB
#define SIZE_64KB 0x10000 // 64KB
#define SIZE_128KB 0x20000 // 128KB
#define SIZE_256KB 0x40000 // 256KB
#define SIZE_512KB 0x80000 // 512KB
#define SIZE_1MB 0x100000 // 1MB
#define SIZE_2MB 0x200000 // 2MB
#define SIZE_3MB 0x300000 // 3MB
#define SIZE_4MB 0x400000 // 4MB
#define SIZE_7MB 0x700000 // 7MB
#define SIZE_8MB 0x800000 // 8MB
#define SIZE_16MB 0x1000000 // 16MB
#define SIZE_32MB 0x2000000 // 32MB
#define SIZE_64MB 0x4000000 // 64MB
#define SIZE_128MB 0x8000000 // 128MB
#define SIZE_256MB 0x10000000 // 256MB
#define SIZE_512MB 0x20000000 // 512MB
#define SIZE_1GB 0x40000000 // 1GB
#define SIZE_2GB 0x80000000 // 2GB
#ifdef __riscv64__
#define SIZE_4GB 0x100000000 // 4GB
#define SIZE_8GB 0x200000000 // 8GB
#define SIZE_16GB 0x400000000 // 16GB
#define SIZE_32GB 0x800000000 // 32GB
#define SIZE_64GB 0x1000000000 // 64GB
#define SIZE_128GB 0x2000000000 // 128GB
#define SIZE_256GB 0x4000000000 // 256GB
#define SIZE_512GB 0x8000000000 // 512GB
#define SIZE_1TB 0x10000000000 // 1TB
#endif

#define PMP_ENTRY_NUM 32
#define PMP_PMPCFG_R_Pos 0U /*!< PMP PMPCFG: R Position */
#define PMP_PMPCFG_R_Msk (0x1UL << PMP_PMPCFG_R_Pos) /*!< PMP PMPCFG: R Mask */

#define PMP_PMPCFG_W_Pos 1U /*!< PMP PMPCFG: W Position */
#define PMP_PMPCFG_W_Msk (0x1UL << PMP_PMPCFG_W_Pos) /*!< PMP PMPCFG: W Mask */

#define PMP_PMPCFG_X_Pos 2U /*!< PMP PMPCFG: X Position */
#define PMP_PMPCFG_X_Msk (0x1UL << PMP_PMPCFG_X_Pos) /*!< PMP PMPCFG: X Mask */

#define PMP_PMPCFG_A_Pos 3U /*!< PMP PMPCFG: A Position */
#define PMP_PMPCFG_A_Msk (0x3UL << PMP_PMPCFG_A_Pos) /*!< PMP PMPCFG: A Mask */

#define PMP_PMPCFG_L_Pos 7U /*!< PMP PMPCFG: L Position */
#define PMP_PMPCFG_L_Msk (0x1UL << PMP_PMPCFG_L_Pos) /*!< PMP PMPCFG: L Mask */

typedef enum {
	RWX,
	RWnX,
	ROX,
	ROnX,
	nRnWnX,
} pmp_attribute;

static inline void __set_PMPADDR0(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr0, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR1(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr1, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR2(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr2, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR3(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr3, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR4(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr4, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR5(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr5, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR6(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr6, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR7(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr7, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR8(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr8, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR9(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr9, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR10(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr10, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR11(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr11, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR12(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr12, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR13(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr13, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR14(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr14, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR15(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr15, %0" : : "r"(pmpaddr));
}

#if PMP_ENTRY_NUM > 16
static inline void __set_PMPADDR16(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr16, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR17(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr17, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR18(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr18, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR19(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr19, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR20(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr20, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR21(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr21, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR22(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr22, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR23(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr23, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR24(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr24, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR25(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr25, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR26(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr26, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR27(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr27, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR28(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr28, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR29(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr29, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR30(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr30, %0" : : "r"(pmpaddr));
}

static inline void __set_PMPADDR31(uint64_t pmpaddr)
{
	asm volatile("csrw pmpaddr31, %0" : : "r"(pmpaddr));
}
#endif

/*
   \brief   Set PMPADDRx by index
   \details Writes the given value to the PMPADDRx Register.
   \param [in]    idx      PMP region index
   \param [in]    pmpaddr  PMPADDRx Register value to set
   */
static inline void __set_PMPADDRx(uint64_t idx, uint64_t pmpaddr)
{
	switch (idx) {
	case 0:
		__set_PMPADDR0(pmpaddr);
		break;
	case 1:
		__set_PMPADDR1(pmpaddr);
		break;
	case 2:
		__set_PMPADDR2(pmpaddr);
		break;
	case 3:
		__set_PMPADDR3(pmpaddr);
		break;
	case 4:
		__set_PMPADDR4(pmpaddr);
		break;
	case 5:
		__set_PMPADDR5(pmpaddr);
		break;
	case 6:
		__set_PMPADDR6(pmpaddr);
		break;
	case 7:
		__set_PMPADDR7(pmpaddr);
		break;
	case 8:
		__set_PMPADDR8(pmpaddr);
		break;
	case 9:
		__set_PMPADDR9(pmpaddr);
		break;
	case 10:
		__set_PMPADDR10(pmpaddr);
		break;
	case 11:
		__set_PMPADDR11(pmpaddr);
		break;
	case 12:
		__set_PMPADDR12(pmpaddr);
		break;
	case 13:
		__set_PMPADDR13(pmpaddr);
		break;
	case 14:
		__set_PMPADDR14(pmpaddr);
		break;
	case 15:
		__set_PMPADDR15(pmpaddr);
		break;
#if PMP_ENTRY_NUM > 16
	case 16:
		__set_PMPADDR16(pmpaddr);
		break;
	case 17:
		__set_PMPADDR17(pmpaddr);
		break;
	case 18:
		__set_PMPADDR18(pmpaddr);
		break;
	case 19:
		__set_PMPADDR19(pmpaddr);
		break;
	case 20:
		__set_PMPADDR20(pmpaddr);
		break;
	case 21:
		__set_PMPADDR21(pmpaddr);
		break;
	case 22:
		__set_PMPADDR22(pmpaddr);
		break;
	case 23:
		__set_PMPADDR23(pmpaddr);
		break;
	case 24:
		__set_PMPADDR24(pmpaddr);
		break;
	case 25:
		__set_PMPADDR25(pmpaddr);
		break;
	case 26:
		__set_PMPADDR26(pmpaddr);
		break;
	case 27:
		__set_PMPADDR27(pmpaddr);
		break;
	case 28:
		__set_PMPADDR28(pmpaddr);
		break;
	case 29:
		__set_PMPADDR29(pmpaddr);
		break;
	case 30:
		__set_PMPADDR30(pmpaddr);
		break;
	case 31:
		__set_PMPADDR31(pmpaddr);
		break;
#endif
	default:
		return;
	}
}

/**
  \brief   Set PMPCFGx
  \details Writes the given value to the PMPCFGx Register.
  \param [in]    pmpcfg  PMPCFGx Register value to set
  */
static inline void __set_PMPCFG0(uint64_t pmpcfg)
{
	asm volatile("csrw pmpcfg0, %0" : : "r"(pmpcfg));
}

static inline void __set_PMPCFG1(uint64_t pmpcfg)
{
	asm volatile("csrw pmpcfg1, %0" : : "r"(pmpcfg));
}

static inline void __set_PMPCFG2(uint64_t pmpcfg)
{
	asm volatile("csrw pmpcfg2, %0" : : "r"(pmpcfg));
}

static inline void __set_PMPCFG3(uint64_t pmpcfg)
{
	asm volatile("csrw pmpcfg3, %0" : : "r"(pmpcfg));
}

static inline void __set_PMPCFG4(uint64_t pmpcfg)
{
	asm volatile("csrw pmpcfg4, %0" : : "r"(pmpcfg));
}

static inline void __set_PMPCFG5(uint64_t pmpcfg)
{
	asm volatile("csrw pmpcfg5, %0" : : "r"(pmpcfg));
}

static inline void __set_PMPCFG6(uint64_t pmpcfg)
{
	asm volatile("csrw pmpcfg6, %0" : : "r"(pmpcfg));
}

/**
  \brief   Get PMPCFGx Register
  \details Returns the content of the PMPCFGx Register.
  \return               PMPCFGx Register value
  */
static inline uint64_t __get_PMPCFG0(void)
{
	uint64_t result;

	asm volatile("csrr %0, pmpcfg0" : "=r"(result));
	return (result);
}

static inline uint64_t __get_PMPCFG1(void)
{
	uint64_t result;

	asm volatile("csrr %0, pmpcfg1" : "=r"(result));
	return (result);
}

static inline uint64_t __get_PMPCFG2(void)
{
	uint64_t result;

	asm volatile("csrr %0, pmpcfg2" : "=r"(result));
	return (result);
}

static inline uint64_t __get_PMPCFG3(void)
{
	uint64_t result;

	asm volatile("csrr %0, pmpcfg3" : "=r"(result));
	return (result);
}

#if PMP_ENTRY_NUM > 16
static inline uint64_t __get_PMPCFG4(void)
{
	uint64_t result;

	asm volatile("csrr %0, pmpcfg4" : "=r"(result));
	return (result);
}

static inline uint64_t __get_PMPCFG5(void)
{
	uint64_t result;

	asm volatile("csrr %0, pmpcfg5" : "=r"(result));
	return (result);
}

static inline uint64_t __get_PMPCFG6(void)
{
	uint64_t result;

	asm volatile("csrr %0, pmpcfg6" : "=r"(result));
	return (result);
}
#endif

/**
  \brief   Set PMPxCFG by index
  \details Writes the given value to the PMPxCFG Register.
  \param [in]    idx      PMPx region index
  \param [in]    pmpxcfg  PMPxCFG Register value to set
  */
static inline void __set_PMPxCFG(uint64_t idx, uint8_t pmpxcfg)
{
	uint64_t pmpcfgx = 0;

#if __RISCV_XLEN == 32
	if (idx < 4) {
		pmpcfgx = __get_PMPCFG0();
		pmpcfgx = (pmpcfgx & ~(0xFF << (idx << 3))) | (pmpxcfg << (idx << 3));
		__set_PMPCFG0(pmpcfgx);
	} else if (idx >= 4 && idx < 8) {
		idx -= 4;
		pmpcfgx = __get_PMPCFG1();
		pmpcfgx = (pmpcfgx & ~(0xFF << (idx << 3))) | (pmpxcfg << (idx << 3));
		__set_PMPCFG1(pmpcfgx);
	} else if (idx >= 8 && idx < 12) {
		idx -= 8;
		pmpcfgx = __get_PMPCFG2();
		pmpcfgx = (pmpcfgx & ~(0xFF << (idx << 3))) | (pmpxcfg << (idx << 3));
		__set_PMPCFG2(pmpcfgx);
	} else if (idx >= 12 && idx < 16) {
		idx -= 12;
		pmpcfgx = __get_PMPCFG3();
		pmpcfgx = (pmpcfgx & ~(0xFF << (idx << 3))) | (pmpxcfg << (idx << 3));
		__set_PMPCFG3(pmpcfgx);
	} else {
		return;
	}
#else
	if (idx < 8) {
		pmpcfgx = __get_PMPCFG0();
		pmpcfgx = (pmpcfgx & ~(0xFF << (idx << 3))) | (pmpxcfg << (idx << 3));
		__set_PMPCFG0(pmpcfgx);
	} else if (idx >= 8 && idx < 16) {
		idx -= 8;
		pmpcfgx = __get_PMPCFG2();
		pmpcfgx = (pmpcfgx & ~(0xFF << (idx << 3))) | (pmpxcfg << (idx << 3));
		__set_PMPCFG2(pmpcfgx);
	}
#if PMP_ENTRY_NUM > 16
	else if (idx >= 16 && idx < 24) {
		idx -= 16;
		pmpcfgx = __get_PMPCFG4();
		pmpcfgx = (pmpcfgx & ~(0xFF << (idx << 3))) | (pmpxcfg << (idx << 3));
		__set_PMPCFG4(pmpcfgx);
	} else {
		idx -= 24;
		pmpcfgx = __get_PMPCFG6();
		pmpcfgx = (pmpcfgx & ~(0xFF << (idx << 3))) | (pmpxcfg << (idx << 3));
		__set_PMPCFG6(pmpcfgx);
	}
#endif
#endif
}

PmpRegion pmp_setting[] = {
	// start_addr  end_addr  region_attr  region_enable
	{ 0x00000000, 0x0003FFFFFF, RWnX, 1 },
	{ 0x04000000, 0x00FFFFFFFF, RWX, 1 },
	{ 0x100000000, 0x3FFFFFFFFF, RWX, 1 },
};

typedef struct {
	s8 region_size_idx;
	uint64_t region_size;
} napot_table_entry;

napot_table_entry napot_table[] = {
	{ REGION_SIZE_4B, SIZE_4B },
	{ REGION_SIZE_8B, SIZE_8B },
	{ REGION_SIZE_16B, SIZE_16B },
	{ REGION_SIZE_32B, SIZE_32B },
	{ REGION_SIZE_64B, SIZE_64B },
	{ REGION_SIZE_128B, SIZE_128B },
	{ REGION_SIZE_256B, SIZE_256B },
	{ REGION_SIZE_512B, SIZE_512B },
	{ REGION_SIZE_1KB, SIZE_1KB },
	{ REGION_SIZE_2KB, SIZE_2KB },
	{ REGION_SIZE_4KB, SIZE_4KB },
	{ REGION_SIZE_8KB, SIZE_8KB },
	{ REGION_SIZE_16KB, SIZE_16KB },
	{ REGION_SIZE_32KB, SIZE_32KB },
	{ REGION_SIZE_64KB, SIZE_64KB },
	{ REGION_SIZE_128KB, SIZE_128KB },
	{ REGION_SIZE_256KB, SIZE_256KB },
	{ REGION_SIZE_512KB, SIZE_512KB },
	{ REGION_SIZE_1MB, SIZE_1MB },
	{ REGION_SIZE_2MB, SIZE_2MB },
	{ REGION_SIZE_4MB, SIZE_4MB },
	{ REGION_SIZE_8MB, SIZE_8MB },
	{ REGION_SIZE_16MB, SIZE_16MB },
	{ REGION_SIZE_32MB, SIZE_32MB },
	{ REGION_SIZE_64MB, SIZE_64MB },
	{ REGION_SIZE_128MB, SIZE_128MB },
	{ REGION_SIZE_256MB, SIZE_256MB },
	{ REGION_SIZE_512MB, SIZE_512MB },
	{ REGION_SIZE_1GB, SIZE_1GB },
	{ REGION_SIZE_2GB, SIZE_2GB },
#ifdef __riscv64__
	{ REGION_SIZE_4GB, SIZE_4GB },
	{ REGION_SIZE_8GB, SIZE_8GB },
	{ REGION_SIZE_16GB, SIZE_16GB },
	{ REGION_SIZE_32GB, SIZE_32GB },
	{ REGION_SIZE_64GB, SIZE_64GB },
	{ REGION_SIZE_128GB, SIZE_128GB },
	{ REGION_SIZE_256GB, SIZE_256GB },
	{ REGION_SIZE_512GB, SIZE_512GB },
	{ REGION_SIZE_1TB, SIZE_1TB },
#endif
};

#define INVALID_REGION_SIZE_ID -2
int32_t get_region_size_id(uint64_t region_size)
{
	int i;
	for (i = 0; i < sizeof(napot_table) / sizeof(napot_table_entry); i++) {
		if (region_size == napot_table[i].region_size)
			return napot_table[i].region_size_idx;
	}
	return INVALID_REGION_SIZE_ID;
}

#endif
