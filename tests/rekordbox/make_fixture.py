#!/usr/bin/env python3
"""Make a synthetic USB export using the repository's DeviceSQL/ANLZ schemas.

No user music or export is modified. Output is a new test-media directory.
"""
import array
import math
from pathlib import Path
import re
import shutil
import struct
import sys
import wave

ROOT = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
be16 = lambda n: struct.pack('>H', n)
be32 = lambda n: struct.pack('>I', n)
le32 = lambda n: struct.pack('<I', n)


def section(kind, body, header=12):
    return kind.encode() + be32(header) + be32(12 + len(body)) + body


def anlz(tags):
    return b'PMAI' + be32(28) + be32(28 + len(tags)) + bytes(16) + tags


def bands(t, variant):
    low = .22 * (.15 + math.exp(-(t % .5) * 16))
    mid = .09 * (1 + math.sin(t * .8 + variant)) / 2
    high = .07 * math.exp(-((t + .25) % .5) * 22)
    return low, mid, high


def wave_tag(kind, columns, variant):
    data = bytearray()
    for i in range(columns):
        low, mid, high = bands(i * 60 / columns, variant)
        data.extend(round(v * 450) for v in (mid, high, low))
    body = be32(3) + be32(columns)
    if kind == 'PWV7':
        body += be32(0)
    return section(kind, body + data, 24 if kind == 'PWV7' else 20)


def rgb_tag(kind, columns, variant):
    data = bytearray()
    colors = [(7, 2, 1), (7, 0, 5), (1, 7, 3), (0, 3, 7)]
    for i in range(columns):
        t = i * 60 / columns
        r, g, b = colors[(int(t / 15) + variant) % len(colors)]
        height = round(12 + 19 * math.exp(-(t % .5) * 12))
        if kind == 'PWV4':
            data.extend((0, 0, 0, r * height // 2, g * height // 2, b * height // 2))
        else:
            data.extend(be16((r << 13) | (g << 10) | (b << 7) | (height << 2)))
    return section(kind, be32(6 if kind == 'PWV4' else 2) + be32(columns) + be32(0) + data, 24)


def cue_tags():
    tags = b''
    for list_type, cues in [(1, [(1, 4100, 0, 'A START'), (2, 16100, 0, 'B CHORUS'),
                                (3, 32100, 34100, 'C LOOP'), (4, 48100, 0, 'D OUTRO')]),
                            (0, [(0, 8100, 0, 'MEM 1'), (0, 24100, 0, 'MEM 2'),
                                 (0, 40100, 44100, 'MEM LOOP'), (0, 56100, 0, 'MEM 4')])]:
        body = be32(list_type) + be16(len(cues)) + bytes(2)
        colors = [(244, 64, 64), (13, 220, 242), (63, 236, 43), (230, 104, 28)]
        for index, (hot, start, end, label) in enumerate(cues):
            comment = (label + '\0').encode('utf-16-be')
            entry = b'PCP2' + be32(16) + be32(48 + len(comment)) + be32(hot)
            entry += bytes([2 if end else 1]) + bytes(3) + be32(start) + be32(end)
            entry += bytes([index + 2]) + bytes(7) + be16(0) + be16(0) + be32(len(comment)) + comment
            entry += bytes([1, *colors[index]])
            body += entry
        tags += section('PCO2', body)
    return tags


def analysis(variant):
    times = [100 + i * 500 for i in range(121)]
    grid = bytes(8) + be32(len(times))
    for i, ms in enumerate(times):
        grid += be16(i % 4 + 1) + be16(12000) + be32(ms)
    starts = [1, 17, 33, 49, 65, 81, 97, 113]
    kinds = [1, 2, 9, 8, 9, 3, 10, 10]
    body = be16(2) + bytes(6) + be16(121) + bytes(4)
    for i, (start, kind) in enumerate(zip(starts, kinds)):
        body += be16(i + 1) + be16(start) + be16(kind) + bytes(14) + bytes([0, 1]) + be16(start + 4)
    phrase = be32(24) + be16(len(starts)) + body
    return anlz(section('PQTZ', grid)), anlz(section('PSSI', phrase) + cue_tags() + rgb_tag('PWV4', 1200, variant) + rgb_tag('PWV5', 9000, variant)), anlz(wave_tag('PWV6', 1200, variant) + wave_tag('PWV7', 9000, variant))


def string(value):
    data = value.encode('ascii')
    if len(data) <= 126:
        return bytes([2 * (len(data) + 1) + 1]) + data
    return b'\x40' + struct.pack('<H', len(data) + 4) + bytes(1) + data


# Derive the fixed field order directly from the checked-in generated parser.
parser = (ROOT / 'lib/rekordbox-metadata/rekordbox_pdb.cpp').read_text()
track_read = parser.split('void rekordbox_pdb_t::track_row_t::_read() {', 1)[1].split('m_ofs_strings =', 1)[0]
fields = re.findall(r'm_(\w+) = m__io->read_u([124])(?:le)?\(\);', track_read)
assert len(fields) == 31
rows = []
(out / 'Contents').mkdir(exist_ok=True)
for index, title in enumerate(['RB Intro and Chorus', 'RB Two Deck Test', 'Native Fallback', 'Damaged Export Fallback'], 1):
    audio_path = out / f'Contents/fixture{index}.wav'
    if index <= 2:
        pcm = array.array('h')
        for i in range(60 * 44100):
            t = i / 44100
            low, mid, high = bands(t, index)
            sample = int(32767 * (low * math.sin(2 * math.pi * 80 * t) + mid * math.sin(2 * math.pi * 1000 * t) + high * math.sin(2 * math.pi * 6000 * t)))
            pcm.extend((sample, sample))
        if sys.byteorder != 'little':
            pcm.byteswap()
        with wave.open(str(audio_path), 'wb') as wav:
            wav.setparams((2, 2, 44100, 0, 'NONE', 'not compressed'))
            wav.writeframes(pcm.tobytes())
    else:
        shutil.copyfile(out / 'Contents/fixture1.wav', audio_path)
    relative = f'/PIONEER/USBANLZ/fixture{index}/ANLZ0000'
    base = out / relative.lstrip('/')
    base.parent.mkdir(parents=True, exist_ok=True)
    dat, ext, detail = analysis(index)
    base.with_suffix('.DAT').write_bytes(dat)
    if index != 3:
        base.with_suffix('.EXT').write_bytes(ext)
        base.with_suffix('.2EX').write_bytes(detail if index != 4 else anlz(section('PWV7', bytes(2))))
    values = dict(sample_rate=44100, file_size=audio_path.stat().st_size,
                  bitrate=1411, track_number=index, tempo=12000, id=index,
                  disc_number=1, year=2026, sample_depth=16, duration=60)
    prefix = b''.join(struct.pack('<' + {'1': 'B', '2': 'H', '4': 'I'}[width], values.get(name, 0)) for name, width in fields)
    texts = {14: relative + '.DAT', 17: title, 19: audio_path.name, 20: '/Contents/' + audio_path.name}
    offsets, strings = [], b''
    for n in range(21):
        offsets.append(len(prefix) + 42 + len(strings))
        strings += string(texts.get(n, ''))
    rows.append(prefix + struct.pack('<21H', *offsets) + strings)

page_size = 4096
header = struct.pack('<7I', 0, page_size, 1, 2, 0, 1, 0)
header += struct.pack('<4I', 0, 2, 1, 1)  # tracks, empty, first, last
header = header.ljust(page_size, b'\0')
page = bytearray(page_size)
page[:24] = struct.pack('<6I', 0, 1, 0, 2, 1, 0)
page[24:27] = (len(rows) | (len(rows) << 13)).to_bytes(3, 'little')
page[27] = 0x24
cursor = 40
for i, row in enumerate(rows):
    struct.pack_into('<H', page, page_size - 6 - 2 * i, cursor - 40)
    page[cursor:cursor+len(row)] = row
    cursor += len(row)
struct.pack_into('<H', page, page_size - 4, (1 << len(rows)) - 1)
struct.pack_into('<HH', page, 28, page_size - cursor - 36, cursor - 40)
pdb = out / 'PIONEER/rekordbox/export.pdb'
pdb.parent.mkdir(parents=True, exist_ok=True)
pdb.write_bytes(header + page)
print(out)
