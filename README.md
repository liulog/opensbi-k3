# OpenSBI S-mode ECALL 延迟基准测试

本测试用于测量通过 `ecall` 从 S-mode 进入 M-mode，再通过 `mret` 返回
S-mode 的开销。它依次测量专用 minimal mtvec、原陷阱入口早返回、完整
`get_spec_version` 和不支持扩展四条路径。只有 minimal mtvec 不经过
OpenSBI 常规的陷阱上下文保存流程和 C 语言 ECALL 分发路径。

上游 OpenSBI 的项目说明已原样保存在
[`README.orig.md`](README.orig.md)。本 `README.md` 专门说明
`k3-br-v1.0.0-ecall-bench` 分支中的基准测试修改。

## 主要特点

- OpenSBI 的常规陷阱入口保持不变；`CONFIG_SBI_ECALL_BENCH` 只在
  `sbi_trap_handler` 判定 `mcause` 为 S-mode ecall 之后增加一次 `a7`
  比对，匹配私有 EID 则走原返回路径，不进入扩展查找。
- 所有基准测试代码仅在启用 `CONFIG_SBI_ECALL_BENCH` 时参与编译。
- M-mode 临时允许 S-mode 读取并使用 cycle 计数器，屏蔽 M-mode 中断源，
  并安装专用的最小陷阱向量。
- S-mode 在两次 `cycle` 读取之间连续执行 1,024 条 ECALL 指令。相邻
  ECALL 之间没有循环控制、分支或其他 S-mode 指令。
- 同一套代码通过不同的 defconfig 支持 QEMU `virt` 和 SpacemiT K3。
  普通配置不会包含基准测试目标文件或运行时挂钩。

## 相对 k3-br-v1.0.0 的修改

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
  随后同一套连续 ECALL 再测两条路径：原 `_trap_handler` 在
  `mcause`/`a7` 处早返回，以及完整 `get_spec_version`。
  最后测量完整的不支持扩展查找，并通过常规 SBI 调试控制台扩展输出结果。

M-mode 快速路径只包含一次针对 `a6` 的分支、`mepc` 的读取和更新、
将 `a0` 与 `a1` 清零，以及 `mret`。该路径有意假设测试运行在受控的
单 hart payload 中，并且不会发生故障。启动普通 payload 或操作系统时
不要启用此选项。

输出的 `cycles/ecall` 是完整 ECALL 往返开销的平均值。S-mode 中的
`rdcycle` 指令和排序屏障只位于每批测试的边界，其开销由 1,024 次连续
ECALL 共同摊销。

## 优化等级与 K3 实测结果

本版本 OpenSBI Makefile 只判断 `DEBUG` 是否为空：任何非空值都会选择
`-O0`，因此宿主环境中的 `DEBUG=release` 实际上仍是无优化构建。两个
benchmark 脚本均显式传入空的 `DEBUG`，固定使用上游 release 默认的
`-O2`，不继承宿主环境中的同名变量。

K3 上使用完全相同的测试代码分别测得：

| 路径 | `-O0` cycles/ecall | `-O2` cycles/ecall | 改善 |
| --- | ---: | ---: | ---: |
| minimal mtvec | 62.029 | 62.029 | 0% |
| original handler early a7 | 412.037 | 351.041 | 14.8% |
| full SBI get_spec_version | 527.406 | 391.095 | 25.8% |
| full SBI unsupported ext | 664.953 | 408.352 | 38.6% |

纯汇编 minimal mtvec 路径在两种优化等级下完全相同，说明 cycle 计数、
特权级切换和测试边界没有变化；路径包含的 C 分发代码越多，`-O2` 消除的
栈访问、寄存器搬运和函数调用开销越明显。

此前 Linux 内核模块测得的 `get_spec_version` 约为 430 cycles。该结果还
可能包含内核导出的 `__sbi_ecall()` 包装函数、参数准备和函数调用开销；
当前 payload 直接执行 1,024 条相邻 ECALL，因此 391.095 cycles 更接近
OpenSBI 完整处理路径本身，两者不能直接视为同一测量口径。

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

恢复原 `mtvec` 之后，同一套连续 ECALL（`a7 = SBI_EXT_ECALL_BENCH`）
再走一遍原始 `_trap_handler`：保存全部上下文、进入
`sbi_trap_handler`，在 `mcause == SUPERVISOR_ECALL` 之后比对 `a7`，
然后 `mepc += 4` 并沿原 restore/`mret` 返回。这条路径量的是「完整
trap 进出 + mcause/a7 分发」，不含扩展表查找和 `get_spec_version`
处理函数。第三条路径才是完整 SBI `get_spec_version`。

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

## 制作可直接烧录的 K3 SD 卡镜像

`scripts/ecall-bench-sdcard.sh` 可以基于官方 Bianbu K3 SD 卡镜像生成
一份独立的 ECALL 测试镜像。脚本不会覆盖输入镜像，也不会修改其中的
ESP、`bootfs` 或 `rootfs`。脚本只替换分区之前的 OpenSBI FIT 和
U-Boot 下一阶段 FIT 两个裸区域。

当前确认的 `Bianbu-LXQt-K3-sdcard-v4.0-20260430170328.img` 布局为：

| 起始位置 | 大小 | 内容 | 测试镜像处理方式 |
| --- | ---: | --- | --- |
| `0x700000` | 1 MiB | OpenSBI FIT | 替换为 benchmark `fw_dynamic` FIT |
| `0x800000` | 3 MiB 安全替换区 | U-Boot 下一阶段 FIT | 替换为独立 S-mode payload FIT |
| 12 MiB | 256 MiB | ESP 分区 | 保持不变 |
| 268 MiB | 256 MiB | `bootfs` 分区 | 保持不变 |
| 524 MiB | 8 GiB | `rootfs` 分区 | 保持不变 |

原镜像在 `0x700000` 存放的是加载到 `0x100000000` 的 `fw_dynamic`。
测试镜像保持相同的 `fw_dynamic` 类型、加载地址和入口地址，仅加入
ECALL benchmark 的 M-mode 准备与陷阱代码。`0x800000` 处仍使用 K3 SPL
认识的 U-Boot FIT 结构，但其中的 `loadables` 被替换为最小 S-mode 测试
程序；它使用原生 U-Boot 地址 `0x102000000` 作为加载地址和入口地址。

脚本从原始 U-Boot FIT 中提取全部 13 份板型 DTB，并原样放入新的
下一阶段 FIT，因此 SPL 仍可按产品名选择 `k3_deb1`、`k3_evb`、
`k3_com260` 等配置。OpenSBI 从 SPL 提供的 `fw_dynamic_info` 获得
`0x102000000`，完成初始化后进入测试 payload，不再启动 U-Boot 和
Linux。当前 K3 benchmark 配置打开了 `CONFIG_ENABLE_LOGGING`，便于从
串口确认 OpenSBI 已经启动；日志发生在测量之前，不计入 ECALL cycle。

脚本不会改写 `0xB00000–0xBFFFFF`。官方 IMG 文件的主 GPT 表位于磁盘
开头，但镜像写入更大 SD 卡并扩展 GPT 后，主分区表项可能被移动到
`0xBFC000`，即第一个分区之前的最后 16 KiB。保留最后 1 MiB 可以同时
兼容原始 IMG 和扩展后的物理卡布局，避免覆盖 GPT 元数据。

从仓库根目录执行：

```sh
CROSS_COMPILE=/opt/spacemit-toolchain-linux-glibc-x86_64-v1.2.4/bin/riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench-sdcard.sh \
  ../Bianbu-LXQt-K3-sdcard-v4.0-20260430170328.img.gz \
  build/sdcard/Bianbu-LXQt-K3-sdcard-v4.0-ecall-bench.img
```

脚本会完成 K3 benchmark 编译、原 DTB 提取、两个 FIT 封装、原镜像
解压/复制、裸区域替换以及替换内容校验。如果输出文件已经存在，脚本会拒绝覆盖。该脚本
针对上述 Bianbu v4.0 镜像布局编写，并会检查 `0x700000` 和 `0x800000`
处是否存在预期的 FIT 头，避免误改布局不兼容的镜像。

脚本会显式向 OpenSBI Makefile 传入空的 `DEBUG`，使用上游 release 默认的
`-O2` 优化等级。这样即使宿主环境中存在 `DEBUG=release` 等变量，也不会被
这版 Makefile 误判为调试构建并退回 `-O0`。`CONFIG_ENABLE_LOGGING` 仍然
保留，用于输出 OpenSBI banner 和启动诊断；这些输出发生在 ECALL 计时窗口
之外。

生成的原始镜像大小为 `9,139,408,896` 字节，因此应使用容量足够的
SD 卡，通常需要 16 GB 或更大的卡。烧录前务必使用 `lsblk` 再次确认
设备名；以下命令中的 `/dev/sdX` 必须替换为整张 SD 卡设备，而不是某个
分区。选错设备会覆盖其他磁盘的数据。

```sh
lsblk -o NAME,SIZE,MODEL,TRAN,MOUNTPOINTS
sudo dd \
  if=build/sdcard/Bianbu-LXQt-K3-sdcard-v4.0-ecall-bench.img \
  of=/dev/sdX bs=16M conv=fsync status=progress
```

烧录后从 SD 卡启动 K3，并通过串口查看结果。测试完成后 payload 会停在
`wfi`，不会继续启动 Linux。如果需要恢复正常系统，重新烧录原始 Bianbu
镜像即可。

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

推荐使用单 hart，以减少 QEMU 虚拟 CPU 调度等额外因素带来的测量噪声：

```sh
qemu-system-riscv64 -M virt -m 256M -smp 1 -nographic \
  -bios build/platform/generic/firmware/fw_payload.bin
```

`-smp 1` 不是功能上的强制要求。使用多个虚拟 hart 时，只有 coldboot
hart 会安装测试专用 `mtvec` 并执行基准测试，其他 hart 停留在 OpenSBI
的 HSM/`wfi` 等待流程；S-mode payload 自身也通过原子 hart lottery 保证
只有一个 hart 调用 `test_main()`。不过为了提高结果的可重复性，延迟测量
仍建议使用 `-smp 1`。

payload 会输出测得的 cycle 总数和每次 ECALL 的平均 cycle 数，随后进入
`wfi` 等待；此时需要手动退出 QEMU。

QEMU 的 cycle 数适合用于功能验证和指令路径确认；实际硬件延迟应以
目标 K3 板上的测量结果为准。
