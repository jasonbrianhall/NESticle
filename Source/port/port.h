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
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#if defined(__DJGPP__) || defined(_WIN32)
#include <io.h>     //filelength
#else
//Linux and the like: no text mode, no filelength
#ifndef O_BINARY
#define O_BINARY 0
#endif
inline long filelength(int h) {struct stat s; return fstat(h,&s) ? -1 : s.st_size;}
#endif
#ifdef _WIN32
#include <direct.h>
#define realpath(n,r) _fullpath((r),(n),PATH_MAX)
#define mkdir(d,m) _mkdir(d)
#define fsync(h) _commit(h)
#endif

//Watcom heap check (stddlg.cpp "Heap check" menu)
#define _HEAPOK      0
#define _HEAPEMPTY   1
#define _HEAPBADNODE 2
inline int _heapset(unsigned int) {return _HEAPOK;}
#endif

#endif
