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
dumpimage_binary="${DUMPIMAGE:-dumpimage}"
jobs="${JOBS:-$(nproc)}"

# Layout observed in Bianbu-LXQt-K3-sdcard-v4.0-20260430170328.img.
opensbi_offset=$((0x700000))
opensbi_slot_size=$((0x100000))
uboot_offset=$((0x800000))
# Keep the final 1 MiB before the first partition untouched. On expanded SD
# cards, the primary GPT entry array can live at 0xBFC000.
uboot_slot_size=$((0x300000))
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

for command_name in make "$mkimage_binary" "$dumpimage_binary" dd od tr cmp stat realpath sha256sum; do
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
	FW_PAYLOAD_OFFSET=0x2000000

firmware_dir="$build_dir/platform/generic/firmware"
opensbi_fit="$firmware_dir/fw_dynamic.itb"
payload_bin="$firmware_dir/payloads/test.bin"
next_fit_source="$source_dir/firmware/payloads/ecall_bench_k3_next.its"
next_fit_local="$firmware_dir/ecall_bench_k3_next.its"
next_fit="$firmware_dir/ecall_bench_k3_next.itb"
original_uboot_fit="$firmware_dir/k3_original_uboot.itb"

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

echo "Extracting the original K3 DTBs from the U-Boot FIT"
dd if="$temporary_image" of="$original_uboot_fit" bs=1M skip=8 count=4 \
	status=none
for ((index = 1; index <= 13; index++)); do
	"$dumpimage_binary" -T flat_dt -p "$index" \
		-o "$firmware_dir/fdt_${index}.dtb" "$original_uboot_fit" >/dev/null
done

cp "$next_fit_source" "$next_fit_local"
(
	cd "$firmware_dir"
	"$mkimage_binary" -f "$(basename "$next_fit_local")" \
		"$(basename "$next_fit")"
)

opensbi_fit_size="$(stat -c %s "$opensbi_fit")"
next_fit_size="$(stat -c %s "$next_fit")"
if (( opensbi_fit_size > opensbi_slot_size )); then
	echo "FIT image is too large for the 1 MiB OpenSBI slot: $opensbi_fit_size bytes" >&2
	exit 1
fi
if (( next_fit_size > uboot_slot_size )); then
	echo "FIT image is too large for the safe 3 MiB next-stage area: $next_fit_size bytes" >&2
	exit 1
fi

echo "Installing fw_dynamic OpenSBI at offset 0x700000"
dd if=/dev/zero of="$temporary_image" bs=1M seek=7 count=1 \
	conv=notrunc status=none
dd if="$opensbi_fit" of="$temporary_image" bs=4K seek=1792 \
	conv=notrunc status=none

echo "Installing the independent S-mode payload FIT at offset 0x800000"
dd if=/dev/zero of="$temporary_image" bs=1M seek=8 count=3 \
	conv=notrunc status=none
dd if="$next_fit" of="$temporary_image" bs=4K seek=2048 \
	conv=notrunc status=none

cmp -n "$opensbi_fit_size" -i "0:$opensbi_offset" "$opensbi_fit" "$temporary_image"
cmp -n "$next_fit_size" -i "0:$uboot_offset" "$next_fit" "$temporary_image"

mv "$temporary_image" "$output_image"
trap - EXIT

echo "Created flashable SD image: $output_image"
echo "OpenSBI fw_dynamic FIT size: $opensbi_fit_size bytes"
echo "S-mode next-stage FIT size: $next_fit_size bytes"
sha256sum "$output_image"
