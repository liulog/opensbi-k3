# OpenSBI S-mode ECALL 延迟基准测试

本测试用于测量通过 `ecall` 从 S-mode 进入 M-mode，再通过 `mret` 返回
S-mode 的开销。测试不经过 OpenSBI 常规的陷阱上下文保存流程和 C 语言
ECALL 分发路径。

上游 OpenSBI 的项目说明已原样保存在
[`README.orig.md`](README.orig.md)。本 `README.md` 专门说明
`k3-ecall-latency-bench` 分支中的基准测试修改。

## 主要特点

- OpenSBI 的常规陷阱入口和 SBI ECALL 分发器保持不变。
- 所有基准测试代码仅在启用 `CONFIG_SBI_ECALL_BENCH` 时参与编译。
- M-mode 临时允许 S-mode 读取并使用 cycle 计数器，屏蔽 M-mode 中断源，
  并安装专用的最小陷阱向量。
- S-mode 在两次 `cycle` 读取之间连续执行 1,024 条 ECALL 指令。相邻
  ECALL 之间没有循环控制、分支或其他 S-mode 指令。
- 同一套代码通过不同的 defconfig 支持 QEMU `virt` 和 SpacemiT K3。
  普通配置不会包含基准测试目标文件或运行时挂钩。

## 相对 k3-br-v1.0.y 的修改

原始顶层文档保存在 `README.orig.md`，OpenSBI 的常规陷阱入口保持不变。
基准测试通过以下相互隔离的修改实现：

- `CONFIG_SBI_ECALL_BENCH` 控制所有仅用于测试的 M-mode 和 S-mode 代码。
- `lib/sbi/sbi_ecall_bench.S` 提供临时、精简的 M-mode ECALL 返回路径，
  并在测试结束时恢复原有机器状态。
- `firmware/payloads/ecall_bench.S` 包含 S-mode 测量代码块。
- `ecall_bench_defconfig` 和 `k3_ecall_bench_defconfig` 分别选择 QEMU 与
  K3 测试镜像；原有的 `k3_defconfig` 不做修改。
- `scripts/ecall-bench.sh` 提供可重复的目录外编译流程，并可选择直接
  启动 QEMU。

两个原有的 K3 专用 CSR/缓存引用由 `CONFIG_PLATFORM_SPACEMIT_K3`
保护，因此同一个基准测试 payload 可以为通用 QEMU 编译，同时不会改变
K3 原有行为。

这是一个有意与正常固件隔离的测试配置：

- OpenSBI 正常完成冷启动初始化和 PMP 配置。
- 在进入内置 S-mode payload 之前，OpenSBI 保存原有的 `mtvec`，关闭
  M-mode 中断源，允许 S-mode 访问 `cycle` 计数器，清除
  `mcountinhibit.CY`，然后安装临时的最小陷阱向量。
- S-mode payload 首先在完全相同的被测代码块上预热一次，随后测量
  800 批、每批 1,024 次连续的 ECALL 往返。
- 每个被测批次都是一个无分支的 4 KiB 指令块，其中只包含连续 ECALL
  指令。计数器读取、结果累加和外层循环分支都位于该代码块之外。
- 最后一次不计时的 ECALL 恢复原有 `mtvec`、`mie` 和计数器状态。
  随后 payload 通过常规 SBI 调试控制台扩展输出结果。

M-mode 快速路径只包含一次针对 `a6` 的分支、`mepc` 的读取和更新、
将 `a0` 与 `a1` 清零，以及 `mret`。该路径有意假设测试运行在受控的
单 hart payload 中，并且不会发生故障。启动普通 payload 或操作系统时
不要启用此选项。

输出的 `cycles/ecall` 是完整 ECALL 往返开销的平均值。S-mode 中的
`rdcycle` 指令和排序屏障只位于每批测试的边界，其开销由 1,024 次连续
ECALL 共同摊销。

## M-mode 处理路径

进入 S-mode 之前，`sbi_ecall_bench_prepare` 保存 `mtvec`、`mie`、
`mcounteren` 和 `mcountinhibit`，随后执行以下准备工作：

- 向 `mie` 写入零，避免 M-mode 中断进入测试专用陷阱向量；
- 设置 `mcounteren.CY`，允许 S-mode 执行 `rdcycle`；
- 清除 `mcountinhibit.CY`，确保 cycle 计数器正在运行；
- 将 `mtvec` 指向 `sbi_ecall_bench_trap`。

每一条被测 S-mode ECALL 都会进入
`lib/sbi/sbi_ecall_bench.S` 中的以下 M-mode 热路径：

```asm
sbi_ecall_bench_trap:
	bnez	a6, .Lbench_stop
	csrr	a0, CSR_MEPC
	addi	a0, a0, 4
	csrw	CSR_MEPC, a0
	li	a0, 0
	li	a1, 0
	mret
```

被测路径中只有以上 7 条 M-mode 指令。对于用于测量的 ECALL，S-mode
始终保持 `a6` 为零，因此第一条分支不会跳转。接下来的 3 条指令将
`mepc` 前移 4 字节，从而跳过当前 ECALL 指令；随后将 `a0` 和 `a1`
清零，作为最小的成功返回值；最后由 `mret` 返回紧邻的下一条 S-mode
ECALL。整个过程不使用栈，不保存通用寄存器上下文，也不经过 C 分发器、
扩展查找或常规 SBI 处理函数。

所有批次测量结束后，S-mode 使用非零 `a6` 发出一次不计时的停止
ECALL。停止路径恢复此前保存的 `mtvec`、`mcounteren`、
`mcountinhibit` 和 `mie`，更新 `mepc` 后返回 S-mode。该恢复路径较长，
但完全位于 cycle 测量区间之外。

## 使用脚本编译

以下命令均在仓库根目录执行。首先可以查看脚本支持的命令和参数：

```sh
./scripts/ecall-bench.sh --help
```

只编译 QEMU 镜像，不启动 QEMU：

```sh
CROSS_COMPILE=riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench.sh qemu-build
```

编译并立即在 QEMU 中运行：

```sh
CROSS_COMPILE=riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench.sh qemu
```

如果编译器不在 `PATH` 中，可以使用完整的工具链前缀。本仓库已经使用
以下工具链完成验证：

```sh
CROSS_COMPILE=/opt/spacemit-toolchain-linux-glibc-x86_64-v1.2.4/bin/riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench.sh qemu-build
```

将 K3 镜像编译到默认的 `build/ecall-bench/k3` 目录：

```sh
CROSS_COMPILE=riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench.sh k3-build
```

参考原有 `k3_defconfig` 和 `scripts/build.sh`，标准 K3 构建通常不需要
指定 `FW_TEXT_START`：OpenSBI 会根据实际加载位置完成运行时重定位。
`--fw-text-start` 主要用于让 ELF 调试符号与已知加载地址一致，或适配
要求固定 ELF 链接地址的特殊加载流程。需要时可以传入对应地址，并可
选择自定义输出目录：

```sh
CROSS_COMPILE=riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench.sh k3-build \
  --fw-text-start 0x80000000 \
  --output build/ecall-bench/k3-0x80000000
```

上面的 `0x80000000` 仅用于展示命令格式，并不是 K3 的默认固定地址。
实际使用时必须换成当前 K3 启动流程的真实加载地址。未指定
`FW_TEXT_START` 时，链接基地址为零；OpenSBI 会在早期启动过程中计算
运行时加载偏移，并应用相对重定位。

默认生成的镜像位于：

```text
build/ecall-bench/qemu/platform/generic/firmware/fw_payload.bin
build/ecall-bench/k3/platform/generic/firmware/fw_payload.bin
```

脚本会在调用 `make` 前创建输出目录，因此生成的 Kconfig 路径始终位于
该目录下，不会意外解析成 `/platform`。

## 直接使用 make 编译

编译 RV64 QEMU `virt` 镜像：

```sh
make PLATFORM=generic \
  PLATFORM_DEFCONFIG=ecall_bench_defconfig \
  CROSS_COMPILE=riscv64-linux-gnu-
```

编译 SpacemiT K3 测试镜像：

```sh
make PLATFORM=generic \
  PLATFORM_DEFCONFIG=k3_ecall_bench_defconfig \
  CROSS_COMPILE=riscv64-linux-gnu-
```

未显式指定 `O=...` 时，镜像生成在：

```text
build/platform/generic/firmware/fw_payload.bin
```

## 链接与加载地址

两个 `ecall_bench.S` 文件都没有各自独立的链接脚本：

- `lib/sbi/sbi_ecall_bench.S` 是 OpenSBI 的一部分，由
  `firmware/fw_payload.elf.ldS` 完成链接。当运行地址与
  `FW_TEXT_START` 不同时，它遵循 OpenSBI 常规的 PIE 重定位流程。
- `firmware/payloads/ecall_bench.S` 是内置 S-mode 测试 payload 的一部分，
  由 `firmware/payloads/test.elf.ldS` 链接到
  `FW_TEXT_START + FW_PAYLOAD_OFFSET`。

通用 RV64 平台的 payload 偏移为 `0x200000`。payload 被嵌入
`fw_payload.bin`，`fw_next_addr()` 通过 PC 相对符号引用取得其运行时
地址。payload 使用 `medany` 代码模型编译，并且不包含动态重定位，因此
QEMU 和硬件板可以将完整的 `fw_payload.bin` 加载到不同基地址。加载时
必须保持完整镜像内部布局；除非同时提供匹配的下一阶段入口地址，否则
不要只提取并单独移动 `test.bin`。

## 在 QEMU 上运行

使用单 hart，保证只有冷启动 hart 进入基准测试 payload：

```sh
qemu-system-riscv64 -M virt -m 256M -smp 1 -nographic \
  -bios build/platform/generic/firmware/fw_payload.bin
```

payload 会输出测得的 cycle 总数和每次 ECALL 的平均 cycle 数，随后进入
`wfi` 等待；此时需要手动退出 QEMU。

QEMU 的 cycle 数适合用于功能验证和指令路径确认；实际硬件延迟应以
目标 K3 板上的测量结果为准。
