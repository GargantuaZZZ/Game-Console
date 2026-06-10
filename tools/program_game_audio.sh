#!/usr/bin/env bash
set -euo pipefail

# Convert all game MP3 files to 8 kHz mono signed 16-bit little-endian PCM,
# assemble the external-flash image expected by User/src/main.c, then program it.
#
# Usage:
#   ./tools/program_game_audio.sh <mp3-directory> [--build-only]
#
# Required files in <mp3-directory>:
#   welcome.mp3
#   difficulty_1.mp3 ... difficulty_7.mp3
#   start.mp3
#   level_1.mp3 ... level_3.mp3
#   bgm_1.mp3 ... bgm_3.mp3
#   win.mp3
#   lose.mp3
#   win_all.mp3
#   hidden_win.mp3
#
# Environment variables accepted by tools/program_spif.sh are also supported:
#   FLASH_CHIP="W25Q64JV-.Q"
#   PROGRAMMER_OPTS="spispeed=250"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "Usage: $0 <mp3-directory> [--build-only]"
  exit 2
fi

INPUT_DIR="$(cd "$1" 2>/dev/null && pwd)" || {
  echo "Input directory not found: $1"
  exit 2
}

BUILD_ONLY=0
if [ "$#" -eq 2 ]; then
  if [ "$2" != "--build-only" ]; then
    echo "Unknown option: $2"
    exit 2
  fi
  BUILD_ONLY=1
fi

command -v ffmpeg >/dev/null 2>&1 || {
  echo "ffmpeg not found in PATH. Install it first (brew install ffmpeg)."
  exit 2
}

OUTPUT_DIR="$ROOT_DIR/tools/out/game_audio"
IMAGE="$OUTPUT_DIR/game_audio_flash.bin"
MANIFEST="$OUTPUT_DIR/game_audio_flash_layout.txt"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/game-audio.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT

mkdir -p "$OUTPUT_DIR"

# End of hidden_win: (0xCE7 + 47) * 1024 = 0x345800.
IMAGE_SIZE=$((0x345800))

# Create a zero-filled image. Signed 16-bit PCM silence is 0x0000.
dd if=/dev/zero of="$IMAGE" bs=1024 count=$((IMAGE_SIZE / 1024)) 2>/dev/null

convert_slot() {
  local name="$1"
  local file="$2"
  local offset="$3"
  local size="$4"
  local duration
  local pcm="$TMP_DIR/${name}.pcm"
  local actual_size

  if [ ! -f "$file" ]; then
    echo "Missing required MP3: $file"
    exit 2
  fi

  duration="$(awk -v bytes="$size" 'BEGIN { printf "%.6f", bytes / 16000.0 }')"
  echo "Converting $name: $(basename "$file") -> offset $offset, $size bytes"

  ffmpeg -hide_banner -loglevel error -y \
    -i "$file" \
    -ac 1 \
    -ar 8000 \
    -acodec pcm_s16le \
    -af "apad=pad_dur=${duration}" \
    -t "$duration" \
    -f s16le \
    "$pcm"

  actual_size="$(wc -c < "$pcm" | tr -d ' ')"
  if [ "$actual_size" -gt "$size" ]; then
    echo "Converted audio is too large: $name ($actual_size > $size bytes)"
    exit 1
  fi

  dd if="$pcm" of="$IMAGE" bs=1 seek=$((offset)) conv=notrunc 2>/dev/null

  printf "%-14s offset=0x%06X size=%7d duration=%7.3fs source=%s\n" \
    "$name" "$offset" "$size" "$(awk -v bytes="$size" 'BEGIN { print bytes / 16000.0 }')" \
    "$(basename "$file")" >> "$MANIFEST"
}

: > "$MANIFEST"
echo "Format: 8000 Hz, mono, signed 16-bit little-endian raw PCM" >> "$MANIFEST"
echo "Image size: $IMAGE_SIZE bytes (0x345800)" >> "$MANIFEST"
echo >> "$MANIFEST"

convert_slot "welcome"     "$INPUT_DIR/welcome.mp3"      $((0x000000)) $((48 * 1024))

convert_slot "difficulty_1" "$INPUT_DIR/difficulty_1.mp3" $((0x00C000)) $((16 * 1024))
convert_slot "difficulty_2" "$INPUT_DIR/difficulty_2.mp3" $((0x010000)) $((16 * 1024))
convert_slot "difficulty_3" "$INPUT_DIR/difficulty_3.mp3" $((0x014000)) $((16 * 1024))
convert_slot "difficulty_4" "$INPUT_DIR/difficulty_4.mp3" $((0x018000)) $((16 * 1024))
convert_slot "difficulty_5" "$INPUT_DIR/difficulty_5.mp3" $((0x01C000)) $((16 * 1024))
convert_slot "difficulty_6" "$INPUT_DIR/difficulty_6.mp3" $((0x020000)) $((16 * 1024))
convert_slot "difficulty_7" "$INPUT_DIR/difficulty_7.mp3" $((0x024000)) $((16 * 1024))

convert_slot "start"       "$INPUT_DIR/start.mp3"        $((0x028000)) $((47 * 1024))
convert_slot "level_1"     "$INPUT_DIR/level_1.mp3"      $((0x033C00)) $((47 * 1024))
convert_slot "level_2"     "$INPUT_DIR/level_2.mp3"      $((0x03F800)) $((47 * 1024))
convert_slot "level_3"     "$INPUT_DIR/level_3.mp3"      $((0x04B400)) $((47 * 1024))

convert_slot "bgm_1"       "$INPUT_DIR/bgm_1.mp3"        $((0x057000)) $((938 * 1024))
convert_slot "bgm_2"       "$INPUT_DIR/bgm_2.mp3"        $((0x141800)) $((938 * 1024))
convert_slot "bgm_3"       "$INPUT_DIR/bgm_3.mp3"        $((0x22C000)) $((938 * 1024))
convert_slot "win"         "$INPUT_DIR/win.mp3"          $((0x316800)) $((47 * 1024))
convert_slot "lose"        "$INPUT_DIR/lose.mp3"         $((0x322400)) $((47 * 1024))
convert_slot "win_all"     "$INPUT_DIR/win_all.mp3"      $((0x32E000)) $((47 * 1024))
convert_slot "hidden_win"  "$INPUT_DIR/hidden_win.mp3"   $((0x339C00)) $((47 * 1024))

echo
echo "Built image: $IMAGE"
echo "Layout: $MANIFEST"

if [ "$BUILD_ONLY" -eq 1 ]; then
  echo "Build-only mode: flash programming skipped."
  exit 0
fi

echo
echo "Programming external SPI flash..."
cd "$ROOT_DIR"
"$SCRIPT_DIR/program_spif.sh" "$IMAGE" 0x000000
