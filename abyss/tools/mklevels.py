#!/usr/bin/env python3
"""levels/world<N>/NN_name.txt -> data/ABYSSLV<N>.bin (archived AppVar payload) + host/pack_levels.c (tests only)."""
import glob, struct, os, sys
sys.path.insert(0, os.path.dirname(__file__))
CH = {'.':0,'#':1,'=':2,'^':3,'H':4,'$':5,'C':6,'E':7,'x':8,'R':9,'G':10,'T':11,'z':13,'S':0,'k':0,'m':0}
def crc16(d):
    c = 0xFFFF
    for b in d:
        c ^= b << 8
        for _ in range(8): c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c
def level_blob(f):
    L = open(f).read().splitlines(); meta = {}
    while ':' in L[0]: k, v = L.pop(0).split(':', 1); meta[k.strip()] = v.strip()
    rows = L; h = len(rows); w = len(rows[0]); spawn = None; ents = []
    assert all(len(r) == w for r in rows), f"{f}: ragged rows"
    assert 8 <= w <= 96 and h == 15, f"{f}: bad size {w}x{h}"
    data = []
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c == 'S': spawn = (x, y)
            if c == 'k': ents.append((0, x, y))
            if c == 'm': ents.append((1, x, y))
            data.append(CH[c])
    assert spawn, f"{f}: no spawn"
    name = meta['name'].encode() + b'\0'
    return bytes([int(meta['world']), w, h, spawn[0], spawn[1], int(meta.get('boss', 0)), len(ents), len(name)]) + name + bytes(b for e in ents for b in e) + bytes(data)
def build_pack(kind, items):
    n = len(items); off = 14 + 4 * n; offs = []; body = b''
    for it in items: offs.append(off + len(body)); body += it
    payload = b''.join(struct.pack('<I', o) for o in offs) + body
    return b'ABYP' + bytes([kind, 2]) + struct.pack('<HHI', n, crc16(payload), 14 + len(payload)) + payload
packs = {}
for w in range(1, 5):
    files = sorted(glob.glob(f'levels/world{w}/*.txt'))
    if not files: continue
    assert len(files) <= 5
    packs[w] = build_pack(1, [level_blob(f) for f in files])
    open(f'data/ABYSSLV{w}.bin', 'wb').write(packs[w])
for w in range(1, 5):
    if w not in packs and os.path.exists(f'data/ABYSSLV{w}.bin'): os.remove(f'data/ABYSSLV{w}.bin')
with open('host/pack_levels.c', 'w') as f:
    f.write('/* GENERATED - host tests only */\n#include <stdint.h>\n')
    for w, b in packs.items(): f.write(f'static const uint8_t PACK_LV{w}[{len(b)}] = {{' + ','.join(map(str, b)) + '};\n')
    f.write('const uint8_t *host_level_pack(int w, uint32_t *len) {\n  switch (w) {\n')
    for w, b in packs.items(): f.write(f'    case {w}: *len = {len(b)}; return PACK_LV{w};\n')
    f.write('    default: return 0;\n  }\n}\n')
print('level packs:', {w: len(b) for w, b in packs.items()}, '(AppVar limit 65,535)')
