#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-2-Clause

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_dir="$(cd "$script_dir/.." && pwd)"

command_name="${1:-qemu}"
if [[ $# -gt 0 ]]; then
	shift
fi

jobs="${JOBS:-$(nproc)}"
fw_text_start="${FW_TEXT_START:-}"
output_dir=""
cross_compile="${CROSS_COMPILE:-riscv64-unknown-linux-gnu-}"
qemu_binary="${QEMU:-qemu-system-riscv64}"

usage()
{
	cat <<EOF
Usage: $(basename "$0") COMMAND [OPTIONS]

Commands:
  qemu          Build and run the benchmark on QEMU (default)
  qemu-build    Build the QEMU benchmark image only
  k3-build      Build the K3 benchmark image only

Options:
  --fw-text-start ADDR  Link-time OpenSBI base address (for example 0x80000000)
  --output DIR          Build output directory
  --jobs NUM            Parallel build jobs (default: nproc)
  -h, --help            Show this help

Environment overrides:
  CROSS_COMPILE, QEMU, FW_TEXT_START, JOBS
  DEBUG is intentionally ignored; benchmark builds use upstream release -O2
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
	--fw-text-start)
		fw_text_start="${2:?missing address after --fw-text-start}"
		shift 2
		;;
	--output)
		output_dir="${2:?missing directory after --output}"
		shift 2
		;;
	--jobs)
		jobs="${2:?missing number after --jobs}"
		shift 2
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		echo "Unknown option: $1" >&2
		usage >&2
		exit 1
		;;
	esac
done

if [[ ! "$jobs" =~ ^[1-9][0-9]*$ ]]; then
	echo "Invalid job count: $jobs" >&2
	exit 1
fi

if [[ -n "$fw_text_start" &&
	! "$fw_text_start" =~ ^(0[xX][0-9a-fA-F]+|[0-9]+)$ ]]; then
	echo "Invalid FW_TEXT_START: $fw_text_start" >&2
	exit 1
fi

case "$command_name" in
qemu|qemu-build)
	defconfig="ecall_bench_defconfig"
	default_output="$source_dir/build/ecall-bench/qemu"
	;;
k3-build)
	defconfig="k3_ecall_bench_defconfig"
	default_output="$source_dir/build/ecall-bench/k3"
	;;
-h|--help|help)
	usage
	exit 0
	;;
*)
	echo "Unknown command: $command_name" >&2
	usage >&2
	exit 1
	;;
esac

output_dir="${output_dir:-$default_output}"
mkdir -p "$output_dir"

make_args=(
	-C "$source_dir"
	-j "$jobs"
	"O=$output_dir"
	# This OpenSBI version selects its release -O2 flags only when DEBUG is
	# empty.  Do not inherit a host value such as DEBUG=release as -O0.
	"DEBUG="
	PLATFORM=generic
	"PLATFORM_DEFCONFIG=$defconfig"
	"CROSS_COMPILE=$cross_compile"
)

if [[ -n "$fw_text_start" ]]; then
	make_args+=("FW_TEXT_START=$fw_text_start")
fi

echo "Building $defconfig in $output_dir"
echo "Optimization: OpenSBI release default (-O2, DEBUG empty)"
if [[ -n "$fw_text_start" ]]; then
	echo "FW_TEXT_START=$fw_text_start"
else
	echo "FW_TEXT_START=0 (OpenSBI runtime relocation enabled)"
fi
make "${make_args[@]}"

firmware_image="$output_dir/platform/generic/firmware/fw_payload.bin"
payload_elf="$output_dir/platform/generic/firmware/payloads/test.elf"
echo "Firmware: $firmware_image"
echo "S-mode payload ELF: $payload_elf"

if [[ "$command_name" == "qemu" ]]; then
	echo "Starting QEMU; use Ctrl-A X to exit."
	"$qemu_binary" -M virt -m 256M -smp 1 -nographic \
		-bios "$firmware_image"
fi
