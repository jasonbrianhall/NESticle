//Compatibility header for building NESticle with GCC (DJGPP, Linux,
//MinGW) and Microsoft C.
//Force-included into every source file (-include port/port.h).
//The original was built with Watcom C/C++ 10.x (DOS) and MSVC (Win95).
#ifndef _NESTICLE_PORT_H_
#define _NESTICLE_PORT_H_

#ifdef _MSC_VER
//(the POSIX names: open, read, stricmp... without the warnings)
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

//calling convention keywords: GCC on i386 uses cdecl by default
#if !defined(__WATCOMC__) && !defined(_MSC_VER)
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
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <direct.h> //getcwd, chdir
#include <stdlib.h> //_MAX_PATH
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&_S_IFMT)==_S_IFDIR)
#endif
#endif
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
//directory browsing in Load ROM (nesdlg.cpp)
#if defined(__GNUC__) || defined(_MSC_VER)
#define NESTICLE_DIRS
#endif

#ifdef _WIN32
#include <direct.h>
#define realpath(n,r) _fullpath((r),(n),PATH_MAX)
#define mkdir(d,m) _mkdir(d)
#define fsync(h) _commit(h)
#endif

//Watcom heap check (stddlg.cpp "Heap check" menu)
#ifndef _HEAPOK
#define _HEAPOK      0
#define _HEAPEMPTY   1
#define _HEAPBADNODE 2
#endif
inline int _heapset(unsigned int) {return _HEAPOK;}
#endif

#endif
