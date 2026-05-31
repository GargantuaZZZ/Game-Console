#!/usr/bin/env bash
set -euo pipefail

# program_spif.sh - write a raw binary to SPI flash via CH341A/B using flashrom
# Usage: ./tools/program_spif.sh <input-file> [offset_hex]
# Example: ./tools/program_spif.sh tools/out/audio_pcm_8k_mono_s16le.pcm 0x000000
#
# Optional env vars:
#   FLASH_CHIP="W25Q64.V"           # force chip model if auto-detect fails
#   PROGRAMMER_OPTS="spispeed=250"  # slower SPI may help long Dupont wires

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <input-file> [offset_hex]"
  exit 2
fi

INPUT_FILE="$1"
OFFSET=${2:-0x0}
PROGRAMMER="ch341a_spi"
PROGRAMMER_OPTS=${PROGRAMMER_OPTS:-}
PROG_SPEC="${PROGRAMMER}${PROGRAMMER_OPTS:+:${PROGRAMMER_OPTS}}"
FLASH_CHIP=${FLASH_CHIP:-}
BACKUP_FILE="tools/backup_flash_$(date +%Y%m%d_%H%M%S).bin"

command -v flashrom >/dev/null 2>&1 || { echo "flashrom not found in PATH. Install it first (brew install flashrom)"; exit 2; }
[ -f "$INPUT_FILE" ] || { echo "Input file not found: $INPUT_FILE"; exit 2; }

RUN=(flashrom)
if [ "$(id -u)" -ne 0 ]; then
  # macOS commonly needs sudo for USB capture with libusb
  RUN=(sudo flashrom)
fi

run_flashrom() {
  if [ -n "$FLASH_CHIP" ]; then
    "${RUN[@]}" -p "$PROG_SPEC" -c "$FLASH_CHIP" "$@"
  else
    "${RUN[@]}" -p "$PROG_SPEC" "$@"
  fi
}

echo "Input file: $INPUT_FILE"
echo "Offset: $OFFSET"
echo "Programmer: $PROG_SPEC"
if [ -n "$FLASH_CHIP" ]; then
  echo "Chip (forced): $FLASH_CHIP"
else
  echo "Chip: auto-detect"
fi

echo "Probing flash chip first..."
run_flashrom --flash-name

echo "Backing up entire chip to $BACKUP_FILE (may take a while)..."
run_flashrom -r "$BACKUP_FILE"

echo "Writing $INPUT_FILE to flash (this will overwrite chip contents starting at offset $OFFSET)..."
# Write: flashrom can write image smaller than chip (it programs starting at 0).
# Some flashrom versions support --offset; use if available.
if flashrom --help | grep -q -- "--offset"; then
  run_flashrom -w "$INPUT_FILE" --offset "$OFFSET"
else
  echo "flashrom without --offset detected; composing full-chip image from backup + payload..."
  OFFSET_DEC=$((OFFSET))
  PAYLOAD_SIZE=$(wc -c < "$INPUT_FILE")
  CHIP_SIZE=$(wc -c < "$BACKUP_FILE")

  if [ "$OFFSET_DEC" -lt 0 ]; then
    echo "Invalid offset: $OFFSET"
    exit 2
  fi

  END_POS=$((OFFSET_DEC + PAYLOAD_SIZE))
  if [ "$END_POS" -gt "$CHIP_SIZE" ]; then
    echo "Payload out of range: offset($OFFSET_DEC) + size($PAYLOAD_SIZE) > chip_size($CHIP_SIZE)"
    exit 2
  fi

  WORK_IMAGE="tools/work_flash_image_$(date +%Y%m%d_%H%M%S).bin"
  cp "$BACKUP_FILE" "$WORK_IMAGE"
  dd if="$INPUT_FILE" of="$WORK_IMAGE" bs=1 seek="$OFFSET_DEC" conv=notrunc status=none

  echo "Programming full image ($CHIP_SIZE bytes)..."
  run_flashrom -w "$WORK_IMAGE"
fi

echo "Verifying written image..."
if flashrom --help | grep -q -- "--offset"; then
  run_flashrom -v "$INPUT_FILE" --offset "$OFFSET"
else
  run_flashrom -v "$WORK_IMAGE"
fi

echo "Done. Keep $BACKUP_FILE for restore and $WORK_IMAGE for audit if needed."
