# K3 S-mode ECALL 周期测试报告

## 1. 报告范围

本报告整理 SpacemiT K3 上从 S-mode 执行 `ecall`、进入 OpenSBI M-mode，再通过
`mret` 返回 S-mode 的周期测试。报告同时记录：

- OpenSBI 最小临时 `mtvec`、注册的空 SBI extension、完整 SBI Base，以及 Base/
  unsupported 的 raw/wrapper 对照，共六条路径的测试方法；
- 上一版原陷阱入口提前检查 EID 路径的 K3 板端实测结果；
- OpenSBI 使用 `-O0` 与官方 release 默认 `-O2` 时的差异；
- 之前 Linux 内核模块约 430 cycles 的历史结果及其口径差异；
- 为隔离 Linux `__sbi_ecall()` wrapper 新增的成对测试方法和当前验证状态。

报告整理日期为 2026-08-31。OpenSBI 测试分支为
`k3-br-v1.0.0-ecall-bench`，基于 `k3-br-v1.0.0`。下表已有 K3 数字对应
`ae4ec3e5` 的 early-check 实现；当前工作区已改用标准注册 extension，尚待重新上板
采集，二者在报告中分别标识。

## 2. 测试目标

测试希望分层回答以下问题：

1. 单纯发生 S-mode → M-mode → S-mode 特权级切换，并由极简汇编推进 `mepc`，需要
   多少周期；
2. OpenSBI 原始 trap 入口保存和恢复完整上下文，并通过标准 extension 框架调用
   一个空 handler，需要多少周期；
3. 完整执行 SBI Base `GET_SPEC_VERSION` 分发路径需要多少周期；
4. Linux 中测得的结果比裸 S-mode payload 更高，是否可能来自 Linux SBI wrapper；
5. OpenSBI 编译优化等级对以上路径有多大影响。

报告的主指标是：

```text
cycles/ecall = 累计 measured cycles / 累计 ECALL 次数
```

结果只报告 cycle 数，不换算成纳秒，因此不依赖对 CPU 主频的额外假设。

## 3. 测试环境

| 项目 | 配置 |
| --- | --- |
| 硬件 | SpacemiT K3 开发板 |
| 固件基础 | OpenSBI `k3-br-v1.0.0` |
| 测试分支 | `k3-br-v1.0.0-ecall-bench` |
| 当前 release 构建 | `DEBUG=`，OpenSBI 默认 `-O2` |
| 对照构建 | `DEBUG` 非空，OpenSBI 选择 `-O0` |
| S-mode 载荷 | 仓库内置最小汇编/C payload，不启动 U-Boot 或 Linux |
| 计数器 | S-mode 读取 `cycle` CSR |
| SBI 版本 | 板端输出为 SBI spec 2.0 |
| 每批操作 | 裸 ECALL 1,024 条；wrapper 调用 64 次 |
| 有效批次 | 800 批 |
| 有效操作总数 | 裸 ECALL 819,200 次；wrapper 调用 51,200 次 |

测试镜像使用 Bianbu K3 SD 卡镜像的原启动布局：OpenSBI FIT 位于
`0x700000`，最小 S-mode payload FIT 位于 `0x800000`。OpenSBI 日志和结果输出均
发生在 cycle 计时窗口之外。

## 4. OpenSBI 裸 payload 测试方法

### 4.1 计时结构

每一项实际执行 801 批。第一批在完全相同的代码地址执行，但结果被丢弃，用于预热
指令缓存、trap 路径和外层分支；之后累计 800 批。

每个有效批次的结构为：

```asm
fence iorw, iorw
rdcycle start
fence iorw, iorw

.rept 1024
ecall
.endr

fence iorw, iorw
rdcycle end
fence iorw, iorw
```

1,024 条 `ecall` 由汇编器完全展开，相邻 `ecall` 之间没有 S-mode 循环计数、分支、
参数准备或其他指令。计数器读取、累计和外层循环均在该连续代码块之外。边界 fence
和 `rdcycle` 的固定成本没有单独扣除，而是由每批 1,024 次 `ecall` 共同摊薄。

wrapper 路径沿用相同的 fence、`rdcycle`、预热和 800 个有效批次，但参考
`../instr-cycles` 的 `CALL_64`，每批静态展开 64 次调用。

### 4.2 六条被测路径

| 路径 | M-mode 行为 | 结果包含的主要内容 |
| --- | --- | --- |
| `minimal mtvec` | 临时替换 `mtvec`，执行 7 条热路径汇编 | 特权切换、`mepc` 更新、最小返回值和 `mret` |
| `registered empty extension` | 恢复原 `_trap_handler`，由 `sbi_ecall_handler()` 查找并调用测试 extension | 完整 trap 上下文保存/恢复、extension 查找、间接调用和标准返回更新 |
| `full SBI get_spec_version` | SBI Base / `GET_SPEC_VERSION` | 完整 trap 进出、扩展查找和 SBI Base handler |
| `SBI get_spec_version wrapper` | 每次准备 `a0` 至 `a7` 并调用独立 S-mode C wrapper | 参数准备、wrapper 栈帧、call/return 和完整 SBI Base handler |
| `full SBI unsupported ext` | 查找一个未注册 EID 并返回 `SBI_ERR_NOT_SUPPORTED` | 完整 trap 进出和扩展查找失败路径 |
| `SBI unsupported ext wrapper` | 通过同一个 S-mode C wrapper 调用未注册 EID | 参数准备、wrapper 栈帧、call/return 和完整查找失败路径 |

最小 `mtvec` 的 M-mode 热路径为：

```asm
bnez a6, stop
csrr  a0, mepc
addi  a0, a0, 4
csrw  mepc, a0
li    a0, 0
li    a1, 0
mret
```

测试准备阶段允许 S-mode 读取 `cycle`、确保 cycle counter 正在运行、暂时清零
`mie`，并安装最小 `mtvec`。最小路径测量完成后，一个不计时的停止 ECALL 恢复
`mtvec`、`mie`、`mcounteren` 和 `mcountinhibit`；之后五项使用原始 OpenSBI trap
环境。S-mode payload 不包含 Linux 调度、抢占或普通 S-mode 中断负载，但恢复原
`mie` 后的测试没有额外屏蔽所有更高特权级异步事件，异常事件仍可能形成噪声。

S-mode wrapper 的参数顺序与 Linux `__sbi_ecall()` 一致，并通过 `noinline` 保证
不会被 `-O2` 展开成裸 `ecall`。它不复制 Linux tracepoint/static-key，也没有内核
调度环境，因此只能用于测量当前裸 payload 的 wrapper 软件增量，不能冒充完整
Linux wrapper。

## 5. K3 板端实测结果

### 5.1 上一版 early-check 的 `-O2` 结果

| 路径 | measured cycles | ECALL 数 | cycles/ecall |
| --- | ---: | ---: | ---: |
| minimal mtvec | 50,814,400 | 819,200 | **62.029** |
| original handler early a7 | 287,573,015 | 819,200 | **351.041** |
| full SBI get_spec_version | 320,385,406 | 819,200 | **391.095** |
| full SBI unsupported ext | 334,522,350 | 819,200 | **408.352** |

板端同时确认：

```text
sbi spec  : 2.0
sbi error : -2
```

### 5.2 上一版 early-check 的 `-O0` 对照结果

| 路径 | measured cycles | ECALL 数 | cycles/ecall |
| --- | ---: | ---: | ---: |
| minimal mtvec | 50,814,400 | 819,200 | **62.029** |
| original handler early a7 | 337,541,002 | 819,200 | **412.037** |
| full SBI get_spec_version | 432,051,030 | 819,200 | **527.406** |
| full SBI unsupported ext | 544,729,774 | 819,200 | **664.953** |

### 5.3 优化等级对比

| 路径 | `-O0` | `-O2` | 减少 cycles/ecall | 改善比例 |
| --- | ---: | ---: | ---: | ---: |
| minimal mtvec | 62.029 | 62.029 | 0.000 | 0% |
| original handler early a7 | 412.037 | 351.041 | 60.996 | 14.8% |
| full SBI get_spec_version | 527.406 | 391.095 | 136.311 | 25.8% |
| full SBI unsupported ext | 664.953 | 408.352 | 256.601 | 38.6% |

最小路径完全由手写汇编构成，两种优化等级下结果一致，说明 ECALL 指令块、cycle
边界和特权级切换本身没有因编译选项改变。路径中的 C 分发和扩展查找越多，`-O2`
带来的改善越明显。当前两个 benchmark 脚本均显式传入空 `DEBUG`，避免宿主环境中
类似 `DEBUG=release` 的非空变量意外触发 `-O0`。

### 5.4 分层开销观察

根据上一版 early-check 的 `-O2` 结果直接相减：

| 对比 | 增量 cycles/ecall | 含义 |
| --- | ---: | --- |
| early handler − minimal mtvec | 289.012 | 完整 trap 上下文进出及早期 C 分发相对极简路径的增量 |
| full get_spec_version − early handler | 40.054 | 扩展查找和 SBI Base handler 相对固定早返回的增量 |
| unsupported ext − get_spec_version | 17.257 | 当前构建中扩展查找失败路径相对 Base 成功路径的增量 |
| full get_spec_version − minimal mtvec | 329.066 | 完整 SBI Base 路径相对极简 ECALL 往返的总增量 |

这些差值是同一次测试程序中的平均值差异，但仍不等价于逐条硬件指令延迟分解；cache、
分支预测、trap 入口实现和更高特权级事件都会影响最终数值。

### 5.5 当前 registered-extension 版本

当前实现已经从 `sbi_trap_handler()` 删除 benchmark EID 的提前判断。测试 EID
`0x08000000` 作为 experimental extension 加入 OpenSBI 的 extension carray，并在
`sbi_ecall_init()` 中通过 `sbi_ecall_register_extension()` 注册。其 FID 0 handler
不执行实际业务，只返回 `SBI_SUCCESS`。

因此新的第二条路径会完整执行：

```text
原 _trap_handler 上下文保存
→ sbi_trap_handler() 的 mcause 分发
→ sbi_ecall_handler()
→ sbi_ecall_find_extension()
→ 空 extension handler
→ 标准 mepc/a0/a1 更新
→ 原上下文恢复和 mret
```

QEMU 和 K3 `-O2` 镜像均已编译成功；QEMU 已确认新 extension 返回 `sbi error = 0`、
SBI spec 仍为 2.0，unsupported 路径仍返回 `-2`。反汇编确认裸 S-mode 被测块仍是
1,024 条连续 `ecall`，两条 wrapper 被测块各有 64 个静态展开调用且没有内部循环。
QEMU cycle 只用于功能验证，不写入 K3 结果表。

从路径组成看，新结果应高于旧 early-check 的 351.041 cycles/ecall，并可能接近
完整 Base handler 的 391.095 cycles/ecall；两者的具体高低还受 extension 查找、
handler 代码和布局影响。这只是路径分析，不是测量结果。
当前 K3 结果为：

| 当前路径 | K3 cycles/call |
| --- | ---: |
| registered empty extension | 待重新上板采集 |
| SBI get_spec_version wrapper | 待重新上板采集 |
| SBI unsupported ext wrapper | 待重新上板采集 |

## 6. Linux 内核模块对照

### 6.1 历史结果

此前 Linux 内核模块通过标准 `sbi_ecall()` 测得 `GET_SPEC_VERSION` 约为
**430 cycles/call**。与当前裸 payload 的 `391.095 cycles/ecall` 相差约
**38.905 cycles**。

这个差值与 Linux `__sbi_ecall()` wrapper 的参数准备、函数 call/return、栈帧、
寄存器保存恢复以及 tracepoint/static-branch 位置有关，但旧结果与裸 payload 并非
同一次运行、同一 S-mode 软件环境，因此不能仅凭 430 − 391.095 就断言 wrapper
固定等于 38.905 cycles。

旧 Linux 用例本身已经执行：

- `preempt_disable()`；
- `local_irq_save()`，清除 `sstatus.SIE`；
- `csrw sie, zero`，清除各类 S-mode 中断使能；
- 800 个有效批次，每批静态展开 64 次 wrapper 调用；
- 第一批原地预热但不计入结果。

因此，关中断和关抢占操作发生在整个测试之外，不进入单次 ECALL 的 cycle 区间，不能
直接解释多出的约 39 cycles；更可能的差异来源是每次 Linux wrapper 软件路径。

### 6.2 新增的成对验证用例

`../instr-cycles` 当前增加了两个使用相同公共环境的用例：

| 日志名 | 被测内容 |
| --- | --- |
| `sbi-base-get-spec-version-irq-off` | 原 Linux `sbi_ecall()`/`__sbi_ecall()` wrapper |
| `sbi-base-get-spec-version-raw-irq-off` | 每批只设置一次参数，随后 64 条严格相邻的裸 `ecall` |

两项都执行 800 × 64 = 51,200 次完整 SBI Base `GET_SPEC_VERSION`，使用相同的
`fence`、`rdcycle`、`rdinstret`、关抢占和关中断状态。模块还会输出：

```text
sbi-base-wrapper-overhead-estimate = wrapper - raw
```

该模块已使用目标内核的 `-O2` 配置编译成功。反汇编确认 raw body 总共且连续生成
64 条 `ecall`，中间没有其他 S-mode 指令；wrapper body 则静态展开 64 个
`__sbi_ecall()` 调用。

截至本报告整理时，这两个新用例尚未在 K3 Linux 环境重新采集，因此结果栏必须保留
为待测，不能用 430 和 391.095 代填：

| Linux 成对用例 | cycles/call |
| --- | ---: |
| wrapper | 待板端采集 |
| raw ECALL | 待板端采集 |
| wrapper − raw | 待板端采集 |

## 7. 结果结论

1. K3 上极简 S-mode ECALL 往返实测为 **62.029 cycles/ecall**。这是临时最小
   `mtvec` 和 7 条 M-mode 汇编路径的结果，不代表完整 OpenSBI SBI 调用。
2. 上一版 `-O2` early-check 实现测得 **351.041 cycles/ecall**，完整 SBI Base
   `GET_SPEC_VERSION` 为 **391.095 cycles/ecall**；这些数字继续作为历史基线。
3. 当前版本不再提前检查 EID，而是完整进入 `sbi_ecall_handler()` 并调用注册的空
   extension。其 K3 周期必须重新采集，不能直接沿用 351.041。
4. 裸 payload 为 SBI Base 和 unsupported EID 各增加了 64 次静态展开的 S-mode
   wrapper 路径，用于分别和对应裸 ECALL 结果对照；它们不包含 Linux tracepoint
   和内核运行环境。
5. OpenSBI C 路径必须使用 release `-O2` 进行性能测试。误用 `-O0` 会把完整
   `GET_SPEC_VERSION` 从 391.095 放大到 527.406 cycles/ecall，偏高约 34.9%。
6. Linux 历史结果约 430 cycles，与裸 payload 相差约 39 cycles；关中断和关抢占
   位于计时窗口之外，不太可能直接形成这 39 cycles。新增的 wrapper/raw 成对用例
   才能在同一次 Linux 运行中更可靠地隔离该增量。

## 8. 复现方法

构建当前 K3 OpenSBI benchmark：

```sh
CROSS_COMPILE=/opt/spacemit-toolchain-linux-glibc-x86_64-v1.2.4/bin/riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench.sh k3-build
```

基于 Bianbu 镜像制作可烧录测试镜像：

```sh
CROSS_COMPILE=/opt/spacemit-toolchain-linux-glibc-x86_64-v1.2.4/bin/riscv64-unknown-linux-gnu- \
  ./scripts/ecall-bench-sdcard.sh \
  ../Bianbu-LXQt-K3-sdcard-v4.0-20260430170328.img.gz \
  build/sdcard/Bianbu-LXQt-K3-sdcard-v4.0-ecall-bench.img
```

构建 Linux 成对对照模块：

```sh
make -C ../instr-cycles \
  KDIR=/home/jingyu/workspace/linux-riscv-gate \
  ARCH=riscv \
  CROSS_COMPILE=riscv64-unknown-linux-gnu-
```

在相同目标内核上加载模块后，从 `dmesg` 同时记录 wrapper、raw ECALL 和
`wrapper-overhead-estimate` 三行结果。正式报告建议重复加载多次，并记录 CPU/hart、
频率、温度和其他 hart 的负载，以便给出均值、方差或分位数，而不是只保留一次累计
平均值。
