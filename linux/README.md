NESticle for Linux
==================

The same NESticle as the DOS build (../msdos), on SDL3 in a window.

Building
--------

    sudo dnf install gcc-c++ make SDL3-devel        (Fedora)
    sudo apt install g++ make libsdl3-dev           (Debian/Ubuntu with SDL3)
    make            ./nesticle
    make dist       build/dist/nesticle: nesticle, GUI.VOL, this README

Running
-------

    ./nesticle                  the GUI
    ./nesticle -run SMB.NES     load and run a ROM

The window can be resized (the picture scales by whole numbers);
F11 (or Alt+Enter) switches to full screen and back. Keys, menus, GUI.VOL and
CURSOR.BMP work as in ../msdos/README.md; GUI.VOL is looked for in the
current folder, then next to the program. File names are case sensitive
here: give ROMs with their real names.
