# S-mode ECALL latency benchmark

This test measures the cost of entering M-mode with `ecall` and returning to
S-mode with `mret`, without using OpenSBI's normal trap context save and C
ECALL dispatch path.

It is intentionally a test-only configuration:

- OpenSBI completes cold-boot initialization and configures PMP normally.
- Immediately before entering the built-in S-mode payload, OpenSBI saves the
  normal `mtvec`, disables M-mode interrupt sources, enables S-mode access to
  the `cycle` counter, and installs a temporary minimal trap vector.
- The S-mode payload warms the path, measures 100,000 ECALL round trips, and
  separately measures the loop overhead.
- A final, untimed ECALL restores the original `mtvec`, `mie`, and
  `mcounteren`. The payload then prints the result through the normal SBI debug
  console extension.

The measured fast path contains only a branch on `a6`, `mepc` read/update,
zeroing of `a0` and `a1`, and `mret`. It deliberately assumes a controlled,
single-hart payload with no faults. Do not enable this option when booting a
normal payload or operating system.

## Build

For RV64 QEMU `virt`:

```sh
make PLATFORM=generic \
  PLATFORM_DEFCONFIG=ecall_bench_defconfig \
  CROSS_COMPILE=riscv64-linux-gnu-
```

For the SpacemiT K3 target, use the corresponding test-only configuration:

```sh
make PLATFORM=generic \
  PLATFORM_DEFCONFIG=k3_ecall_bench_defconfig \
  CROSS_COMPILE=riscv64-linux-gnu-
```

The image is generated at:

```text
build/platform/generic/firmware/fw_payload.bin
```

## Run on QEMU

Use one hart so only the cold-boot hart enters the benchmark payload:

```sh
qemu-system-riscv64 -M virt -m 256M -smp 1 -nographic \
  -bios build/platform/generic/firmware/fw_payload.bin
```

The payload prints the measured cycles, matching loop overhead, net cycles,
and the net cycles per ECALL. It then waits in `wfi`; terminate QEMU manually.

QEMU cycle counts are useful for functional and instruction-path validation,
but hardware latency must be measured on the target K3 board.
