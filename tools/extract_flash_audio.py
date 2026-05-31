#!/usr/bin/env python3
"""
Extract an audio payload from a flash image and export playable WAV.

Example:
  python3 tools/extract_flash_audio.py \
    --flash-image tools/work_flash_image_20260530_113504.bin \
    --offset 0x000000 --length 160000 --rate 8000 \
    --out-pcm tools/out/flash_audio_dump.pcm \
    --out-wav tools/out/flash_audio_dump.wav
"""

from __future__ import annotations

import argparse
import hashlib
import pathlib
import wave


def parse_int(value: str) -> int:
    return int(value, 0)


def main() -> None:
    parser = argparse.ArgumentParser(description="Extract audio bytes from flash image and write PCM/WAV")
    parser.add_argument("--flash-image", required=True, help="Path to full flash image binary")
    parser.add_argument("--offset", required=True, type=parse_int, help="Start offset in flash image (supports 0x)")
    parser.add_argument("--length", required=True, type=parse_int, help="Audio byte length to extract")
    parser.add_argument("--rate", type=int, default=8000, help="Sample rate, default 8000")
    parser.add_argument("--channels", type=int, default=1, help="Channel count, default 1")
    parser.add_argument("--sample-width", type=int, default=2, help="Bytes per sample, default 2 (s16le)")
    parser.add_argument("--out-pcm", required=True, help="Output raw PCM file")
    parser.add_argument("--out-wav", required=True, help="Output WAV file")
    parser.add_argument("--compare", help="Optional original PCM to compare hash/contents")
    args = parser.parse_args()

    flash_image = pathlib.Path(args.flash_image)
    out_pcm = pathlib.Path(args.out_pcm)
    out_wav = pathlib.Path(args.out_wav)

    blob = flash_image.read_bytes()
    start = args.offset
    end = start + args.length
    if start < 0 or end > len(blob):
        raise ValueError(f"range out of bounds: offset={start} length={args.length} image_size={len(blob)}")

    payload = blob[start:end]
    out_pcm.parent.mkdir(parents=True, exist_ok=True)
    out_wav.parent.mkdir(parents=True, exist_ok=True)

    out_pcm.write_bytes(payload)

    with wave.open(str(out_wav), "wb") as wf:
        wf.setnchannels(args.channels)
        wf.setsampwidth(args.sample_width)
        wf.setframerate(args.rate)
        wf.writeframes(payload)

    print(f"Wrote PCM: {out_pcm} ({len(payload)} bytes)")
    print(f"Wrote WAV: {out_wav}")

    payload_md5 = hashlib.md5(payload).hexdigest()
    print(f"Extracted MD5: {payload_md5}")

    if args.compare:
        cmp_path = pathlib.Path(args.compare)
        cmp_data = cmp_path.read_bytes()
        cmp_trim = cmp_data[: len(payload)]
        cmp_md5 = hashlib.md5(cmp_trim).hexdigest()
        print(f"Compare MD5  : {cmp_md5}")
        if cmp_trim == payload:
            print("COMPARE: MATCH")
        else:
            print("COMPARE: DIFFER")


if __name__ == "__main__":
    main()
