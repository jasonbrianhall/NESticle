# Source/port: the missing pieces

The leaked NESticle source isn't complete. Some files were never part of it,
and some were x86 assembly for Watcom's/Borland's TASM that no current
toolchain assembles. This folder fills those gaps with portable C++, behind
the same interfaces, so the original sources build with GCC (DJGPP) almost
unchanged.

| File | Replaces | Notes |
|---|---|---|
| `m6502.cpp` | `M6502\M6502.ASM` (Neil Bradley's 6502 core, modified for NESticle) | Same API (`m6502.h`): `m6502exec`, `m6502nmi`, `m6502int`, `m6502reset`, memory trap pointers, `cyclesRemaining`. Instruction semantics and cycle table from [WarpNES](https://github.com/jasonbrianhall/warpnes) (`Emulation/Instructions.cpp`). |
| `draw.cpp` | `TILE.ASM`, `SPRITE.ASM`, `SPRITEBG.ASM`, `R2IMGASM.ASM` | NES tile/sprite blitters, RLE image drawing, lines, rectangles. |
| `guirect.cpp`, `gui.cpp`, `guimenu.cpp` | `GUIRECT.CPP`, `GUI.CPP`, `GUIMENU.CPP` (never released) | A new implementation of the GUI library declared in `guirect.h`, `gui.h`, `guimenu.h`, `guiroot.h`: windows, buttons, edits, scroll bars, list boxes, menus. |
| `guidata.cpp`, `font5x8.h` | `GUI.VOL` (never released) | Palette, font, mouse cursor and icons, built at startup when there's no `GUI.VOL`. The font is the public domain X11 misc-fixed 5x8. |
| `nespal.cpp` | `NESticle.pal` (never released) | Default NES palette (WarpNES's), used when there's no `NESticle.pal`. |
| `nesapu.cpp` | the sound in `nessound.cpp` | NESticle's own sound was still a sketch (two rough squares and a triangle). `nessound` now feeds every register write to WarpNES's APU channels (2 pulse, triangle, noise) and mixes from them. |
| `port.h` | – | Force-included compatibility header (`_fastcall`, `__cdecl`, Watcom heap check). |

The platform layer (video, keyboard, mouse, timer, sound output) is in
`../../msdos/sdlmain.cpp`, on SDL3's DOS port; see `../../msdos/README.md`.

## Changes to the original sources

Kept small, and marked where it isn't obvious:

- `m6502.h` is the M6502 API header the code was written against (the old
  top-level `m6502.h`, for Icer's later n6502 core used only by `ntest.cpp`,
  is now `n6502.h`).
- `guimenu.h`: `menuitem m[]` → `m[32]` (Watcom accepted initialized flexible
  arrays; GCC doesn't).
- `r2img.h`: `PALETTE` back to a plain color array (it is read raw from .VOL
  files); `drawrect` declared on every platform.
- `file.cpp`: DJGPP defines `__unix__`, which made files open twice, in text
  mode; files are created in binary mode; `enumdir` for DJGPP (`findfirst`).
- `main.cpp`: falls back to the built-in GUI data; DOS default resolution
  320x240 (was 256x224, too narrow for the dialogs).
- `nesvideo.cpp`: falls back to the built-in NES palette.
- `nessound.cpp`, `nes.cpp`: sound through `nesapu`; $4017 reaches the APU.
- `message.cpp`, `command.cpp`: `-log` option (messages to stdout too).
- `build.cpp`: compiler name for GCC/DJGPP.
- A few `for (i=...)` loops that relied on the old for-scope rule.
