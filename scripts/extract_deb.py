#!/usr/bin/env python3
"""Extract Debian .deb data.tar.* into sysroot (stdlib only)."""
import io
import struct
import sys
import tarfile
from pathlib import Path


def iter_ar(path: Path):
    data = path.read_bytes()
    pos = 0
    while pos + 60 <= len(data):
        header = data[pos : pos + 60]
        name = header[0:16].decode("ascii", errors="replace").strip()
        size_str = header[48:58].decode("ascii").strip()
        if not size_str:
            break
        size = int(size_str)
        pos += 60
        payload = data[pos : pos + size]
        pos += size + (size % 2)
        yield name, payload


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: extract_deb.py package.deb sysroot/", file=sys.stderr)
        return 1
    deb = Path(sys.argv[1])
    root = Path(sys.argv[2])
    root.mkdir(parents=True, exist_ok=True)
    for name, payload in iter_ar(deb):
        if not name.startswith("data.tar"):
            continue
        with tarfile.open(fileobj=io.BytesIO(payload), mode="r:*") as tf:
            for member in tf.getmembers():
                if not (member.name.startswith("usr/") or member.name.startswith("./usr/")):
                    continue
                member = tf.getmember(member.name)
                member.name = member.name.lstrip("./")
                tf.extract(member, root, filter="data")
        break
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
