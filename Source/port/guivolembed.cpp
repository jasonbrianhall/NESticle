//The original GUI.VOL, built into the program, so the original art is there
//wherever the program is run from (a GUI.VOL next to it still comes first).
//
//The build says where GUI.VOL is, if it has one:
//  NESTICLE_GUIVOL_FILE="path"   GCC (DJGPP, Linux, MinGW): .incbin
//  NESTICLE_GUIVOL_INC="file.h"  any compiler (CMake/MSVC): a generated
//                                header with the bytes as an array
//Without either, there's nothing built in, and the built-in stand-in art
//(guidata.cpp) is used when there's no GUI.VOL.

#if defined(NESTICLE_GUIVOL_INC)

static const unsigned char guivol[]=
{
#include NESTICLE_GUIVOL_INC
};
extern "C" const unsigned char *nesticle_guivol_data() {return guivol;}
extern "C" long nesticle_guivol_size() {return (long)sizeof(guivol);}

#elif defined(NESTICLE_GUIVOL_FILE) && defined(__GNUC__)

//(symbol names get the platform's C prefix: "_" on DJGPP)
#define NGV_STR2(x) #x
#define NGV_STR(x) NGV_STR2(x)
#define NGV_SYM(n) NGV_STR(__USER_LABEL_PREFIX__) #n

#if defined(_WIN32)
#define NGV_SECTION ".section .rdata,\"dr\"\n"
#elif defined(__DJGPP__)
#define NGV_SECTION ".section .data\n"
#else
#define NGV_SECTION ".section .rodata\n"
#endif

__asm__(
 NGV_SECTION
 ".balign 4\n"
 ".globl " NGV_SYM(nesticle_guivol_begin) "\n"
 NGV_SYM(nesticle_guivol_begin) ":\n"
 ".incbin \"" NESTICLE_GUIVOL_FILE "\"\n"
 ".globl " NGV_SYM(nesticle_guivol_end) "\n"
 NGV_SYM(nesticle_guivol_end) ":\n"
 ".byte 0\n"
 ".text\n"
);

extern "C" const unsigned char nesticle_guivol_begin[],nesticle_guivol_end[];
extern "C" const unsigned char *nesticle_guivol_data() {return nesticle_guivol_begin;}
extern "C" long nesticle_guivol_size() {return (long)(nesticle_guivol_end-nesticle_guivol_begin);}

#else

extern "C" const unsigned char *nesticle_guivol_data() {return 0;}
extern "C" long nesticle_guivol_size() {return 0;}

#endif
