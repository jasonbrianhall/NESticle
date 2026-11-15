NESticle for MS-DOS
===================

The leaked NESticle source (../Source), built as a 32-bit protected-mode
DOS program with DJGPP. The original DOS platform code (VESA/ModeX video,
keyboard IRQ, PIT timer, Sound Blaster mixer, gameport) was Watcom-only C and
TASM; here SDL3's DOS port does that job, the same way SDLPoPDOS does
(https://github.com/jasonbrianhall/SDLPoPDOS, msdos/). The parts of NESticle
that were never released (GUI library, GUI.VOL, the 6502 and drawing
assembly) are rewritten in ../Source/port (see the README there).


Building
--------

    make            nesticle.exe, with CWSDPMI built in
    make msdos_original
                    nesticle.exe that needs CWSDPMI.EXE (or another DPMI host)
    make dist       build/dist/NESTICLE, a folder to copy to a DOS machine
    make run        run it in DOSBox
    make clean      (distclean also removes the compiled SDL3)

Needs a DJGPP cross compiler on the PATH (i586-pc-msdosdjgpp-gcc and -g++;
prebuilt: https://github.com/andrewwutw/build-djgpp/releases), or Docker,
which uses the djfdyuruiry/djgpp image. SDL3 (with its DOS port) and CWSDPMI
are in this folder, as in SDLPoPDOS; SDL3 is compiled into build/libSDL3.a
the first time.


Running it
----------

    C:\NES> NESTICLE.EXE                    the GUI
    C:\NES> NESTICLE.EXE -run SMB           load SMB.NES and run it
    C:\NES> NESTICLE.EXE -res 640 480       a bigger desktop
    C:\NES> NESTICLE.EXE -?                 all the options

Requirements: a 486DX or better (a Pentium for full speed; in DOSBox,
cycles=max), 8 MB of RAM, VGA. A VESA BIOS gives the 320x240 and bigger
8-bit modes; without one it uses VGA mode 13h. Sound: a Sound Blaster with
the BLASTER variable set (e.g. SET BLASTER=A220 I7 D1 H5 T6); -nosound to
turn it off. A mouse driver for the GUI. Optional gameport joystick.

ROMs go next to NESTICLE.EXE (8.3 names, .NES). Battery saves, snapshots
(.PCX), save states and NESticle.cfg are written there too.

The video mode: by default the 8-bit mode that fits the desktop best (VESA
320x240 for the default 320x240 desktop); to choose, set NESTICLE_VIDEO
to WIDTHxHEIGHTxBITS before starting, e.g. SET NESTICLE_VIDEO=640x480x8.
A smaller desktop is shown scaled by a whole number and centered.


Keys
----

    Esc             show/hide the GUI
    Space           maximize the NES screen and hide the GUI / back
    Arrows          joypad 1 (Home/PgUp/End/PgDn diagonals)
    O P [ L         B, A, Select, Start (Keyboard 1; Settings > Redefine
                    input to change them, Save config to keep them)
    Enter, Tab      Start, Select
    Alt+E           reset (CPU menu: Alt+P pause, Alt+R resume, ...)
    Alt+L           load a ROM
    Alt+Q           quit
    F1              messages        F2 pattern tables  F3 name tables
    F4              palette         F5 save state      F7 restore state
    F8              sound on/off    F9 PCX snapshot

Menu shortcuts that are letters need Alt held (in the original they were
plain keys; with the GUI hidden a stray key could quit).


What's what
-----------

    sdlmain.cpp     the platform layer: SDL3 window/surface (8-bit, or
                    hicolor through a palette table), XT scan codes from
                    SDL keys, mouse, gameport, Sound Blaster through an
                    SDL audio stream, the main loop and timer
    SDL3/           SDL3 source with its DOS port (from SDLPoPDOS)
    sdl3-config/    the generated SDL headers for DJGPP (from SDLPoPDOS)
    cwsdpmi/        CWSDPMI r7 (from SDLPoPDOS)
