//NES APU sound generation (see nesapu.cpp)
#ifndef _NESAPU_H_
#define _NESAPU_H_
void nesapu_reset();
void nesapu_write(unsigned short address,unsigned char value);
void nesapu_mix(short *buf,int n,int rate); //adds n samples at rate Hz to buf
#endif
