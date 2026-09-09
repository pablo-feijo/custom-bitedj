#!/usr/bin/env python3
"""Generate a large synthetic DeviceSQL export, including sparse sort positions.

Creates a new output directory only; never accepts an existing music directory.
No copyrighted music, user exports or analysis files are required.
"""
import argparse
from pathlib import Path
import re
import struct
import wave


def string(value):
    data = value.encode("ascii")
    if len(data) <= 126:
        return bytes([2 * (len(data) + 1) + 1]) + data
    return b"\x40" + struct.pack("<H", len(data) + 4) + b"\0" + data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--tracks", type=int, default=10000)
    args = parser.parse_args()
    if not 1 <= args.tracks <= 100000:
        parser.error("--tracks must be between 1 and 100000")
    args.output.mkdir(parents=True, exist_ok=False)
    contents = args.output / "Contents"
    contents.mkdir()
    root = Path(__file__).resolve().parents[2]
    source = (root / "lib/rekordbox-metadata/rekordbox_pdb.cpp").read_text()
    read = source.split("void rekordbox_pdb_t::track_row_t::_read() {", 1)[1].split("m_ofs_strings =", 1)[0]
    fields = re.findall(r"m_(\w+) = m__io->read_u([124])(?:le)?\(\);", read)
    assert len(fields) == 31
    rows = []
    for index in range(1, args.tracks + 1):
        name = f"stress{index:06}.wav"
        with wave.open(str(contents / name), "wb") as wav:
            wav.setparams((1, 2, 8000, 0, "NONE", "not compressed"))
            wav.writeframes(bytes(1600))
        values = dict(sample_rate=8000, file_size=1644, bitrate=128,
                      track_number=index % 65536, id=index, disc_number=1,
                      year=2026, sample_depth=16, duration=1)
        prefix = b"".join(struct.pack("<" + {"1": "B", "2": "H", "4": "I"}[width],
                                      values.get(name, 0)) for name, width in fields)
        texts = {14: f"/PIONEER/USBANLZ/stress{index}/ANLZ.DAT",
                 17: f"Stress Track {index:06}", 19: name, 20: f"/Contents/{name}"}
        offsets, strings = [], b""
        for n in range(21):
            offsets.append(len(prefix) + 42 + len(strings))
            strings += string(texts.get(n, ""))
        rows.append(prefix + struct.pack("<21H", *offsets) + strings)

    # Deliberately non-dense ordering: old operator[] loops grow their maps
    # across the gaps instead of visiting just the two real tree entries.
    tree = [struct.pack("<5I", 0, 0, 1000000000, 10, 1) + string("Stress Folder"),
            struct.pack("<5I", 10, 0, 2000000000, 11, 0) + string("Stress Playlist")]
    entries = [struct.pack("<3I", index * 1000, index, 11)
               for index in range(1, args.tracks + 1)]
    page_size = 4096
    pages, descriptors = [], []
    for kind, table in [(0, rows), (7, tree), (8, entries)]:
        first = len(pages) + 1
        for start in range(0, len(table), 8):
            chunk = table[start:start + 8]
            page_index = len(pages) + 1
            page = bytearray(page_size)
            page[:24] = struct.pack("<6I", 0, page_index, kind, page_index + 1, 1, 0)
            page[24:27] = (len(chunk) | (len(chunk) << 13)).to_bytes(3, "little")
            page[27] = 0x24
            cursor = 40
            for i, row in enumerate(chunk):
                struct.pack_into("<H", page, page_size - 6 - 2 * i, cursor - 40)
                page[cursor:cursor + len(row)] = row
                cursor += len(row)
            assert cursor < page_size - 36
            struct.pack_into("<H", page, page_size - 4, (1 << len(chunk)) - 1)
            struct.pack_into("<HH", page, 28, page_size - cursor - 36, cursor - 40)
            pages.append(page)
        descriptors.append(struct.pack("<4I", kind, len(pages) + 1, first, len(pages)))
    header = struct.pack("<7I", 0, page_size, 3, len(pages) + 1, 0, 1, 0)
    header = (header + b"".join(descriptors)).ljust(page_size, b"\0")
    pdb = args.output / "PIONEER/rekordbox/export.pdb"
    pdb.parent.mkdir(parents=True)
    pdb.write_bytes(header + b"".join(pages))
    print(f"{args.tracks} tracks, sparse playlist, {pdb.stat().st_size} byte database: {args.output}")


if __name__ == "__main__":
    main()
