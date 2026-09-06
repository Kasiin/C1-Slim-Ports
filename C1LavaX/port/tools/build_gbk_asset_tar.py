#!/usr/bin/env python3
"""Repack LavaXOS with GBK-encoded tar member names for the original VM ABI."""

from __future__ import annotations

import argparse
import datetime as dt
import io
import pathlib
import tarfile
import zipfile


PREFIX = "LavaX_a30/LavaXOS/"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_zip", type=pathlib.Path)
    parser.add_argument("output_tar", type=pathlib.Path)
    args = parser.parse_args()

    args.output_tar.parent.mkdir(parents=True, exist_ok=True)
    file_count = 0
    byte_count = 0
    with zipfile.ZipFile(args.source_zip) as source, tarfile.open(
        args.output_tar,
        mode="w",
        format=tarfile.GNU_FORMAT,
        encoding="gbk",
        errors="strict",
    ) as output:
        for entry in source.infolist():
            if not entry.filename.startswith(PREFIX):
                continue
            relative = entry.filename[len(PREFIX) :].replace("\\", "/")
            if not relative:
                continue
            parts = [part for part in relative.split("/") if part]
            if relative.startswith("/") or any(part in (".", "..") for part in parts):
                raise ValueError(f"unsafe archive member: {entry.filename!r}")

            name = "/".join(parts)
            is_directory = entry.is_dir()
            if is_directory:
                name += "/"
            info = tarfile.TarInfo(name)
            info.mtime = int(dt.datetime(*entry.date_time).timestamp())
            info.mode = 0o755 if is_directory else 0o644
            info.uid = 0
            info.gid = 0
            info.uname = "root"
            info.gname = "root"
            if is_directory:
                info.type = tarfile.DIRTYPE
                output.addfile(info)
                continue

            payload = source.read(entry)
            info.size = len(payload)
            output.addfile(info, io.BytesIO(payload))
            file_count += 1
            byte_count += len(payload)

    print(f"wrote {file_count} files, {byte_count} bytes: {args.output_tar}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
