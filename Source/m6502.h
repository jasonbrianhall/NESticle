/* 6502 CPU core interface (Neil Bradley's M6502 API, as used by NESticle).
   The original core was M6502\M6502.ASM (TASM, flat model). In this port it
   is port/m6502.cpp, a portable C++ implementation of the same interface:

   - reads/writes below 0x2000 go straight to m6502ram (mirrored every 2K)
   - reads at 0x8000 and up come straight from m6502rom (m6502rom+addr)
   - everything else goes through m6502MemoryRead / m6502MemoryWrite
     (writes at 0x8000 and up too, for the memory mappers)
   - m6502exec(cycles) returns 0x10000 when the cycles ran out, or the
     address of an unknown opcode it stopped at
*/
#ifndef _M6502_H_
#define _M6502_H_

#ifndef DWORD
#define DWORD unsigned long int
#endif
#ifndef WORD
#define WORD  unsigned short int
#endif
#ifndef BYTE
#define BYTE  unsigned char
#endif

extern "C" {

extern DWORD m6502clockticks;
extern BYTE *m6502rom; //used for accesssing >=0x8000
extern BYTE *m6502ram; //used for accessing <0x2000
extern WORD m6502lastjsr;

extern WORD m6502pc;
extern WORD m6502af;  //A in the low byte, flags in the high byte
extern BYTE m6502x;
extern BYTE m6502y;
extern BYTE m6502s;
extern DWORD m6502nmi(void);
extern DWORD m6502int(void);
extern DWORD m6502exec(DWORD);
extern void  m6502reset(void);
extern DWORD m6502GetInfo(void);
extern int cyclesRemaining;
extern BYTE syncCycle;
extern BYTE inNmi;

extern int  (*m6502MemoryWrite)(WORD,BYTE);
extern WORD (*m6502MemoryRead)(WORD);

}

#endif
