# OpenSBI S-mode ECALL 延迟基准测试

本分支基于 `k3-br-v1.0.0`，用于测量 S-mode 执行 `ecall`、进入 OpenSBI
M-mode，再通过 `mret` 返回 S-mode 的周期数。测试代码由
`CONFIG_SBI_ECALL_BENCH` 隔离，原 `k3_defconfig` 和普通 OpenSBI 路径保持不变。
`sbi_trap.c`、`sbi_ecall.c` 和 K3 多核启动代码均未修改；当前版本不包含 hart8、
中断控制器或平台初始化规避。测试 extension 放在查找链表末尾，不改变原有
extension 的查找顺序。使用相同版本元数据构建时，关闭 benchmark 的 K3
`fw_dynamic.bin` 与 `k3-br-v1.0.0` 逐字节一致。

- 完整测试报告：[ECALL_BENCH_REPORT.md](ECALL_BENCH_REPORT.md)
- 上游 OpenSBI 原说明：[README.orig.md](README.orig.md)

## 测试内容

裸 ECALL 路径每批连续执行 1,024 条 `ecall`，相邻指令之间没有循环、分支或其他
S-mode 指令。新增的 wrapper 路径参考 `../instr-cycles`，每批静态展开 64 次
Linux ABI 风格的 `__sbi_ecall()` 调用。第一批均用于预热并丢弃，之后累计 800 批。
当前依次测试以下路径：

| 路径 | 含义 | 已有 K3 `-O2` 参考值 |
| --- | --- | ---: |
| minimal mtvec | 专用最小 M-mode 返回路径 | 62.029 |
| registered empty extension | 原 trap 入口和标准 SBI extension 分发 | 待重新实测 |
| full SBI get_spec_version | 完整 SBI Base `GET_SPEC_VERSION` | 391.095 |
| SBI get_spec_version wrapper | S-mode 参数准备、函数 call/return 和完整 SBI Base 路径 | 待实测 |
| full SBI unsupported ext | 完整的不支持扩展查找 | 408.352 |
| SBI unsupported ext wrapper | S-mode wrapper 和完整的不支持扩展查找 | 待实测 |

上一版在 `sbi_trap_handler()` 中提前检查私有 EID，测得 351.041 cycles/ecall。当前
版本已经删除该特例，改为通过 `sbi_ecall_handler()` 查找并调用注册的空 extension；
表中其余数字也应在同一份新镜像上重新采集后再作严格比较。

计时边界使用 `fence iorw, iorw` 和 S-mode `rdcycle`。计数器读取、结果累计和
外层循环都位于连续 ECALL 块之外。构建脚本固定使用 OpenSBI release 默认 `-O2`，
避免非空 `DEBUG` 意外触发 `-O0`。

## 最小 M-mode 路径

进入 S-mode 前，OpenSBI 临时开放 S-mode cycle counter、保存原 `mtvec` 和相关
计数器/中断状态，然后安装以下测试入口：

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

被测热路径只有以上 7 条 M-mode 指令，不使用栈、不保存通用寄存器上下文，也不
进入 C 分发器。测量结束后使用一次不计时的 ECALL 恢复原状态，再测注册的空
extension、裸 SBI Base、S-mode wrapper 和不支持扩展路径。

wrapper 使用与 Linux `__sbi_ecall()` 相同的参数顺序，并强制保持为独立 C 函数。
每次调用前重新准备 `a0` 至 `a7`，最终反汇编包含独立栈帧、`ecall` 和 `ret`。
它不包含 Linux tracepoint/static-key、内核调度或中断环境，因此用于观察裸 payload
中的 wrapper 增量，不能直接等同于 Linux 内核中的完整 `__sbi_ecall()`。

## 主要文件

- `lib/sbi/sbi_ecall_bench.S`：M-mode 准备、最小 trap 和状态恢复；
- `lib/sbi/sbi_ecall_bench_ext.c`：通过标准框架注册的空 SBI extension；
- `firmware/payloads/ecall_bench.S`：连续展开的 S-mode ECALL 与 cycle 计时；
- `firmware/payloads/test_main.c`：提供 S-mode C wrapper，并依次运行六条路径；
- `scripts/ecall-bench.sh`：QEMU/K3 构建与 QEMU 启动；
- `scripts/ecall-bench-sdcard.sh`：制作可直接烧录的 K3 SD 卡镜像。

## 构建和运行

查看脚本帮助：

```sh
./scripts/ecall-bench.sh --help
```

构建并运行 QEMU：

```sh
CROSS_COMPILE=riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench.sh qemu
```

只构建 QEMU 或 K3：

```sh
CROSS_COMPILE=riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench.sh qemu-build

CROSS_COMPILE=riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench.sh k3-build
```

默认输出：

```text
build/ecall-bench/qemu/platform/generic/firmware/fw_payload.bin
build/ecall-bench/k3/platform/generic/firmware/fw_payload.bin
```

通常无需指定 `--fw-text-start`：链接基址为零时 OpenSBI 会按实际加载位置完成运行时
重定位。只有启动流程要求固定链接地址时才应传入真实地址；示例中的
`0x80000000` 不是 K3 默认地址。

QEMU 推荐 `-smp 1` 以减少噪声，但并非 ECALL 功能限制。benchmark 本身按单个
coldboot hart 设计，不增加任何多核或 hart8 特判；其他 hart 完全沿用原版 OpenSBI
和 K3 平台启动流程。

## 制作 K3 SD 卡镜像

基于已确认布局的 Bianbu v4.0 镜像生成独立测试镜像：

```sh
CROSS_COMPILE=/opt/spacemit-toolchain-linux-glibc-x86_64-v1.2.4/bin/riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench-sdcard.sh \
  ../Bianbu-LXQt-K3-sdcard-v4.0-20260430170328.img.gz \
  build/sdcard/Bianbu-LXQt-K3-sdcard-v4.0-ecall-bench.img
```

脚本只替换分区前的 OpenSBI 和下一阶段 payload FIT，不修改 ESP、`bootfs` 或
`rootfs`，并保留可能位于首分区前末尾区域的 GPT 元数据。测试配置开启
`CONFIG_ENABLE_LOGGING`，串口输出发生在计时窗口之外。

烧录前必须再次确认整卡设备名，不能填写某个分区：

```sh
lsblk -o NAME,SIZE,MODEL,TRAN,MOUNTPOINTS
sudo dd if=build/sdcard/Bianbu-LXQt-K3-sdcard-v4.0-ecall-bench.img \
  of=/dev/sdX bs=16M conv=fsync status=progress
```

测试完成后 payload 停在 `wfi`，不会继续启动 Linux。SD 卡布局、完整结果、
`-O0`/`-O2` 对比和 Linux wrapper 分析均见测试报告。
