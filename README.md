# NESticle
The Source Code of the NESticle emulator. Created by Bloodlust Software. It was leaked in 1997 by MindRape.

# History
The NESticle emulator was made by Icer Addis which its source code was later leaked by Donald Moore (MindRape). In Early 2019, togemet2 released the source code to the public, and I decided to put the source code here with togemet2's permission.

## Can it be compiled?
Yes: for MS-DOS ([msdos](msdos/README.md)), Linux ([linux](linux/README.md)) and Windows with MinGW ([windows](windows/README.md)); GitHub Actions builds all three on every push (packages under Actions, and on releases for v* tags). The parts that were never in the leak (the GUI library, GUI.VOL, the NES palette file) and the TASM assembly (6502 core, tile/sprite drawing) are rewritten in [Source/port](Source/port/README.md), and the DOS platform layer uses SDL3's DOS port, as SDLPoPDOS does. With a DJGPP cross compiler:

    cd msdos
    make

Original note: I have no clue. We had several attempts at compiling. We're not even sure if this is a completed version!

If anyone's interested in helping us compile this source code to DOS/WIN9x or even Modern Windows, you can fork the code from this GitHub repository and make whatever changes you want with it.

## What are these zip files with cuss words containing more source code about?
I don't know, the best speculation we have that it's full of versions that Addis, well, fucked up.

## Copyright
NESticle is © Bloodlust Software
