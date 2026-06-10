#!/usr/bin/env python3
import argparse
import pathlib
import subprocess

def run_ffmpeg(input_path: pathlib.Path, output_path: pathlib.Path, rate: int, seconds: float) -> None:
    cmd = [
        "ffmpeg",
        "-y",
        "-i",
        str(input_path),
        "-ac",
        "1",
        "-ar",
        str(rate),
        "-f",
        "s16le",
        "-t",
        f"{seconds}",
        str(output_path),
    ]
    subprocess.run(cmd, check=True)


def write_c_array(pcm_path: pathlib.Path, header_path: pathlib.Path, source_path: pathlib.Path) -> None:
    data = pcm_path.read_bytes()

    header_path.write_text(
        "#ifndef AUDIO_PCM_H\n"
        "#define AUDIO_PCM_H\n\n"
        "#include <stdint.h>\n\n"
        "extern const uint8_t audio_pcm[];\n"
        "extern const uint32_t audio_pcm_len_bytes;\n\n"
        "#endif\n",
        encoding="ascii",
    )

    lines = []
    for i, b in enumerate(data):
        if i % 16 == 0:
            lines.append("    ")
        lines[-1] += f"0x{b:02X}"
        if i != len(data) - 1:
            lines[-1] += ", "
        if i % 16 == 15:
            lines[-1] += "\n"

    source_path.write_text(
        "#include \"inc/audio_pcm.h\"\n\n"
        "const uint8_t audio_pcm[] = {\n"
        + "".join(lines)
        + "\n};\n\n"
        "const uint32_t audio_pcm_len_bytes = sizeof(audio_pcm);\n",
        encoding="ascii",
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Convert MP3 to 16-bit mono PCM and generate C array")
    parser.add_argument("--input", required=True, help="Input MP3 path")
    parser.add_argument("--rate", type=int, default=8000, help="Sample rate, default 8000")
    parser.add_argument("--seconds", type=float, default=5.0, help="Duration to keep")
    parser.add_argument("--out-dir", default=".", help="Output directory")
    args = parser.parse_args()

    input_path = pathlib.Path(args.input).resolve()
    out_dir = pathlib.Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    pcm_path = out_dir / "audio_pcm_8k_mono_s16le.pcm"
    header_path = pathlib.Path("User/inc/audio_pcm.h")
    source_path = pathlib.Path("User/src/audio_pcm.c")

    run_ffmpeg(input_path, pcm_path, args.rate, args.seconds)
    write_c_array(pcm_path, header_path, source_path)

    print(f"Wrote {pcm_path}")
    print(f"Wrote {header_path} and {source_path}")


if __name__ == "__main__":
    main()
