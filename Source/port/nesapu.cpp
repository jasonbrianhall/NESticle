//NES APU sound for NESticle.
//
//NESticle's own sound (nessound.cpp) was still a sketch when the source was
//released: two rough square waves and a triangle, no noise. This replaces
//the sound generation with the APU channels of WarpNES
//(github.com/jasonbrianhall/warpnes, Emulation/APU.cpp): two pulse
//channels with envelope and sweep, triangle with linear counter, and noise,
//mixed with the usual nonlinear NES mixer. nessound.cpp passes it every
//register write and asks it for samples.
//
//Changes from WarpNES: samples are generated on demand at any rate, the
//channel timers advance by phase accumulators instead of one step at a
//time, and the frame sequencer runs on sample time (240 Hz).

#include <string.h>

#include "nesapu.h"

static const unsigned char lengthTable[32]=
{
 10,254,20, 2,40, 4,80, 6,160, 8,60,10,14,12,26,14,
 12, 16,24,18,48,20,96,22,192,24,72,26,16,28,32,30
};

static const unsigned char dutyTable[4][8]=
{
 {0,1,0,0,0,0,0,0},
 {0,1,1,0,0,0,0,0},
 {0,1,1,1,1,0,0,0},
 {1,0,0,1,1,1,1,1}
};

static const unsigned char triangleTable[32]=
{
 15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
};

static const unsigned short noiseTable[16]=
{
 4,8,16,32,64,96,128,160,202,254,380,508,762,1016,2034,4068
};

#define CPUCLOCK 1789773

//----------------------------------------------------------------
struct Envelope
{
 bool loop,enabled,start;
 unsigned char period,value,volume,constant;

 void write(unsigned char v)
 {
  loop=(v>>5)&1;
  enabled=!((v>>4)&1);
  period=v&15;
  constant=v&15;
 }
 void step()
 {
  if (start) {volume=15; value=period; start=false;}
  else if (value>0) value--;
  else
   {
    if (volume>0) volume--; else if (loop) volume=15;
    value=period;
   }
 }
 unsigned char out() {return enabled ? volume : constant;}
};

struct Pulse
{
 int channel;
 bool enabled,lengthEnabled;
 unsigned char lengthValue;
 unsigned short timerPeriod;
 unsigned char dutyMode,dutyValue;
 bool sweepReload,sweepEnabled,sweepNegate;
 unsigned char sweepShift,sweepPeriod,sweepValue;
 Envelope env;
 unsigned phase; //cpu cycles (16.16) into the current duty step

 void reset(int ch) {memset(this,0,sizeof(*this)); channel=ch;}

 void writeControl(unsigned char v)
 {
  dutyMode=(v>>6)&3;
  lengthEnabled=!((v>>5)&1);
  env.write(v);
 }
 void writeSweep(unsigned char v)
 {
  sweepEnabled=(v>>7)&1;
  sweepPeriod=((v>>4)&7)+1;
  sweepNegate=(v>>3)&1;
  sweepShift=v&7;
  sweepReload=true;
 }
 void writeTimerLow(unsigned char v) {timerPeriod=(timerPeriod&0xFF00)|v;}
 void writeTimerHigh(unsigned char v)
 {
  if (enabled) lengthValue=lengthTable[v>>3];
  timerPeriod=(timerPeriod&0xFF)|((v&7)<<8);
  env.start=true;
  dutyValue=0;
 }
 void stepSweep()
 {
  if (sweepReload)
   {
    if (sweepEnabled && sweepValue==0) sweepTarget();
    sweepValue=sweepPeriod;
    sweepReload=false;
   }
  else if (sweepValue>0) sweepValue--;
  else
   {
    if (sweepEnabled) sweepTarget();
    sweepValue=sweepPeriod;
   }
 }
 void sweepTarget()
 {
  if (!sweepShift) return;
  int delta=timerPeriod>>sweepShift;
  if (sweepNegate)
   {
    timerPeriod-=delta;
    if (channel==1) timerPeriod--;
   }
  else timerPeriod+=delta;
 }
 void stepLength() {if (lengthEnabled && lengthValue>0) lengthValue--;}

 //advance by c cpu cycles (16.16)
 void run(unsigned c)
 {
  unsigned step=((unsigned)timerPeriod+1)*2<<16;
  phase+=c;
  while (phase>=step) {phase-=step; dutyValue=(dutyValue+1)&7;}
 }
 unsigned char output()
 {
  if (!enabled || lengthValue==0) return 0;
  if (dutyTable[dutyMode][dutyValue]==0) return 0;
  if (timerPeriod<8 || timerPeriod>0x7FF) return 0;
  return env.out();
 }
};

struct Triangle
{
 bool enabled,lengthEnabled;
 unsigned char lengthValue;
 unsigned short timerPeriod;
 unsigned char dutyValue,counterPeriod,counterValue;
 bool counterReload;
 unsigned phase;

 void reset() {memset(this,0,sizeof(*this));}
 void writeControl(unsigned char v)
 {
  lengthEnabled=!((v>>7)&1);
  counterPeriod=v&0x7F;
 }
 void writeTimerLow(unsigned char v) {timerPeriod=(timerPeriod&0xFF00)|v;}
 void writeTimerHigh(unsigned char v)
 {
  if (enabled) lengthValue=lengthTable[v>>3];
  timerPeriod=(timerPeriod&0xFF)|((v&7)<<8);
  counterReload=true;
 }
 void run(unsigned c)
 {
  unsigned step=((unsigned)timerPeriod+1)<<16;
  if (timerPeriod<2) return; //ultrasonic: hold (avoids popping)
  phase+=c;
  while (phase>=step)
   {
    phase-=step;
    if (lengthValue>0 && counterValue>0) dutyValue=(dutyValue+1)&31;
   }
 }
 void stepLength() {if (lengthEnabled && lengthValue>0) lengthValue--;}
 void stepCounter()
 {
  if (counterReload) counterValue=counterPeriod;
  else if (counterValue>0) counterValue--;
  if (lengthEnabled) counterReload=false;
 }
 unsigned char output()
 {
  if (!enabled) return 0;
  return triangleTable[dutyValue];
 }
};

struct Noise
{
 bool enabled,mode,lengthEnabled;
 unsigned short shiftRegister;
 unsigned char lengthValue;
 unsigned short timerPeriod;
 Envelope env;
 unsigned phase;

 void reset() {memset(this,0,sizeof(*this)); shiftRegister=1; timerPeriod=noiseTable[0];}
 void writeControl(unsigned char v)
 {
  lengthEnabled=!((v>>5)&1);
  env.write(v);
 }
 void writePeriod(unsigned char v)
 {
  mode=(v&0x80)!=0;
  timerPeriod=noiseTable[v&0x0F];
 }
 void writeLength(unsigned char v)
 {
  if (enabled) lengthValue=lengthTable[v>>3];
  env.start=true;
 }
 void run(unsigned c)
 {
  unsigned step=(unsigned)timerPeriod<<16;
  phase+=c;
  while (phase>=step)
   {
    phase-=step;
    int shift=mode ? 6 : 1;
    unsigned short b1=shiftRegister&1;
    unsigned short b2=(shiftRegister>>shift)&1;
    shiftRegister>>=1;
    shiftRegister|=(b1^b2)<<14;
   }
 }
 void stepLength() {if (lengthEnabled && lengthValue>0) lengthValue--;}
 unsigned char output()
 {
  if (!enabled || lengthValue==0) return 0;
  if (shiftRegister&1) return 0;
  return env.out();
 }
};

//----------------------------------------------------------------
static Pulse pulse1,pulse2;
static Triangle triangle;
static Noise noise;

static int frameValue;   //frame sequencer step
static unsigned frameAcc; //sample time towards the next step (16.16)
static int frameMode5;    //5-step mode ($4017 bit 7)

static short pulseTable[31];
static short tndTable[203];
static int tablesmade=0;
static int dcin,dcout;

static void maketables()
{
 //the NES mixer, to 16 bits; full volume on all channels ~ 0x7000
 for (int i=0; i<31; i++)
   pulseTable[i]=(short)(i ? 95.52/(8128.0/i+100.0)*0xA000 : 0);
 for (int i=0; i<203; i++)
   tndTable[i]=(short)(i ? 163.67/(24329.0/i+100.0)*0xA000 : 0);
 tablesmade=1;
}

void nesapu_reset()
{
 if (!tablesmade) maketables();
 pulse1.reset(1);
 pulse2.reset(2);
 triangle.reset();
 noise.reset();
 frameValue=0;
 frameAcc=0;
 frameMode5=0;
}

static void stepEnvelope()
{
 pulse1.env.step();
 pulse2.env.step();
 triangle.stepCounter();
 noise.env.step();
}

static void stepSweepLength()
{
 pulse1.stepSweep();  pulse2.stepSweep();
 pulse1.stepLength(); pulse2.stepLength();
 triangle.stepLength();
 noise.stepLength();
}

static void stepFrameSequencer()
{
 if (!frameMode5)
  {
   frameValue=(frameValue+1)%4;
   stepEnvelope();
   if (frameValue==1 || frameValue==3) stepSweepLength();
  }
 else
  {
   frameValue=(frameValue+1)%5;
   if (frameValue!=4) stepEnvelope();
   if (frameValue==1 || frameValue==3) stepSweepLength();
  }
}

void nesapu_write(unsigned short address,unsigned char value)
{
 switch (address)
 {
  case 0x4000: pulse1.writeControl(value); break;
  case 0x4001: pulse1.writeSweep(value); break;
  case 0x4002: pulse1.writeTimerLow(value); break;
  case 0x4003: pulse1.writeTimerHigh(value); break;
  case 0x4004: pulse2.writeControl(value); break;
  case 0x4005: pulse2.writeSweep(value); break;
  case 0x4006: pulse2.writeTimerLow(value); break;
  case 0x4007: pulse2.writeTimerHigh(value); break;
  case 0x4008: triangle.writeControl(value); break;
  case 0x400A: triangle.writeTimerLow(value); break;
  case 0x400B: triangle.writeTimerHigh(value); break;
  case 0x400C: noise.writeControl(value); break;
  case 0x400E: noise.writePeriod(value); break;
  case 0x400F: noise.writeLength(value); break;
  case 0x4015:
   pulse1.enabled=(value&1)!=0;
   pulse2.enabled=(value&2)!=0;
   triangle.enabled=(value&4)!=0;
   noise.enabled=(value&8)!=0;
   if (!pulse1.enabled) pulse1.lengthValue=0;
   if (!pulse2.enabled) pulse2.lengthValue=0;
   if (!triangle.enabled) triangle.lengthValue=0;
   if (!noise.enabled) noise.lengthValue=0;
   break;
  case 0x4017:
   frameMode5=(value&0x80)!=0;
   frameValue=0;
   frameAcc=0;
   if (frameMode5) {stepEnvelope(); stepSweepLength();}
   break;
 }
}

//mix n samples at 'rate' Hz into buf (added to what's there)
void nesapu_mix(short *buf,int n,int rate)
{
 if (!tablesmade) maketables();
 if (rate<=0) return;
 unsigned cyc=(unsigned)(((double)CPUCLOCK/rate)*65536.0); //cpu cycles per sample (16.16)
 unsigned fstep=(unsigned)((double)rate/240.0*65536.0);     //samples per sequencer step (16.16)

 for (int i=0; i<n; i++)
  {
   frameAcc+=0x10000;
   if (frameAcc>=fstep) {frameAcc-=fstep; stepFrameSequencer();}

   pulse1.run(cyc);
   pulse2.run(cyc);
   triangle.run(cyc);
   noise.run(cyc);

   int p=pulse1.output()+pulse2.output();
   int t=3*triangle.output()+2*noise.output();
   //DC blocker (the mixer output is all positive)
   int x=pulseTable[p]+tndTable[t];
   dcout=x-dcin+(dcout*1020>>10);
   dcin=x;
   int s=buf[i]+dcout;
   if (s>32767) s=32767;
   if (s<-32768) s=-32768;
   buf[i]=(short)s;
  }
}
