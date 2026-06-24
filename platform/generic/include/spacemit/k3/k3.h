#ifndef __K2_FPGA_CONFIG_H__
#define __K2_FPGA_CONFIG_H__

/***************************cci******************************/
#define PLATFORM_CCI_ADDR           (0xD8500000)

#define PLAT_CCI_CLUSTER0_IFACE_IX  0
#define PLAT_CCI_CLUSTER1_IFACE_IX  1
#define PLAT_CCI_CLUSTER2_IFACE_IX  2
#define PLAT_CCI_CLUSTER3_IFACE_IX  3

#define PLAT_CCI_MAP static const int cci_map[] = { \
	PLAT_CCI_CLUSTER0_IFACE_IX,	\
	PLAT_CCI_CLUSTER1_IFACE_IX,	\
	PLAT_CCI_CLUSTER2_IFACE_IX,	\
	PLAT_CCI_CLUSTER3_IFACE_IX,	\
};

/***************************cpu******************************/
#define C0_RVBADDR_LO_ADDR          (0xD4282DB0)
#define C0_RVBADDR_HI_ADDR          (0xD4282DB4)

#define C1_RVBADDR_LO_ADDR          (0xD4282C00 + 0x2B0)
#define C1_RVBADDR_HI_ADDR          (0xD4282C00 + 0X2B4)

#define C2_RVBADDR_LO_ADDR          (0xD4282C00 + 0x3E8)
#define C2_RVBADDR_HI_ADDR          (0xD4282C00 + 0X3EC)

#define C3_RVBADDR_LO_ADDR          (0xD4282C00 + 0x260)
#define C3_RVBADDR_HI_ADDR          (0xD4282C00 + 0X264)

#define PMU_CAP_CORE0_WAKEUP		(0xD4282800 + 0x12C)
#define PMU_CAP_CORE1_WAKEUP		(0xD4282800 + 0x130)
#define PMU_CAP_CORE2_WAKEUP		(0xD4282800 + 0x134)
#define PMU_CAP_CORE3_WAKEUP		(0xD4282800 + 0x138)
#define PMU_CAP_CORE4_WAKEUP		(0xD4282800 + 0x324)
#define PMU_CAP_CORE5_WAKEUP		(0xD4282800 + 0x328)
#define PMU_CAP_CORE6_WAKEUP		(0xD4282800 + 0x32C)
#define PMU_CAP_CORE7_WAKEUP		(0xD4282800 + 0x330)
#define PMU_CAP_CORE8_WAKEUP		(0xD4282800 + 0x360)
#define PMU_CAP_CORE9_WAKEUP		(0xD4282800 + 0x364)
#define PMU_CAP_CORE10_WAKEUP		(0xD4282800 + 0x368)
#define PMU_CAP_CORE11_WAKEUP		(0xD4282800 + 0x36C)
#define PMU_CAP_CORE12_WAKEUP		(0xD4282800 + 0x22C)
#define PMU_CAP_CORE13_WAKEUP		(0xD4282800 + 0x230)
#define PMU_CAP_CORE14_WAKEUP		(0xD4282800 + 0x234)
#define PMU_CAP_CORE15_WAKEUP		(0xD4282800 + 0x238)

#define PMU_CAP_CORE0_IDLE_CFG		(0xD4282800 + 0x124)
#define PMU_CAP_CORE1_IDLE_CFG		(0xD4282800 + 0x128)
#define PMU_CAP_CORE2_IDLE_CFG		(0xD4282800 + 0x160)
#define PMU_CAP_CORE3_IDLE_CFG		(0xD4282800 + 0x164)
#define PMU_CAP_CORE4_IDLE_CFG		(0xD4282800 + 0x304)
#define PMU_CAP_CORE5_IDLE_CFG		(0xD4282800 + 0x308)
#define PMU_CAP_CORE6_IDLE_CFG		(0xD4282800 + 0x30c)
#define PMU_CAP_CORE7_IDLE_CFG		(0xD4282800 + 0x310)
#define PMU_CAP_CORE8_IDLE_CFG		(0xD4282800 + 0x340)
#define PMU_CAP_CORE9_IDLE_CFG		(0xD4282800 + 0x344)
#define PMU_CAP_CORE10_IDLE_CFG		(0xD4282800 + 0x348)
#define PMU_CAP_CORE11_IDLE_CFG		(0xD4282800 + 0x34c)
#define PMU_CAP_CORE12_IDLE_CFG		(0xD4282800 + 0x20c)
#define PMU_CAP_CORE13_IDLE_CFG		(0xD4282800 + 0x210)
#define PMU_CAP_CORE14_IDLE_CFG		(0xD4282800 + 0x214)
#define PMU_CAP_CORE15_IDLE_CFG		(0xD4282800 + 0x218)

#define PMU_CX_CAPMP_IDLE_CFG0		(0xd4282800 + 0x120)
#define PMU_CX_CAPMP_IDLE_CFG1		(0xd4282800 + 0xe4)
#define PMU_CX_CAPMP_IDLE_CFG2		(0xd4282800 + 0x150)
#define PMU_CX_CAPMP_IDLE_CFG3		(0xd4282800 + 0x154)
#define PMU_CX_CAPMP_IDLE_CFG4		(0xd4282800 + 0x314)
#define PMU_CX_CAPMP_IDLE_CFG5		(0xd4282800 + 0x318)
#define PMU_CX_CAPMP_IDLE_CFG6		(0xd4282800 + 0x31c)
#define PMU_CX_CAPMP_IDLE_CFG7		(0xd4282800 + 0x320)
#define PMU_CX_CAPMP_IDLE_CFG8		(0xd4282800 + 0x350)
#define PMU_CX_CAPMP_IDLE_CFG9		(0xd4282800 + 0x354)
#define PMU_CX_CAPMP_IDLE_CFG10		(0xd4282800 + 0x358)
#define PMU_CX_CAPMP_IDLE_CFG11		(0xd4282800 + 0x35c)
#define PMU_CX_CAPMP_IDLE_CFG12		(0xd4282800 + 0x21c)
#define PMU_CX_CAPMP_IDLE_CFG13		(0xd4282800 + 0x220)
#define PMU_CX_CAPMP_IDLE_CFG14		(0xd4282800 + 0x224)
#define PMU_CX_CAPMP_IDLE_CFG15		(0xd4282800 + 0x228)

#define APCR_CORE0_VETE_REG		(0xd4050000 + 0x10c0)
#define APCR_CORE1_VETE_REG		(0xd4050000 + 0x10c4)
#define APCR_CORE2_VETE_REG		(0xd4050000 + 0x10c8)
#define APCR_CORE3_VETE_REG		(0xd4050000 + 0x10cc)
#define APCR_CORE4_VETE_REG		(0xd4050000 + 0x10d0)
#define APCR_CORE5_VETE_REG		(0xd4050000 + 0x10d4)
#define APCR_CORE6_VETE_REG		(0xd4050000 + 0x10d8)
#define APCR_CORE7_VETE_REG		(0xd4050000 + 0x10dc)
#define APCR_CORE8_VETE_REG		(0xd4050000 + 0x10e0)
#define APCR_CORE9_VETE_REG		(0xd4050000 + 0x10e4)
#define APCR_CORE10_VETE_REG		(0xd4050000 + 0x10e8)
#define APCR_CORE11_VETE_REG		(0xd4050000 + 0x10ec)
#define APCR_CORE12_VETE_REG		(0xd4050000 + 0x10f0)
#define APCR_CORE13_VETE_REG		(0xd4050000 + 0x10f4)
#define APCR_CORE14_VETE_REG		(0xd4050000 + 0x10f8)
#define APCR_CORE15_VETE_REG		(0xd4050000 + 0x10fc)
#define APCR_PER_VETE_REG		(0xd4050000 + 0x1098)

#define APCR_COREX_DEFAULT_VATE_VALUE	((1 << 3) | (1 << 13) | (1 << 14) | (1 << 19) | (1 << 25) | (1 << 26) | (1 << 27) | (1 << 29) | (1 << 31))

#define PMU_C0_L2_FLUSH_CTRL		(0xd8440000 + 0x1b0)
#define PMU_C1_L2_FLUSH_CTRL		(0xd8440000 + 0x1b4)
#define PMU_C2_L2_FLUSH_CTRL		(0xd8440000 + 0x1c4)
#define PMU_C3_L2_FLUSH_CTRL		(0xd8440000 + 0x1ec)

#define RCPU0_RUNTIME_SPACE_BASE_ADDR	(0x100200000UL)
#define RCPU0_RUNTIME_SPACE_SIZE	(0x400000UL)
#define RCPU1_RUNTIME_SPACE_BASE_ADDR	(0x100800000UL)
#define RCPU1_RUNTIME_SPACE_SIZE	(0x400000UL)
#define RCPU_DTB_SPACE_BASE_ADDR	(0x100d00000UL)
#define RCPU_DTB_SPACEMI_SIZE		(0x100000UL)
#define REGISTER_PRESERVATION_BASE	(0xd4282000)
#define REGISTER_PRESERVATION_SIZE	(0x1000)

#define RCPU0_MEMRANGE_GRAN		(0x200000UL)
#define RCPU1_MEMRANGE_GRAN		(0x400000UL)
#define RCPU_DTB_MEMRANGE_GRAN		(0x100000UL)

#define DMASYS_RESET			(0xd8440000 + 0x22c)
#define DMASYS_CLK_EN			(0xd8440000 + 0x234)

#define PMU_L2_FLUSH_HW_TYPE		(1 << 0)
#define PMU_L2_FLUSH_HW_EN		(1 << 2)

#define CPU_MASK_FI_INTTERUPT		((1 << 3) | (1 << 4))
#define CPU_PWR_DOWN_VALUE		(0x1f)
#define CLUSTER_PWR_DOWN_VALUE		(0x8f)

/* this is the max cluster count of this platform */
#define PLATFORM_CLUSTER_COUNT		(4U)
/* this is the max cpu cores per cluster*/
#define PLATFORM_MAX_CPUS_PER_CLUSTER	(4U)

/***************************cpu affin*************************/
#define MPIDR_AFFINITY0_MASK		0x3U
#define MPIDR_AFFINITY1_MASK		0xfU
#define MPIDR_AFF0_SHIFT		0U
#define MPIDR_AFF1_SHIFT		2U

#define MAILBOX_DOORBALL_TRIGGER_OFFSET	0x40
#define MAILBOX_INT_EN_REG_OFFSET	0x118

#define MAILBOX_DOORBALL_TRIGGER_OFFSET	0x40
#define MAILBOX_INT_EN_REG_OFFSET	0x118

/***************************imsic config**********************/
#define MAX_IMSIC_EIE_REGISTERS		64
#define IMSIC_FIRST_EIE_REG		0xc0
#define IMSIC_EIDELIVERY		0x70
#define IMSIC_EITHRESHOLD		0x72
#define IMSIC_MAX_VGEN			0x8

struct himsic_config {
	/* h-mode */
	unsigned long long heidelivery;
	unsigned long long heithreshold;
	unsigned long long heie[MAX_IMSIC_EIE_REGISTERS];
};

struct imsic_config {
	unsigned int flags;
	/* m-mode */
	unsigned long long meidelivery;
	unsigned long long meithreshold;
	unsigned long long meie[MAX_IMSIC_EIE_REGISTERS];
	/* s-mode */
	unsigned long long seidelivery;
	unsigned long long seithreshold;
	unsigned long long seie[MAX_IMSIC_EIE_REGISTERS];
	/* h-mode */
	unsigned long long hstatus;
	unsigned long long hedeleg;
	unsigned long long hideleg;
	unsigned long long hie;
	unsigned long long hcounteren;
	unsigned long long hgeie;
	unsigned long long henvcfg;
	unsigned long long henvcfgh;
	unsigned long long htval;
	unsigned long long hgatp;
	unsigned long long htimedelta;
	unsigned long long htimedeltah;
	struct himsic_config hc[IMSIC_MAX_VGEN];
};

#endif /* __K2_PLATFORM_CONFIG_H__ */
