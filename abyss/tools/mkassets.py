#!/usr/bin/env python3
"""Procedural pixel-art generator -> data/ABYSSCOR.bin (AppVar payload) + include/sprite_ids.h + host/pack_core.c (tests only).
All sprites are indexed 8-bit (index 0 = transparent). Palette indices 1..11 are the per-world THEME colours (recoloured at runtime)."""
import math, struct
PAL = [
 ('clear',0,0,0),('bg1',8,10,24),('bg2',18,22,48),('bg3',30,36,70),('stone_d',50,54,74),('stone',84,90,116),
 ('stone_l',128,136,160),('stone_h',170,178,196),('purple',96,64,128),('purple_d',56,36,80),('moss',110,160,120),('moss_d',62,100,76),
 ('skin',236,200,160),('skin_d',190,150,120),('cloak',54,74,170),('cloak_l',90,120,220),('cloak_d',30,40,100),('rune',90,240,240),
 ('rune_d',30,140,160),('red',220,50,50),('red_d',120,20,30),('gold',255,214,70),('gold_d',180,130,30),('white',255,255,255),
 ('brown',130,86,44),('brown_d',80,50,24),('bone',230,226,200),('bone_d',170,164,140),('outline',10,10,22),('fire',255,150,40),
 ('fire_l',255,230,120),('green',80,200,130)]
THEMES = [   # colours for palette indices 1..11 (bg1,bg2,bg3,stone_d,stone,stone_l,stone_h,purple,purple_d,moss,moss_d)
 [(8,10,24),(18,22,48),(30,36,70),(50,54,74),(84,90,116),(128,136,160),(170,178,196),(96,64,128),(56,36,80),(110,160,120),(62,100,76)],      # W1 crypt
 [(6,16,22),(12,34,44),(20,56,70),(52,40,30),(92,72,52),(140,110,76),(190,160,120),(60,150,160),(30,84,96),(110,230,230),(50,150,170)],   # W2 mines
 [(16,4,4),(44,10,8),(80,20,10),(40,28,28),(78,52,48),(120,80,66),(170,120,90),(150,60,40),(90,30,24),(255,150,40),(190,70,20)],        # W3 inferno
 [(4,2,12),(22,10,48),(48,20,90),(40,26,70),(76,52,120),(120,90,180),(180,150,240),(150,90,255),(80,40,150),(200,160,255),(120,70,200)]] # W4 void
I = {n: i for i, (n, *_) in enumerate(PAL)}
def H(x, y, s=0): return ((x * 73856093) ^ (y * 19349663) ^ (s * 83492791)) & 0xFFFF

class C:
    def __init__(s, w, h): s.w, s.h = w, h; s.p = [[0] * w for _ in range(h)]
    def px(s, x, y, c):
        if 0 <= x < s.w and 0 <= y < s.h: s.p[y][x] = I[c] if isinstance(c, str) else c
    def rect(s, x, y, w, h, c):
        for j in range(h):
            for i in range(w): s.px(x + i, y + j, c)
    def hline(s, x, y, w, c): s.rect(x, y, w, 1, c)
    def vline(s, x, y, h, c): s.rect(x, y, 1, h, c)
    def ellipse(s, cx, cy, rx, ry, c):
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            for x in range(int(cx - rx - 1), int(cx + rx + 2)):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0: s.px(x, y, c)
    def line(s, x0, y0, x1, y1, c):
        dx, dy = abs(x1 - x0), -abs(y1 - y0); sx = 1 if x0 < x1 else -1; sy = 1 if y0 < y1 else -1; e = dx + dy
        while True:
            s.px(x0, y0, c)
            if x0 == x1 and y0 == y1: break
            e2 = 2 * e
            if e2 >= dy: e += dy; x0 += sx
            if e2 <= dx: e += dx; y0 += sy
    def outline(s, c='outline'):
        o = [r[:] for r in s.p]
        for y in range(s.h):
            for x in range(s.w):
                if s.p[y][x] != 0: continue
                for dx, dy in ((1,0),(-1,0),(0,1),(0,-1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < s.w and 0 <= ny < s.h and s.p[ny][nx] not in (0, I[c]): o[y][x] = I[c]
        s.p = o
    def flip(s): n = C(s.w, s.h); n.p = [r[::-1] for r in s.p]; return n
    def data(s): return [s.w, s.h] + [v for r in s.p for v in r]

# ================= tiles =================
def brick(variant=0, top=False, deep=False, under=False):
    c = C(16, 16); mortar = 'bg3' if deep else 'stone_d'
    c.rect(0, 0, 16, 16, mortar)
    base, hi, lo = ('stone_d', 'stone', 'bg3') if deep else ('stone', 'stone_l', 'stone_d')
    for ci, y0 in enumerate((0, 8)):
        off = 0 if (ci + variant) % 2 == 0 else 4
        for k in range(-1, 3):
            x0 = off + 8 * k; t = H(k, ci, variant + 3) % 5
            col = hi if t == 0 else lo if t == 1 else base
            for y in range(y0, y0 + 7):
                for x in range(x0, x0 + 7): c.px(x, y, col)
            hcol = 'stone_h' if (col == hi and not deep) else hi
            c.hline(x0, y0, 7, hcol); c.vline(x0, y0, 7, hcol)
            c.px(x0 + 6, y0 + 6, lo)
            if H(k, ci, variant) % 3 == 0: c.px(x0 + 2 + H(k, 5, 1) % 3, y0 + 2 + H(ci, 4, 2) % 3, lo)
    if under:
        for x in range(16):
            c.px(x, 15, 'bg3'); c.px(x, 14, 'stone_d' if H(x, 1, 9) % 2 else 'stone')
    if top:
        for x in range(16):
            d = 2 + (H(x, 0, variant) % 3 == 0) + (H(x, 1, 5) % 4 == 0) + (H(x, 2, 7) % 9 == 0)
            for y in range(d): c.px(x, y, 'moss' if y < d - 1 else 'moss_d')
            if H(x, 3, 1) % 5 == 0: c.px(x, 0, 'stone_h')
    return c
def crumble():
    c = brick(2); 
    for y in range(16):
        for x in range(16):
            if H(x, y, 8) % 17 == 0: c.px(x, y, 'stone_h')
    for (x, y) in [(3,1),(4,2),(4,3),(5,4),(6,5),(6,6),(7,7),(7,8),(8,9),(9,10),(9,11),(11,4),(10,5),(10,6),(9,7),(12,10),(12,11),(13,12)]: c.px(x, y, 'outline')
    for (x, y) in [(0,0),(15,0),(0,15),(15,15),(1,0),(0,1),(14,15),(15,14)]: c.px(x, y, 'bg3')
    return c
def oneway():
    c = C(16, 16)
    c.rect(0, 0, 16, 5, 'purple_d'); c.hline(0, 0, 16, 'purple'); c.hline(0, 1, 16, 'stone_l'); c.hline(0, 4, 16, 'outline')
    for x in (2, 8, 14): c.px(x, 2, 'stone_h'); c.px(x, 3, 'bg3')
    c.px(0, 5, 'outline'); c.px(15, 5, 'outline'); c.px(1, 5, 'purple_d'); c.px(14, 5, 'purple_d')
    return c
def spike():
    c = C(16, 16)
    for i in range(3):
        cx = 3 + i * 5
        for y in range(5, 16):
            w = 1 + (y - 5) * 4 // 10; x0 = cx - w // 2
            for x in range(x0, x0 + w): c.px(x, y, 'stone_l' if x >= cx else 'stone_h')
        c.px(cx, 5, 'red'); c.px(cx, 6, 'red_d'); c.px(cx - 1, 8, 'white')
    c.hline(0, 15, 16, 'stone_d')
    return c
def ladder():
    c = C(16, 16)
    for x in (3, 11): c.vline(x, 0, 16, 'brown'); c.vline(x + 1, 0, 16, 'brown_d')
    for y in (2, 7, 12): c.hline(3, y, 10, 'brown'); c.hline(3, y + 1, 10, 'brown_d')
    return c
def coin(f):
    c = C(16, 16); hw = [5, 3, 1, 3][f]
    c.ellipse(8, 8, hw + .5, 5.5, 'gold_d'); c.ellipse(8, 8, max(hw - 1, .5), 4.5, 'gold')
    if f in (0, 3): c.px(6, 5, 'white'); c.px(6, 6, 'fire_l')
    c.outline(); return c
def chest():
    c = C(16, 16)
    c.rect(1, 7, 14, 9, 'brown'); c.rect(1, 5, 14, 4, 'brown_d'); c.hline(2, 4, 12, 'brown_d')
    c.hline(1, 7, 14, 'outline'); c.vline(1, 5, 11, 'outline'); c.vline(14, 5, 11, 'outline'); c.hline(1, 15, 14, 'outline')
    for x in (3, 11): c.vline(x, 4, 12, 'gold_d')
    c.rect(7, 8, 2, 3, 'gold'); c.px(7, 8, 'fire_l'); c.hline(2, 5, 12, 'brown'); c.hline(2, 12, 12, 'brown_d')
    return c
def door(f):
    c = C(16, 16); c.rect(1, 0, 14, 16, 'stone_l'); c.rect(2, 0, 12, 16, 'stone')
    c.rect(3, 3, 10, 13, 'green'); c.rect(4, 2, 8, 1, 'green'); c.rect(5, 1, 6, 1, 'green')
    for y in range(3, 16): c.hline(3, y, 10, 'green' if (y + f) % 2 else 'rune_d')
    c.rect(6, 5, 4, 11, 'white'); c.rect(7, 4, 2, 1, 'white')
    for (x, y) in [(4,5),(11,8),(5,12),(10,4)] if f == 0 else [(11,5),(4,8),(10,12),(5,4)]: c.px(x, y, 'white')
    c.vline(1, 0, 16, 'outline'); c.vline(14, 0, 16, 'outline'); c.hline(1, 0, 14, 'stone_h')
    return c
def rune(on):
    c = C(16, 16)
    c.rect(4, 12, 8, 4, 'stone'); c.hline(4, 12, 8, 'stone_h'); c.vline(4, 12, 4, 'outline'); c.vline(11, 12, 4, 'outline')
    col = 'rune' if on else 'purple'; dk = 'rune_d' if on else 'purple_d'
    for (x, y) in [(8,2),(7,3),(9,3),(6,4),(10,4),(5,5),(11,5),(6,6),(10,6),(7,7),(9,7),(8,8)]: c.px(x, y, col)
    for (x, y) in [(8,4),(8,5),(8,6),(7,5),(9,5)]: c.px(x, y, dk)
    c.vline(8, 9, 3, dk)
    if on:
        for (x, y) in [(3,4),(13,4),(4,8),(12,8),(8,0)]: c.px(x, y, 'white')
        c.px(8, 5, 'white')
    return c
def gate():
    c = C(16, 16)
    for x in (1, 6, 11): c.rect(x, 0, 3, 16, 'stone_d'); c.vline(x, 0, 16, 'stone_l'); c.vline(x + 2, 0, 16, 'outline')
    c.rect(0, 3, 16, 2, 'purple'); c.rect(0, 11, 16, 2, 'purple'); c.hline(0, 3, 16, 'stone_h'); c.hline(0, 11, 16, 'stone_h')
    for y in (4, 12): c.px(7, y, 'rune'); c.px(8, y, 'rune')
    return c
def torch(f):
    c = C(16, 16); c.rect(7, 8, 2, 8, 'brown'); c.hline(6, 8, 4, 'brown_d'); c.vline(8, 9, 7, 'brown_d')
    h = 6 if f == 0 else 5; sh = 0 if f == 0 else 1
    c.ellipse(8 + sh, 5, 3, h / 2 + 1, 'fire'); c.ellipse(8 + sh, 6, 1.5, 2.5, 'fire_l')
    c.px(8 - sh, 1, 'fire')
    return c
def skull():
    c = C(16, 16)
    c.ellipse(8, 11, 4, 3.5, 'bone'); c.rect(6, 13, 5, 2, 'bone'); c.hline(6, 15, 5, 'bone_d')
    c.rect(5, 10, 2, 2, 'outline'); c.rect(9, 10, 2, 2, 'outline'); c.px(8, 12, 'outline')
    for x in (6, 8, 10): c.px(x, 14, 'outline')
    c.hline(1, 15, 4, 'bone'); c.hline(12, 15, 3, 'bone_d'); c.px(0, 14, 'bone'); c.px(15, 14, 'bone')
    return c
def glow():
    c = C(32, 32); bayer = [[0,8,2,10],[12,4,14,6],[3,11,1,9],[15,7,13,5]]
    for y in range(32):
        for x in range(32):
            r = math.hypot(x - 15.5, y - 15.5) / 16
            if r < 1 and bayer[y % 4][x % 4] < (1 - r) ** 1.4 * 11: c.px(x, y, 'fire_l' if r < .3 else 'fire')
    return c
# ================= backgrounds =================
def bg_arch():
    c = C(80, 112)
    for y in range(112):
        for x in range(80):
            if y < 40:
                r = math.hypot(x - 39.5, y - 40)
                if 28 <= r <= 40: c.px(x, y, 'bg3')
                if 27 <= r < 28 or 40 < r <= 41: c.px(x, y, 'bg2' if r < 28 else 'stone_d')
            elif x < 12 or x >= 68: c.px(x, y, 'bg3')
    for y in range(40, 112): c.px(11, y, 'bg2'); c.px(68, y, 'bg2'); c.px(0, y, 'stone_d'); c.px(79, y, 'stone_d')
    for y in range(40, 112, 8): c.hline(0, y, 12, 'bg2'); c.hline(68, y, 12, 'bg2')
    return c
def bg_pillar():
    c = C(20, 128)
    c.rect(4, 8, 12, 112, 'bg3'); c.rect(0, 0, 20, 8, 'bg3'); c.rect(0, 120, 20, 8, 'bg3')
    c.hline(0, 7, 20, 'stone_d'); c.hline(0, 120, 20, 'stone_d'); c.vline(4, 8, 112, 'stone_d'); c.vline(15, 8, 112, 'bg2')
    for x in (7, 10, 13): c.vline(x, 10, 108, 'bg2')
    for y in (0, 3): c.hline(0, y, 20, 'bg2')
    return c
def bg_chain():
    c = C(8, 40)
    for k in range(0, 40, 6):
        c.ellipse(4, k + 3, 2.5, 3.2, 'stone_d' if k % 12 == 0 else 'stone'); c.ellipse(4, k + 3, 1, 1.6, 0)
    return c

# ================= player =================
BODY = ["...oooooo...", "..occCCccco.", ".occcccccco.", ".occcssrsco.", ".occcssssco.", "..occcSSco..",
        ".occggggcco.", ".occccrccco.", ".occccRccco.", ".obbbbbbbbo.", ".occcdddccco"]
CH = {'.':0,'o':'outline','c':'cloak','C':'cloak_l','d':'cloak_d','s':'skin','S':'skin_d','r':'rune','R':'rune_d','g':'gold','b':'brown','B':'brown_d'}
def leg(c, x, h, bottom=15):
    top = bottom - h + 1
    c.rect(x, top, 4, h, 'outline'); c.rect(x + 1, top, 2, h - 1, 'brown_d'); c.px(x + 4, bottom, 'outline')
PFRAMES = ['idle0','idle1','run0','run1','run2','run3','jump','fall','climb0','climb1','land','hurt']
def player(name):
    c = C(12, 16)
    bob = 1 if name in ('idle1', 'run1', 'run3', 'land') else 0
    if name == 'land': bob = 2
    for y, row in enumerate(BODY):
        for x, ch in enumerate(row):
            if ch != '.': c.px(x, y + bob, CH[ch])
    if name.startswith('run') or name in ('jump', 'fall'):     # cape streams behind
        for (x, y) in [(0, 7), (0, 8), (1, 9)]: c.px(x, y + bob, 'cloak_d')
    if name == 'hurt':
        for (x, y) in [(6, 3), (7, 3)]: c.px(x, y, 'outline')
        c.px(8, 4, 'red')
    L = {'idle0': [(3,5,15),(7,5,15)], 'idle1': [(3,5,15),(7,5,15)], 'run0': [(2,5,15),(7,3,13)], 'run1': [(4,5,15),(6,5,15)],
         'run2': [(7,5,15),(2,3,13)], 'run3': [(4,5,15),(6,5,15)], 'jump': [(3,3,13),(7,3,13)], 'fall': [(2,5,15),(8,5,15)],
         'climb0': [(3,5,15),(7,3,13)], 'climb1': [(3,3,13),(7,5,15)], 'land': [(1,3,15),(8,3,15)], 'hurt': [(2,5,15),(8,5,15)]}[name]
    for (x, h, b) in L: leg(c, x, h, b)
    return c
# ================= creatures / bosses =================
def crawler(f):
    c = C(14, 10)
    spans = [(4,9),(2,11),(1,12),(1,12),(1,12),(1,12),(1,12)]
    for y, (a, b) in enumerate(spans):
        for x in range(a, b + 1): c.px(x, y + 1, 'bone_d' if y >= 5 else 'bone')
    for x in range(3, 10): c.px(x, 1, 'bone')
    c.px(10, 3, 'red'); c.px(11, 3, 'red'); c.px(10, 4, 'red_d'); c.hline(9, 6, 4, 'outline')
    for (x, y) in [(4,3),(5,4),(3,4)]: c.px(x, y, 'bone_d')
    off = 0 if f == 0 else 1
    for x in (2, 6, 10): c.vline(x + off, 8, 2, 'outline'); c.px(x + off + 1, 9, 'outline')
    c.outline(); return c
def bone():
    c = C(6, 12)
    c.rect(1, 1, 1, 2, 'bone'); c.rect(3, 1, 1, 2, 'bone'); c.rect(2, 2, 1, 8, 'bone'); c.rect(1, 9, 1, 2, 'bone'); c.rect(3, 9, 1, 2, 'bone')
    c.vline(2, 3, 6, 'bone_d'); c.px(2, 2, 'bone'); c.outline(); return c
def shard():
    c = C(6, 12)
    for y in range(1, 11):
        w = 1 + min(y - 1, 10 - y) // 2
        for x in range(3 - w, 3 + w): c.px(x, y, 'rune' if x < 3 else 'rune_d')
    c.px(2, 3, 'white'); c.px(2, 4, 'white'); c.outline(); return c
def king(f):
    """The Bone King, 48x60. f: 0 idle, 1 breathe (shoulders up, eyes white), 2 attack (arms raised, jaw open, eyes red)."""
    c = C(48, 60); cx = 23.5
    lift = 1 if f == 1 else 0
    def mirror(fn):
        fn(1); fn(-1)
    # ---- tattered cape (behind everything) ----
    for y in range(38 - lift, 60):
        hw = min(23, 14 + (y - 38) * 0.55)
        for x in range(int(cx - hw), int(cx + hw) + 1):
            tat = 57 + (H(x, 1, 4) % 3)
            if y > tat and y > 56: continue
            c.px(x, y, 'bg3' if (x + y // 14) % 6 in (0, 1) else 'purple_d')
        c.px(int(cx - hw), y, 'purple'); c.px(int(cx + hw), y, 'purple')
    for x in range(2, 46):
        if 56 < 57 + (H(x, 1, 4) % 3) and x % 3 == 0: c.px(x, 57, 'purple')
    # ---- ribcage + spine ----
    c.rect(23, 39 - lift, 2, 17, 'bone_d')
    for k, y0 in enumerate((40, 44, 48, 52)):
        y0 -= lift; ext = 9 - k * 1
        for sd in (1, -1):
            c.line(int(cx + 0.5) + (0 if sd > 0 else -1), y0, int(cx + 0.5) + sd * ext + (0 if sd > 0 else -1), y0 + 1, 'bone')
            c.line(int(cx + 0.5) + sd * ext + (0 if sd > 0 else -1), y0 + 1, int(cx + 0.5) + sd * (ext - 1) + (0 if sd > 0 else -1), y0 + 3, 'bone_d')
    eyec = ['rune', 'white', 'red'][f]
    for (x, y) in [(23, 43), (24, 43), (22, 44), (25, 44), (23, 45), (24, 45), (23, 44), (24, 44)]: c.px(x, y - lift, eyec if (x, y) in [(23, 44), (24, 44)] else 'rune_d')
    c.px(23, 44 - lift, 'white'); c.px(24, 44 - lift, eyec)
    # ---- arms ----
    def arm(sd):
        sx = int(cx + 0.5) + (0 if sd > 0 else -1)
        if f == 2:
            pts = [(sx + sd * 14, 40), (sx + sd * 19, 33), (sx + sd * 20, 24)]
        else:
            pts = [(sx + sd * 14, 41 - lift), (sx + sd * 17, 50), (sx + sd * 15, 57)]
        for (p0, p1) in zip(pts, pts[1:]):
            for off in (0, 1): c.line(p0[0] + off, p0[1], p1[0] + off, p1[1], 'bone' if off == 0 else 'bone_d')
        hx, hy = pts[-1]
        for k in (-2, 0, 2): c.line(hx + k, hy, hx + k + sd * (1 if f == 2 else 0), hy + (-3 if f == 2 else 3), 'bone')
        c.rect(hx - 1, hy - 1, 3, 3, 'bone')
    mirror(arm)
    # ---- pauldrons with bone spikes ----
    def pauld(sd):
        px_ = int(cx + 0.5) + sd * 15 - (1 if sd < 0 else 0); py = 39 - lift
        c.ellipse(px_, py, 7.5, 5, 'stone'); c.ellipse(px_, py - 1, 6.5, 3.8, 'stone_l')
        c.hline(int(px_ - 6), py + 3, 13, 'gold'); c.hline(int(px_ - 5), py + 4, 11, 'gold_d')
        for dx, h in ((-4, 6), (0, 8), (4, 6)):
            for k in range(h):
                w = 1 if k < h - 2 else 0
                c.rect(px_ + dx - w, py - 4 - k, 2 * w + 1, 1, 'bone' if sd > 0 else 'bone')
            c.px(px_ + dx, py - 4 - h, 'bone_d')
    mirror(pauld)
    # ---- neck ----
    c.rect(21, 34, 6, 5, 'bone_d'); c.hline(21, 36, 6, 'outline')
    # ---- skull ----
    c.ellipse(cx, 22, 11.5, 10.5, 'bone'); c.ellipse(cx + 3, 24, 8, 8.5, 'bone_d'); c.ellipse(cx - 1, 21, 10, 9.5, 'bone')
    for (x, y) in [(15, 14), (16, 15), (17, 16), (33, 13), (32, 14)]: c.px(x, y, 'bone_d')          # cracks
    for (x, y) in [(31, 15), (32, 16)]: c.px(x, y, 'outline')
    c.px(18, 13, 'bone_d'); c.px(19, 12, 'bone_d')
    c.ellipse(13, 26, 2.5, 3, 'bone_d'); c.ellipse(34, 26, 2.5, 3, 'bone_d')                         # cheekbones
    for sd, ex in ((-1, 18), (1, 29)):
        c.ellipse(ex, 22, 4.2, 4.8, 'outline')
        c.rect(ex - 1, 21, 3, 3, eyec); c.px(ex, 22, 'white')
        c.px(ex, 17, 'fire' if f != 1 else 'fire_l'); c.px(ex + sd, 16, 'fire_l'); c.px(ex - sd, 18, 'fire')   # eye flames
    for (x, y) in [(23, 26), (24, 26), (22, 27), (25, 27), (23, 28), (24, 28)]: c.px(x, y, 'outline')       # nose
    # ---- jaw + teeth (opens in attack) ----
    j = [0, 1, 5][f]
    c.rect(15, 30, 18, 2, 'bone'); c.hline(15, 29, 18, 'outline')
    for x in range(16, 33, 2): c.vline(x, 30, 2, 'outline')
    if f == 2: c.rect(16, 32, 16, j - 1, 'red_d'); c.rect(21, 32, 6, j - 2, 'red')
    c.rect(15, 32 + j, 18, 3, 'bone'); c.rect(16, 35 + j, 16, 1, 'bone_d')
    for x in range(17, 32, 2): c.vline(x, 32 + j, 2, 'outline')
    c.vline(14, 30, 6 + j, 'bone_d'); c.vline(33, 30, 6 + j, 'bone_d')
    # ---- crown ----
    c.rect(12, 10, 24, 4, 'gold'); c.hline(12, 10, 24, 'fire_l'); c.hline(12, 13, 24, 'gold_d')
    for (x, h) in ((13, 5), (18, 7), (23, 10), (24, 10), (29, 7), (34, 5)):
        for k in range(h): c.px(x, 9 - k, 'gold' if k % 3 else 'fire_l')
        for k in range(h // 2): c.px(x - 1, 9 - k, 'gold_d'); c.px(x + 1, 9 - k, 'gold_d')
    for x in (17, 23, 29): c.px(x, 12, 'red'); c.px(x + 1, 12, 'red_d')
    c.px(23, 12, 'rune'); c.px(24, 12, 'rune_d')
    if f == 2:
        for (x, y) in [(5, 18), (42, 20), (3, 40), (45, 42), (24, 2), (10, 8), (38, 6)]: c.px(x, y, 'red')
    c.outline(); return c
def brood(f):
    c = C(40, 40)
    legy = [0, 2, -2][f]
    for side in (0, 1):
        sx = 1 if side else -1; cx = 20
        for k, (ex, ey, tx, ty) in enumerate([(14, -10, 20, 4), (17, -4, 27, 14), (18, 3, 29, 24), (15, 9, 25, 33)]):
            x1 = cx + sx * (6 + k); y1 = 17 + k * 2
            kx, ky = cx + sx * ex, 17 + ey + (legy if k % 2 else -legy)
            tx2, ty2 = cx + sx * tx // 1 - (sx * 10 if k == 0 else 0), ty + legy
            c.line(x1, y1, kx, ky, 'brown_d'); c.line(kx, ky, min(39, max(0, tx2)), min(39, max(0, ty2)), 'brown_d')
            c.px(kx, ky, 'brown')
    c.ellipse(20, 27, 11, 10, 'brown_d'); c.ellipse(19, 25, 9, 8, 'brown')
    c.ellipse(20, 31, 3, 5, 'red_d'); c.px(20, 28, 'red'); c.px(20, 31, 'red'); c.px(20, 34, 'red')
    c.ellipse(20, 13, 7, 6, 'brown_d'); c.ellipse(19, 12, 6, 5, 'brown')
    eye = ['fire', 'fire_l', 'red'][f]
    for (x, y) in [(16, 10), (19, 9), (22, 9), (25, 10), (17, 13), (23, 13)]: c.px(x, y, eye)
    fang = 4 if f == 2 else 2
    c.rect(16, 17, 2, fang, 'bone'); c.rect(22, 17, 2, fang, 'bone')
    c.px(20, 3, 'gold'); c.px(18, 4, 'gold'); c.px(22, 4, 'gold')
    c.outline(); return c
def heart():
    rows = [".ooo.ooo.", "oRRRoRRRo", "oRhRRRRRo", "oRRRRRRRo", ".oRRRRRo.", "..oRRRo..", "...oRo...", "....o...."]
    m = {'.':0,'o':'outline','R':'red','h':'white'}; c = C(9, 8)
    for y, r in enumerate(rows):
        for x, ch in enumerate(r): c.px(x, y, m[ch])
    return c
def bolt():
    rows = ["...oo.", "..oRo.", ".oRRo.", "oRRRRo", ".ooRRo", "..oRo.", "..oRo.", "..oo.."]
    m = {'.':0,'o':'outline','R':'rune'}; c = C(6, 8)
    for y, r in enumerate(rows):
        for x, ch in enumerate(r): c.px(x, y, m[ch])
    return c
def pulse_icon(on):
    c = C(18, 18); c.rect(0, 0, 18, 18, 'outline'); c.rect(1, 1, 16, 16, 'purple_d' if not on else 'cloak_d'); c.rect(2, 2, 14, 14, 'bg2')
    col = 'rune' if on else 'purple'
    for (x, y) in [(9,3),(8,4),(10,4),(7,5),(11,5),(6,6),(12,6),(7,7),(11,7),(8,8),(10,8),(9,9)]: c.px(x, y, col)
    c.vline(9, 10, 4, col); c.hline(6, 14, 7, col)
    return c
FONT = {'A': ["01110","10001","10001","11111","10001","10001","10001"], 'B': ["11110","10001","10001","11110","10001","10001","11110"],
        'Y': ["10001","10001","01010","00100","00100","00100","00100"], 'S': ["01111","10000","10000","01110","00001","00001","11110"]}
def logo():
    S = 5; word = "ABYSS"; gap = 4; w = len(word) * (5 * S + gap) + 8; h = 7 * S + 8; c = C(w, h)
    grad = ['white', 'rune', 'rune', 'rune_d', 'cloak_l', 'cloak_l', 'cloak_l']      # one colour per glyph row (top -> bottom)
    cells = []
    for gi, ch in enumerate(word):
        gx = 3 + gi * (5 * S + gap)
        for ry, row in enumerate(FONT[ch]):
            for rx, v in enumerate(row):
                if v == '1': cells.append((gx + rx * S, 2 + ry * S, ry, rx, ch))
    lit = {(x, y) for (x, y, _, _, _) in cells}
    for (x, y, ry, rx, ch) in cells: c.rect(x + 2, y + 2, S, S, 'outline')            # drop shadow
    for (x, y, ry, rx, ch) in cells:
        c.rect(x, y, S, S, grad[ry])
        if (x, y - S) not in lit: c.hline(x, y, S, 'white'); c.hline(x, y + 1, S, 'rune' if ry < 3 else 'cloak_l')   # bright top lip
        if (x - S, y) not in lit: c.vline(x, y, S, 'white')                                                       # bright left lip
        if (x, y + S) not in lit: c.hline(x, y + S - 1, S, 'cloak')                                               # dark bottom lip
        if (x + S, y) not in lit: c.vline(x + S - 1, y, S, 'cloak')                                               # dark right lip
    c.outline('cloak_d'); return c

# ================= assemble =================
SPR = []   # (name, canvas)
def add(name, cv): SPR.append((name, cv))
add('SOLID_A', brick(0)); add('SOLID_B', brick(1)); add('SOLID_TOP_A', brick(0, top=True)); add('SOLID_TOP_B', brick(1, top=True))
add('SOLID_DEEP', brick(0, deep=True)); add('SOLID_UNDER', brick(1, under=True)); add('ONEWAY', oneway()); add('SPIKE', spike()); add('LADDER', ladder())
for f in range(4): add(f'COIN{f}', coin(f))
add('CHEST', chest()); add('EXIT0', door(0)); add('EXIT1', door(1)); add('CRUMBLE', crumble())
add('RUNE_OFF', rune(False)); add('RUNE_ON', rune(True)); add('GATE', gate()); add('TORCH0', torch(0)); add('TORCH1', torch(1)); add('SKULL', skull())
add('GLOW', glow()); add('BG_ARCH', bg_arch()); add('BG_PILLAR', bg_pillar()); add('BG_CHAIN', bg_chain())
for d in (0, 1):
    for n in PFRAMES: add(f'PLAYER_{"RL"[d]}_{n.upper()}', player(n).flip() if d else player(n))
for d in (0, 1):
    for f in (0, 1): add(f'CRAWLER_{"RL"[d]}{f}', crawler(f).flip() if d else crawler(f))
add('BONE', bone()); add('SHARD', shard())
for f in range(3): add(f'KING{f}', king(f))
for f in range(3): add(f'BROOD{f}', brood(f))
add('HEART', heart()); add('BOLT', bolt()); add('PULSE_OFF', pulse_icon(False)); add('PULSE_ON', pulse_icon(True)); add('LOGO', logo())
names = [n for n, _ in SPR]
items = [cv.data() for _, cv in SPR]
theme_blob = []
for t in THEMES:
    for (r, g, b) in t: theme_blob += [r, g, b]
pal_blob = [v for (_, r, g, b) in PAL for v in (r, g, b)]
items.append(theme_blob); items.append(pal_blob)          # item SPR_COUNT = themes (4*33), SPR_COUNT+1 = base palette (32*3)

def crc16(d):
    c = 0xFFFF
    for b in d:
        c ^= b << 8
        for _ in range(8): c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c
def build_pack(kind, its):
    n = len(its); off = 14 + 4 * n; offs = []; body = b''
    for it in its: offs.append(off + len(body)); body += bytes(it)
    payload = b''.join(struct.pack('<I', o) for o in offs) + body
    return b'ABYP' + bytes([kind, 2]) + struct.pack('<HHI', n, crc16(payload), 14 + len(payload)) + payload
def write_c(path, arrays):
    with open(path, 'w') as f:
        f.write('/* GENERATED - host tests only (the calculator reads these packs from archived AppVars) */\n#include <stdint.h>\n')
        for name, blob in arrays:
            f.write(f'const unsigned {name}_LEN = {len(blob)};\nconst uint8_t {name}[{len(blob)}] = {{' + ','.join(map(str, blob)) + '};\n')
pack = build_pack(0, items)
open('data/ABYSSCOR.bin', 'wb').write(pack)
write_c('host/pack_core.c', [('PACK_CORE', pack)])
with open('include/sprite_ids.h', 'w') as f:
    f.write('/* GENERATED by tools/mkassets.py */\n#ifndef SPRITE_IDS_H\n#define SPRITE_IDS_H\nenum {\n' + ''.join(f'  SPR_{n},\n' for n in names) + '  SPR_COUNT,\n  ITEM_THEMES = SPR_COUNT,\n  ITEM_PALETTE\n};\n')
    f.write(f'#define PL_FRAMES {len(PFRAMES)}\n#define SPR_PLAYER(dir, frame) ((dir) ? SPR_PLAYER_L_IDLE0 + (frame) : SPR_PLAYER_R_IDLE0 + (frame))\n')
    f.write('#define SPR_CRAWLER(dir, frame) ((dir) ? SPR_CRAWLER_L0 + (frame) : SPR_CRAWLER_R0 + (frame))\n')
    f.write('enum { FR_IDLE0 = 0, FR_IDLE1, FR_RUN0, FR_RUN1, FR_RUN2, FR_RUN3, FR_JUMP, FR_FALL, FR_CLIMB0, FR_CLIMB1, FR_LAND, FR_HURT, FR_COUNT };\n#endif\n')
print(f'core pack: {len(pack)} bytes, {len(names)} sprites (AppVar limit 65,535)')
assert len(pack) < 65000
# preview helper data for art sheet
import pickle; pickle.dump({'PAL': PAL, 'THEMES': THEMES, 'SPR': [(n, c.p, c.w, c.h) for n, c in SPR]}, open('/tmp/spr_dump.pkl', 'wb'))
