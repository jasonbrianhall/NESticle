NESticle for Windows
====================

The same NESticle as the DOS build (../msdos), on SDL3 in a window,
64-bit, built with MinGW-w64.

Building
--------

From Linux (cross-compiling):

    sudo dnf install mingw64-gcc-c++ make           (Fedora)
    sudo apt install g++-mingw-w64-x86-64 make      (Debian/Ubuntu)
    make            nesticle.exe + SDL3.dll
    make dist       build/dist/NESticle: nesticle.exe, SDL3.dll, GUI.VOL, README

SDL3's MinGW development package is downloaded from libsdl-org/SDL the
first time. On Windows with MSYS2: pacman -S mingw-w64-x86_64-gcc make
curl, then make CROSS= .

Running
-------

Keep SDL3.dll (and GUI.VOL) next to nesticle.exe. Run it, or drop
options on it from a command prompt: nesticle -run SMB.NES. The window
can be resized; Alt+Enter switches to full screen and back. Keys, menus,
GUI.VOL and CURSOR.BMP work as in ../msdos/README.md.
