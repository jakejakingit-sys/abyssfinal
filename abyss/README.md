# ABYSS — TI-84 CE side-scrolling dungeon game (work in progress)

**Playable now: World 1 (Forgotten Crypt) and World 2 (Sunken Mines) — 10 levels, 2 guardians.**
Worlds 3–4 (10 levels, 2 guardians), 6 of 7 abilities, sound and the ending are not built yet.

## Files you must send to the calculator (all of them)
| File | What it is |
|---|---|
| `ABYSS.8xp` | the program |
| `ABYSSCOR.8xv` | sprites + palettes (AppVar, archived by the game on first run) |
| `ABYSSLV1.8xv`, `ABYSSLV2.8xv` | levels for world 1 and 2 (AppVars) |

CI builds all of them (`make` + `make appvars`) and uploads them as one artifact. If a data file is missing the game says so instead of crashing.

## Memory model (archive vs RAM)
* **Archive / flash (read in place, never copied):** every sprite, palette and level lives in AppVars. The game gets a read-only pointer (`ti_GetDataPtr`) and draws sprites straight from flash. If an AppVar was sent to RAM it is archived once, automatically.
* **RAM:** only the current level (≤ 1,440 bytes tile copy + 360 bytes of flags), the player/entity state (~2 KB total), the save struct (100 bytes), and a 56-entry particle pool. No framebuffer in RAM (graphx double-buffers in VRAM). No allocation in the game loop.
* **Archive writes:** the save file (107 bytes, versioned + CRC-16) is written only on level end, quit and purchase. Because an archive write can move data, every pointer is re-acquired right after (`assets_refresh`).
* **Integrity:** each pack has magic/version/length/CRC-16, checked at startup (a damaged pack is reported, not drawn).

## Controls
← → move · **2ND** jump (hold = higher) · ↑↓ climb ladders, ↓ drops through ledges · **ALPHA** Rune Pulse (lights runes within 4 tiles, 20 energy) · ENTER confirm · CLEAR back/pause.

## Content
**World 1:** The Entrance · Forgotten Halls · Burial Grounds (crumbling floors, rune-opened tomb) · Bone Chambers (moving platforms, 2 runes) · **Bone King's Throne** (guardian).
**World 2:** Mine Entrance · Crystal Caverns · Collapsing Tunnels (long crumbling bridges) · Deep Shafts (ladders, 2 runes) · **Broodmother's Nest** (guardian, crystal shards).
Guardians are non-violent: dodge the telegraphed falling bones/shards, climb to three runes, light them with Rune Pulse, the guardian collapses and the gate opens.
Menus: Waystation (Play / Upgrades / Collection / Stats), 20-slot level select with per-world palettes, level-complete screen, pause.
Visuals: 68 sprites, 4 world colour themes applied at runtime, parallax arches/pillars/chains, torch glow, particles, screen shake, palette fades, animated boss.

## Verification status — read this
* **Verified (real gcc, `make -f Makefile.host`): 448 checks, 0 failures** — physics, collisions, platforms, crumbling, runes/gates, creatures, guardian logic, upgrades, saves (every bit-flip detected), pack format + CRC, RAM/archive separation, all 10 levels (structure, reachability heuristic) and simulated-input runs that cross every moving-platform gap and the spike field.
* **Written, NOT compiled/run by the author:** `src/main.c`, `src/platform_ce.c`, the `Makefile` AppVar rule (`convbin` flags from memory) and the CI job. Syntax-checked against stub headers only. Expect compile errors on the first real build, and treat frame rate / feel on hardware as unknown.
* Reachability is a heuristic + simulated routes, not a full solver.

## Not implemented
Worlds 3–4 · ability gameplay for Heavy Landing, Phase Dash, Parry, Ground Wave, Whirlwind, Resonance (they unlock but do nothing) · sound · ending · relics · checkpoints · crouch/dash/death/victory animations · world-specific backgrounds beyond palette swaps.

## Build
`make -f Makefile.host` — tests (regenerates art/levels). `make` — program. `make appvars` — AppVars (`convbin`). Levels: `levels/world*/*.txt` (legend in `tools/author_world1.py`); regenerate packs with `python3 tools/mklevels.py`.
