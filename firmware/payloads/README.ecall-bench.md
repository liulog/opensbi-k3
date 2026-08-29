# S-mode ECALL latency benchmark

This test measures the cost of entering M-mode with `ecall` and returning to
S-mode with `mret`, without using OpenSBI's normal trap context save and C
ECALL dispatch path.

It is intentionally a test-only configuration:

- OpenSBI completes cold-boot initialization and configures PMP normally.
- Immediately before entering the built-in S-mode payload, OpenSBI saves the
  normal `mtvec`, disables M-mode interrupt sources, enables S-mode access to
  the `cycle` counter, and installs a temporary minimal trap vector.
- The S-mode payload executes one warm-up pass through the exact measured code
  block, then measures 800 batches of 1,024 adjacent ECALL round trips.
- Each measured batch is a branch-free 4 KiB instruction block containing
  only consecutive ECALL instructions. Counter reads, accumulation, and the
  outer-loop branch are outside the block.
- A matching block of 1,024 non-compressed NOP instructions measures the
  counter-boundary and S-mode instruction-stream baseline.
- A final, untimed ECALL restores the original `mtvec`, `mie`, and
  `mcounteren`. The payload then prints the result through the normal SBI debug
  console extension.

The M-mode fast path contains only a branch on `a6`, `mepc` read/update,
zeroing of `a0` and `a1`, and `mret`. It deliberately assumes a controlled,
single-hart payload with no faults. Do not enable this option when booting a
normal payload or operating system.

The primary `cycles/ecall` result is the complete measured ECALL round-trip
average. The S-mode `rdcycle` instructions are only the timing boundaries and
are amortized over 1,024 adjacent ECALLs per batch. `net cycles/ecall` also
subtracts the matching NOP block and therefore represents the ECALL cost above
an ordinary 4-byte instruction stream; it is provided as a secondary value.

## Build

The convenience script builds both variants and can run the QEMU image:

```sh
CROSS_COMPILE=riscv64-linux-gnu- scripts/ecall-bench.sh qemu
CROSS_COMPILE=riscv64-linux-gnu- scripts/ecall-bench.sh k3-build
```

If a boot stage requires an ELF linked at the board's actual OpenSBI load
address, pass it explicitly. For example:

```sh
scripts/ecall-bench.sh k3-build --fw-text-start 0x80000000
```

Leaving `FW_TEXT_START` unset uses zero as the link-time base. OpenSBI computes
the runtime load offset and applies its relative relocations during early boot.

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

## Link and load addresses

There is no standalone linker script specifically for either
`ecall_bench.S` file:

- `lib/sbi/sbi_ecall_bench.S` is part of OpenSBI and is linked by
  `firmware/fw_payload.elf.ldS`. It follows the normal OpenSBI PIE relocation
  path when the runtime address differs from `FW_TEXT_START`.
- `firmware/payloads/ecall_bench.S` is part of the built-in S-mode test
  payload and is linked by `firmware/payloads/test.elf.ldS` at
  `FW_TEXT_START + FW_PAYLOAD_OFFSET`.

The generic RV64 platform uses a payload offset of `0x200000`. The payload is
embedded in `fw_payload.bin`, and `fw_next_addr()` obtains its runtime address
with a PC-relative symbol reference. The payload is compiled with the `medany`
code model and contains no dynamic relocations, so QEMU and a board may load
the complete `fw_payload.bin` at different base addresses. They must preserve
the layout of the complete image; do not extract and independently move only
`test.bin` without also providing a matching next-stage entry address.

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
