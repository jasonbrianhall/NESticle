//Compatibility header for building NESticle with GCC (DJGPP).
//Force-included into every source file (-include port/port.h).
//The original was built with Watcom C/C++ 10.x (DOS) and MSVC (Win95).
#ifndef _NESTICLE_PORT_H_
#define _NESTICLE_PORT_H_

//calling convention keywords: GCC on i386 uses cdecl by default
#ifndef __WATCOMC__
#define _fastcall
#define __fastcall
#ifndef __cdecl
#define __cdecl
#endif
#define _cdecl
#endif

#ifdef __cplusplus
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#ifdef __DJGPP__
#include <io.h>     //filelength
#endif

//Watcom heap check (stddlg.cpp "Heap check" menu)
#define _HEAPOK      0
#define _HEAPEMPTY   1
#define _HEAPBADNODE 2
inline int _heapset(unsigned int) {return _HEAPOK;}
#endif

#endif
