# Memory report

**Host x86-64 numbers (sizeof, file sizes). NOT a link-map measurement of the calculator build** — eZ80 `int` is 24-bit and code size is unmeasured.

```
memory (HOST sizeof - not a measurement of the calculator build)
  sizeof Game=2120 Player=24 LevelState=1880 SaveData=100  save file=107 bytes
  level data: 9900 bytes for 10 levels (archived AppVars, read in place); only ONE level (<= 1440 bytes) is copied to RAM

```

| Item | Bytes | Where |
|---|---|---|
| Sprites + palettes (ABYSSCOR AppVar) | 45509 | archive, read in place (AppVar limit 65,535) |
| World 1 levels (ABYSSLV1) | 5158 | archive, read in place |
| World 2 levels (ABYSSLV2) | 5107 | archive, read in place |
| Current level copy + flags (`LevelState`) | see sizeof above | RAM (one level at a time) |
| `Game` (player, entities, level state) | see sizeof above | RAM |
| `SaveData` / save file | 100 / 107 | RAM / archive (written on events only) |
| Particle pool | 56 x 12 = 672 | RAM |
| Framebuffer | 0 | graphx double-buffers in VRAM |

Not yet measured: program code size, total runtime RAM, FPS.
