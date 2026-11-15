//Portable 6502 core for NESticle.
//
//Replaces M6502\M6502.ASM (Neil Bradley's M6502, TASM flat model, as modified
//for NESticle), which can't be assembled with DJGPP's tools. It implements
//the same interface (see m6502.h) and the same memory model:
//
//  - 0x0000-0x1FFF: m6502ram, mirrored every 2K, accessed directly
//  - 0x8000-0xFFFF: read directly from m6502rom (m6502rom+addr), written
//                   through m6502MemoryWrite (the memory mapper)
//  - 0x2000-0x7FFF: m6502MemoryRead / m6502MemoryWrite
//  - the stack and zero page are always m6502ram
//
//The instruction semantics and cycle table follow WarpNES's 6502
//(github.com/jasonbrianhall/warpnes, Emulation/Instructions.cpp).
//Unofficial NOPs are treated as NOPs; any other unknown opcode stops
//execution and m6502exec returns its address, like the original.

#include "../m6502.h"

extern "C" {
DWORD m6502clockticks=0;
BYTE *m6502rom=0;
BYTE *m6502ram=0;
WORD m6502lastjsr=0;
WORD m6502pc=0;
WORD m6502af=0;
BYTE m6502x=0;
BYTE m6502y=0;
BYTE m6502s=0xFF;
int cyclesRemaining=0;
BYTE syncCycle=0;
BYTE inNmi=0;
int  (*m6502MemoryWrite)(WORD,BYTE)=0;
WORD (*m6502MemoryRead)(WORD)=0;
}

#define F_C 0x01
#define F_Z 0x02
#define F_I 0x04
#define F_D 0x08
#define F_B 0x10
#define F_R 0x20
#define F_V 0x40
#define F_N 0x80

//base cycle counts (from WarpNES); 0 = not implemented
static const BYTE cyc[256]=
{
 7,6,0,0,3,3,5,0,3,2,2,0,4,4,6,0,
 2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
 6,6,0,0,3,3,5,0,4,2,2,0,4,4,6,0,
 2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
 6,6,0,0,3,3,5,0,3,2,2,0,3,4,6,0,
 2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
 6,6,0,0,3,3,5,0,4,2,2,0,5,4,6,0,
 2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
 2,6,2,0,3,3,3,0,2,2,2,0,4,4,4,0,
 2,6,0,0,4,4,4,0,2,5,2,0,0,5,0,0,
 2,6,2,0,3,3,3,0,2,2,2,0,4,4,4,0,
 2,5,0,0,4,4,4,0,2,4,2,0,4,4,4,0,
 2,6,2,0,3,3,5,0,2,2,2,0,4,4,6,0,
 2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0,
 2,6,2,0,3,3,5,0,2,2,2,0,4,4,6,0,
 2,5,0,0,4,4,6,0,2,4,2,0,4,4,7,0
};

//CPU state while executing (copied in/out of the m6502xxx globals)
static BYTE A,X,Y,S,P;
static WORD PC;

static inline BYTE rd(WORD a)
{
 if (a<0x2000) return m6502ram[a&0x7FF];
 if (a>=0x8000) return m6502rom[a];
 return (BYTE)m6502MemoryRead(a);
}

static inline void wr(WORD a,BYTE d)
{
 if (a<0x2000) {m6502ram[a&0x7FF]=d; return;}
 m6502MemoryWrite(a,d);
}

//code fetch: code can run from RAM, SRAM or ROM
static inline BYTE fetch()
{
 WORD a=PC++;
 if (a>=0x8000) return m6502rom[a];
 if (a<0x2000) return m6502ram[a&0x7FF];
 return m6502ram[a]; //SRAM/expansion (0x5000-0x7FFF) lives in ram[] too
}
static inline WORD fetchw() {WORD l=fetch(); return l|(fetch()<<8);}

static inline void push(BYTE d) {m6502ram[0x100+S]=d; S--;}
static inline BYTE pull() {S++; return m6502ram[0x100+S];}

static inline void zn(BYTE v) {P=(P&~(F_Z|F_N))|(v ? 0 : F_Z)|(v&F_N);}

static inline WORD rdzpw(BYTE a) {return m6502ram[a]|(m6502ram[(BYTE)(a+1)]<<8);}

static void loadregs()
{
 A=m6502af&0xFF; P=(m6502af>>8)|F_R;
 X=m6502x; Y=m6502y; S=m6502s; PC=m6502pc;
}

static void saveregs()
{
 m6502af=A|((P|F_R)<<8);
 m6502x=X; m6502y=Y; m6502s=S; m6502pc=PC;
}

//address modes
#define ZP()   ((WORD)fetch())
#define ZPX()  ((WORD)(BYTE)(fetch()+X))
#define ZPY()  ((WORD)(BYTE)(fetch()+Y))
#define ABS()  (fetchw())
#define ABSX() ((WORD)(fetchw()+X))
#define ABSY() ((WORD)(fetchw()+Y))
#define INDX() (rdzpw((BYTE)(fetch()+X)))
#define INDY() ((WORD)(rdzpw(fetch())+Y))

//operations
#define ADC(v) {BYTE m=(v); unsigned r=A+m+(P&F_C); \
  P=(P&~(F_C|F_V))|(r>0xFF ? F_C : 0)|(((A^r)&(m^r)&0x80) ? F_V : 0); A=(BYTE)r; zn(A);}
#define SBC(v) ADC((BYTE)~(v))
#define CMPR(r,v) {BYTE m=(v); unsigned t=(r)-m; P=(P&~F_C)|((r)>=m ? F_C : 0); zn((BYTE)t);}
#define BITT(v) {BYTE m=(v); P=(P&~(F_Z|F_V|F_N))|((A&m) ? 0 : F_Z)|(m&(F_V|F_N));}
#define BRANCH(cond) {signed char o=(signed char)fetch(); if (cond) { \
  WORD n=PC+o; c+=((n^PC)&0xFF00) ? 2 : 1; PC=n;}}

//read-modify-write on memory
#define RMW(addrexpr,op) {WORD ad=(addrexpr); BYTE v=rd(ad); op; wr(ad,v); zn(v);}
#define ASLOP {P=(P&~F_C)|(v>>7); v<<=1;}
#define LSROP {P=(P&~F_C)|(v&1); v>>=1;}
#define ROLOP {BYTE c0=P&F_C; P=(P&~F_C)|(v>>7); v=(v<<1)|c0;}
#define ROROP {BYTE c0=P&F_C; P=(P&~F_C)|(v&1); v=(v>>1)|(c0<<7);}
#define INCOP {v++;}
#define DECOP {v--;}

extern "C" DWORD m6502exec(DWORD cycles)
{
 cyclesRemaining=(int)cycles;
 if (cyclesRemaining<=0) return 0x10000;
 loadregs();

 while (cyclesRemaining>0)
 {
  WORD opaddr=PC;
  BYTE op=fetch();
  int c=cyc[op];

  switch (op)
  {
   //loads
   case 0xA9: A=fetch(); zn(A); break;
   case 0xA5: A=m6502ram[ZP()]; zn(A); break;
   case 0xB5: A=m6502ram[ZPX()]; zn(A); break;
   case 0xAD: A=rd(ABS()); zn(A); break;
   case 0xBD: A=rd(ABSX()); zn(A); break;
   case 0xB9: A=rd(ABSY()); zn(A); break;
   case 0xA1: A=rd(INDX()); zn(A); break;
   case 0xB1: A=rd(INDY()); zn(A); break;

   case 0xA2: X=fetch(); zn(X); break;
   case 0xA6: X=m6502ram[ZP()]; zn(X); break;
   case 0xB6: X=m6502ram[ZPY()]; zn(X); break;
   case 0xAE: X=rd(ABS()); zn(X); break;
   case 0xBE: X=rd(ABSY()); zn(X); break;

   case 0xA0: Y=fetch(); zn(Y); break;
   case 0xA4: Y=m6502ram[ZP()]; zn(Y); break;
   case 0xB4: Y=m6502ram[ZPX()]; zn(Y); break;
   case 0xAC: Y=rd(ABS()); zn(Y); break;
   case 0xBC: Y=rd(ABSX()); zn(Y); break;

   //stores
   case 0x85: m6502ram[ZP()]=A; break;
   case 0x95: m6502ram[ZPX()]=A; break;
   case 0x8D: wr(ABS(),A); break;
   case 0x9D: wr(ABSX(),A); break;
   case 0x99: wr(ABSY(),A); break;
   case 0x81: wr(INDX(),A); break;
   case 0x91: wr(INDY(),A); break;

   case 0x86: m6502ram[ZP()]=X; break;
   case 0x96: m6502ram[ZPY()]=X; break;
   case 0x8E: wr(ABS(),X); break;

   case 0x84: m6502ram[ZP()]=Y; break;
   case 0x94: m6502ram[ZPX()]=Y; break;
   case 0x8C: wr(ABS(),Y); break;

   //transfers
   case 0xAA: X=A; zn(X); break;
   case 0xA8: Y=A; zn(Y); break;
   case 0x8A: A=X; zn(A); break;
   case 0x98: A=Y; zn(A); break;
   case 0xBA: X=S; zn(X); break;
   case 0x9A: S=X; break;

   //stack
   case 0x48: push(A); break;
   case 0x08: push(P|F_B|F_R); break;
   case 0x68: A=pull(); zn(A); break;
   case 0x28: P=pull()|F_R; P&=~F_B; break;

   //logic
   case 0x29: A&=fetch(); zn(A); break;
   case 0x25: A&=m6502ram[ZP()]; zn(A); break;
   case 0x35: A&=m6502ram[ZPX()]; zn(A); break;
   case 0x2D: A&=rd(ABS()); zn(A); break;
   case 0x3D: A&=rd(ABSX()); zn(A); break;
   case 0x39: A&=rd(ABSY()); zn(A); break;
   case 0x21: A&=rd(INDX()); zn(A); break;
   case 0x31: A&=rd(INDY()); zn(A); break;

   case 0x09: A|=fetch(); zn(A); break;
   case 0x05: A|=m6502ram[ZP()]; zn(A); break;
   case 0x15: A|=m6502ram[ZPX()]; zn(A); break;
   case 0x0D: A|=rd(ABS()); zn(A); break;
   case 0x1D: A|=rd(ABSX()); zn(A); break;
   case 0x19: A|=rd(ABSY()); zn(A); break;
   case 0x01: A|=rd(INDX()); zn(A); break;
   case 0x11: A|=rd(INDY()); zn(A); break;

   case 0x49: A^=fetch(); zn(A); break;
   case 0x45: A^=m6502ram[ZP()]; zn(A); break;
   case 0x55: A^=m6502ram[ZPX()]; zn(A); break;
   case 0x4D: A^=rd(ABS()); zn(A); break;
   case 0x5D: A^=rd(ABSX()); zn(A); break;
   case 0x59: A^=rd(ABSY()); zn(A); break;
   case 0x41: A^=rd(INDX()); zn(A); break;
   case 0x51: A^=rd(INDY()); zn(A); break;

   case 0x24: BITT(m6502ram[ZP()]); break;
   case 0x2C: BITT(rd(ABS())); break;

   //arithmetic
   case 0x69: ADC(fetch()); break;
   case 0x65: ADC(m6502ram[ZP()]); break;
   case 0x75: ADC(m6502ram[ZPX()]); break;
   case 0x6D: ADC(rd(ABS())); break;
   case 0x7D: ADC(rd(ABSX())); break;
   case 0x79: ADC(rd(ABSY())); break;
   case 0x61: ADC(rd(INDX())); break;
   case 0x71: ADC(rd(INDY())); break;

   case 0xE9: case 0xEB: SBC(fetch()); break;
   case 0xE5: SBC(m6502ram[ZP()]); break;
   case 0xF5: SBC(m6502ram[ZPX()]); break;
   case 0xED: SBC(rd(ABS())); break;
   case 0xFD: SBC(rd(ABSX())); break;
   case 0xF9: SBC(rd(ABSY())); break;
   case 0xE1: SBC(rd(INDX())); break;
   case 0xF1: SBC(rd(INDY())); break;

   case 0xC9: CMPR(A,fetch()); break;
   case 0xC5: CMPR(A,m6502ram[ZP()]); break;
   case 0xD5: CMPR(A,m6502ram[ZPX()]); break;
   case 0xCD: CMPR(A,rd(ABS())); break;
   case 0xDD: CMPR(A,rd(ABSX())); break;
   case 0xD9: CMPR(A,rd(ABSY())); break;
   case 0xC1: CMPR(A,rd(INDX())); break;
   case 0xD1: CMPR(A,rd(INDY())); break;

   case 0xE0: CMPR(X,fetch()); break;
   case 0xE4: CMPR(X,m6502ram[ZP()]); break;
   case 0xEC: CMPR(X,rd(ABS())); break;
   case 0xC0: CMPR(Y,fetch()); break;
   case 0xC4: CMPR(Y,m6502ram[ZP()]); break;
   case 0xCC: CMPR(Y,rd(ABS())); break;

   //increments/decrements
   case 0xE6: {BYTE a=fetch(); zn(++m6502ram[a]);} break;
   case 0xF6: {BYTE a=fetch()+X; zn(++m6502ram[a]);} break;
   case 0xEE: RMW(ABS(),INCOP); break;
   case 0xFE: RMW(ABSX(),INCOP); break;
   case 0xC6: {BYTE a=fetch(); zn(--m6502ram[a]);} break;
   case 0xD6: {BYTE a=fetch()+X; zn(--m6502ram[a]);} break;
   case 0xCE: RMW(ABS(),DECOP); break;
   case 0xDE: RMW(ABSX(),DECOP); break;
   case 0xE8: X++; zn(X); break;
   case 0xC8: Y++; zn(Y); break;
   case 0xCA: X--; zn(X); break;
   case 0x88: Y--; zn(Y); break;

   //shifts
   case 0x0A: P=(P&~F_C)|(A>>7); A<<=1; zn(A); break;
   case 0x4A: P=(P&~F_C)|(A&1); A>>=1; zn(A); break;
   case 0x2A: {BYTE c0=P&F_C; P=(P&~F_C)|(A>>7); A=(A<<1)|c0; zn(A);} break;
   case 0x6A: {BYTE c0=P&F_C; P=(P&~F_C)|(A&1); A=(A>>1)|(c0<<7); zn(A);} break;
   case 0x06: RMW(ZP(),ASLOP); break;
   case 0x16: RMW(ZPX(),ASLOP); break;
   case 0x0E: RMW(ABS(),ASLOP); break;
   case 0x1E: RMW(ABSX(),ASLOP); break;
   case 0x46: RMW(ZP(),LSROP); break;
   case 0x56: RMW(ZPX(),LSROP); break;
   case 0x4E: RMW(ABS(),LSROP); break;
   case 0x5E: RMW(ABSX(),LSROP); break;
   case 0x26: RMW(ZP(),ROLOP); break;
   case 0x36: RMW(ZPX(),ROLOP); break;
   case 0x2E: RMW(ABS(),ROLOP); break;
   case 0x3E: RMW(ABSX(),ROLOP); break;
   case 0x66: RMW(ZP(),ROROP); break;
   case 0x76: RMW(ZPX(),ROROP); break;
   case 0x6E: RMW(ABS(),ROROP); break;
   case 0x7E: RMW(ABSX(),ROROP); break;

   //jumps
   case 0x4C: PC=fetchw(); break;
   case 0x6C:
    {
     WORD a=fetchw();
     WORD hi=(a&0xFF00)|((a+1)&0xFF); //page wrap bug
     PC=rd(a)|(rd(hi)<<8);
    }
    break;
   case 0x20:
    {
     WORD t=fetchw();
     WORD r=PC-1;
     push(r>>8); push(r&0xFF);
     m6502lastjsr=t;
     PC=t;
    }
    break;
   case 0x60: {WORD l=pull(); PC=(l|(pull()<<8))+1;} break;
   case 0x40: P=pull()|F_R; P&=~F_B; {WORD l=pull(); PC=l|(pull()<<8);} if (inNmi) inNmi--; break;
   case 0x00:
    {
     WORD r=PC+1;
     push(r>>8); push(r&0xFF);
     push(P|F_B|F_R);
     P|=F_I;
     PC=m6502rom[0xFFFE]|(m6502rom[0xFFFF]<<8);
    }
    break;

   //branches
   case 0x10: BRANCH(!(P&F_N)); break;
   case 0x30: BRANCH(P&F_N); break;
   case 0x50: BRANCH(!(P&F_V)); break;
   case 0x70: BRANCH(P&F_V); break;
   case 0x90: BRANCH(!(P&F_C)); break;
   case 0xB0: BRANCH(P&F_C); break;
   case 0xD0: BRANCH(!(P&F_Z)); break;
   case 0xF0: BRANCH(P&F_Z); break;

   //flags
   case 0x18: P&=~F_C; break;
   case 0x38: P|=F_C; break;
   case 0x58: P&=~F_I; break;
   case 0x78: P|=F_I; break;
   case 0xB8: P&=~F_V; break;
   case 0xD8: P&=~F_D; break;
   case 0xF8: P|=F_D; break;

   //NOPs (official and the common unofficial ones)
   case 0xEA: case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: c=2; break;
   case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2: PC++; c=2; break;
   case 0x04: case 0x44: case 0x64: PC++; c=3; break;
   case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4: PC++; c=4; break;
   case 0x0C: PC+=2; c=4; break;
   case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: PC+=2; c=4; break;

   default:
    //unknown opcode: stop on it and report its address
    PC=opaddr;
    saveregs();
    return opaddr;
  }

  m6502clockticks+=c;
  cyclesRemaining-=c;
 }

 saveregs();
 return 0x10000;
}

static void interrupt(WORD vector)
{
 loadregs();
 push(PC>>8); push(PC&0xFF);
 push((P&~F_B)|F_R);
 P|=F_I;
 P&=~F_D;
 PC=m6502rom[vector]|(m6502rom[vector+1]<<8);
 m6502clockticks+=7;
 saveregs();
}

extern "C" DWORD m6502nmi(void)
{
 inNmi++;
 interrupt(0xFFFA);
 return (DWORD)-1;
}

extern "C" DWORD m6502int(void)
{
 if ((m6502af>>8)&F_I) return 0; //interrupts disabled
 interrupt(0xFFFE);
 return (DWORD)-1;
}

extern "C" void m6502reset(void)
{
 inNmi=0;
 m6502x=m6502y=0;
 m6502s=0xFF;
 m6502clockticks=0;
 m6502af=(F_R|F_Z|F_I)<<8;
 m6502pc=m6502rom[0xFFFC]|(m6502rom[0xFFFD]<<8);
}

extern "C" DWORD m6502GetInfo(void)
{
 return 0x00020000; //TIMING_INFO
}
