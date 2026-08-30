#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-2-Clause

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_dir="$(cd "$script_dir/.." && pwd)"

if [[ $# -ne 2 ]]; then
	echo "Usage: $(basename "$0") BIANBU_IMAGE[.gz] OUTPUT_IMAGE" >&2
	exit 1
fi

input_image="$(realpath "$1")"
output_image="$(realpath -m "$2")"
output_parent="$(dirname "$output_image")"
build_dir="${BUILD_DIR:-$source_dir/build/ecall-bench/k3-sdcard}"
cross_compile="${CROSS_COMPILE:-riscv64-unknown-linux-gnu-}"
mkimage_binary="${MKIMAGE:-mkimage}"
jobs="${JOBS:-$(nproc)}"

# Layout observed in Bianbu-LXQt-K3-sdcard-v4.0-20260430170328.img.
opensbi_offset=$((0x700000))
opensbi_slot_size=$((0x100000))
uboot_offset=$((0x800000))
fit_magic="d00dfeed"

if [[ "$input_image" == "$output_image" ]]; then
	echo "Input and output images must be different" >&2
	exit 1
fi

if [[ -e "$output_image" ]]; then
	echo "Refusing to overwrite existing output: $output_image" >&2
	exit 1
fi

if [[ ! "$jobs" =~ ^[1-9][0-9]*$ ]]; then
	echo "Invalid job count: $jobs" >&2
	exit 1
fi

for command_name in make "$mkimage_binary" dd od tr cmp stat realpath sha256sum; do
	if ! command -v "$command_name" >/dev/null 2>&1; then
		echo "Required command not found: $command_name" >&2
		exit 1
	fi
done

if [[ "$input_image" == *.gz ]] && ! command -v gzip >/dev/null 2>&1; then
	echo "Required command not found: gzip" >&2
	exit 1
fi

mkdir -p "$build_dir" "$output_parent"

echo "Building the K3 ECALL benchmark for the Bianbu SD layout"
make -C "$source_dir" -j "$jobs" \
	O="$build_dir" \
	PLATFORM=generic \
	PLATFORM_DEFCONFIG=k3_ecall_bench_defconfig \
	CROSS_COMPILE="$cross_compile" \
	FW_TEXT_START=0x100000000 \
	FW_PAYLOAD_OFFSET=0x80000

firmware_dir="$build_dir/platform/generic/firmware"
payload_bin="$firmware_dir/fw_payload.bin"
fit_source="$source_dir/firmware/payloads/ecall_bench_k3_sd.its"
fit_local="$firmware_dir/ecall_bench_k3_sd.its"
fit_image="$firmware_dir/ecall_bench_k3_sd.itb"

cp "$fit_source" "$fit_local"
(
	cd "$firmware_dir"
	"$mkimage_binary" -f "$(basename "$fit_local")" \
		"$(basename "$fit_image")"
)

fit_size="$(stat -c %s "$fit_image")"
if (( fit_size > opensbi_slot_size )); then
	echo "FIT image is too large for the 1 MiB OpenSBI slot: $fit_size bytes" >&2
	exit 1
fi

temporary_image="$output_image.tmp.$$"
cleanup()
{
	rm -f "$temporary_image"
}
trap cleanup EXIT

echo "Creating an independent image copy: $output_image"
case "$input_image" in
*.gz)
	gzip -dc -- "$input_image" > "$temporary_image"
	;;
*)
	cp --reflink=auto -- "$input_image" "$temporary_image"
	;;
esac

read_magic()
{
	dd if="$1" bs=1 skip="$2" count=4 status=none |
		od -An -tx1 | tr -d ' \n'
}

if [[ "$(read_magic "$temporary_image" "$opensbi_offset")" != "$fit_magic" ]]; then
	echo "No FIT image found at Bianbu OpenSBI offset 0x700000" >&2
	exit 1
fi

if [[ "$(read_magic "$temporary_image" "$uboot_offset")" != "$fit_magic" ]]; then
	echo "No U-Boot FIT found at expected offset 0x800000" >&2
	exit 1
fi

echo "Replacing only the 1 MiB OpenSBI slot at offset 0x700000"
dd if=/dev/zero of="$temporary_image" bs=1M seek=7 count=1 \
	conv=notrunc status=none
dd if="$fit_image" of="$temporary_image" bs=4K seek=1792 \
	conv=notrunc status=none

cmp -n "$fit_size" -i "0:$opensbi_offset" "$fit_image" "$temporary_image"

if [[ "$(read_magic "$temporary_image" "$uboot_offset")" != "$fit_magic" ]]; then
	echo "U-Boot FIT was unexpectedly modified" >&2
	exit 1
fi

mv "$temporary_image" "$output_image"
trap - EXIT

echo "Created flashable SD image: $output_image"
echo "OpenSBI FIT size: $fit_size bytes"
sha256sum "$output_image"
