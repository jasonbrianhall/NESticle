//CPU handler  starts/stops cpu etc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
//#include <process.h>
#include <unistd.h>

#include "dd.h"

#include "types.h"

#include "message.h"
#include "file.h"

#include "keyb.h"

#include "m6502.h"

#include "nes.h"
#include "nesvideo.h"
#include "slist.h"

#include "prof.h"

int HBLANKCYCLES=115;
int VBLANKLINES=33;
int TIMERSPEED=60;

byte CPUpaused=0; //flag for when cpu is paused
byte CPURunning=0; //flag for when cpu is running
int CPUtickframe=0; //tick a frame at a time
byte CPUtrace=0; //trace one instruction

volatile int cycles;

void startframe();
void startvblank();

#include "timing.h"

extern volatile byte frame;

//are we currently in a virtual frame ? (emulating)
volatile byte inemu=0;

void m_showmessages();

void disasm(char *s,byte *base,word pc);


void cpuabort()
{
 word addr=m6502pc;
 msg.error("Unrecognized opcode %X at addr: %X",
    (addr<0x8000) ? ram[addr] : m6502rom[addr],addr);
 inemu=0;
 m_stop();
 m_showmessages();
 pf.cpu_timer.leave();


 int numrom8k=numrom*2;
 for (int i=0; i<numrom8k; i++)
  {
   char s[128];
   disasm(s,((byte *)ROM)+i*0x2000,m6502pc&0x1FFF);
   msg.printf(1,"bank[%X] %s",i,s);
  }
  
}


//get current virtual scanline being draw
//line 0 = scanline 8  line 224=scanline 232
int getscanline()
{
 if (!frame) return 0;
// return (m6502clockticks+HBLANKCYCLES/2)/HBLANKCYCLES;
 return m6502clockticks/HBLANKCYCLES;
}


//--------------------
//irq stuff
int doIRQ=0;
int IRQline=0;

void setIRQ(int line)
{
 if (line<=0 || line>240) {doIRQ=0; IRQline=0; return;}

 doIRQ=1;
 if (frame)
  {
   IRQline=line+getscanline();
   cyclesRemaining=0; //abort!
  } else IRQline=line-STARTFRAME;

// msg.printf(1,"line=%d IRQline=%X line=%d",line,IRQline,getscanline());
}


//--------------------------------------
//emu heartbeat (once per virtual frame)
void tickemu()
{
 if (CPUtrace)
  {
   char s[128];
//   disasm((char *)s,(byte *)((m6502pc<0x8000) ? ram : m6502rom),m6502pc);
    disasm(s,ram,m6502pc);
   msg.printf(1,s);
   m6502exec(1);
   CPUtrace=0;
   return;
  }


 if (inemu) return; inemu=1;

 pf.cpu_timer.enter();

 //reset event list
 nv->sl.reset();

 //execute during vblank
 startvblank();
// msg.printf(1,"startvblank");
 m6502clockticks=0;
 if (ram[0x2000]&0x80) m6502nmi(); //generate vblank?
 if (m6502exec(VBLANKCYCLES)!=0x10000) {cpuabort(); return;}

 //execute during virtual frame...
 m6502clockticks=0; //STARTFRAME*HBLANKCYCLES; //reset clock ticks
 startframe();
// msg.printf(1,"startframe");

 if (!doIRQ || !IRQline)
  { //dont bother with IRQ
   if (m6502exec(FRAMECYCLES)!=0x10000) {cpuabort(); return;}
  } else
  { //do IRQ
    int framecycles=FRAMECYCLES;
    while (m6502clockticks<framecycles) //until we've done all the cycles
     {
      int line=getscanline(); //current scanline
      if (line<IRQline)
       {
        m6502exec((IRQline-line)*HBLANKCYCLES); //execute until IRQ
        m6502int();   //int!
        nv->sl.addevent(SE_IRQLINE,0);
  //      msg.printf(1,"IRQ");
       } else m6502exec(framecycles-m6502clockticks); //just execute rest of frame
     }
   }

 //create context list based on event list
 nv->sc.create(nv->sl);

 //done
 pf.cpu_timer.leave();
 pf.cycles.add(CYCLESPERTICK);
 pf.vframes.inc();
 inemu=0;
}

//void m_execute()
void m_reset()
{
 if (!romloaded) {msg.error("ROM not loaded"); return;}
// if (CPURunning) {msg.error("CPU already running"); return;}
 if (resetNEShardware()!=0)
   { msg.error("Unable to reset NES hardware"); return;}

 setIRQ(0); //disable IRQs
 msg.printf(3,"CPU emulation started");
// msg.printf(1,"%X",m6502pc);
 CPURunning=1;
 CPUtrace=0;
 CPUpaused=0;
}

void m_stop()
{
 if (!romloaded) return;
 CPUpaused=0;
 if (CPURunning) msg.printf(3,"CPU emulation stopped");
 CPURunning=0;
 CPUtrace=0;
 inemu=0;

 //copy battery backed ram
 if (batterymem) batterymem->copyfromRAM();
}

void m_resume()
{
 if (!romloaded) {msg.error("ROM not loaded"); return;}
 if (CPURunning) {msg.error("CPU already running"); return;}
 if (!CPUpaused) {msg.error("CPU not paused"); return;}

 msg.printf(3,"CPU emulation resumed");
 CPURunning=1;
 CPUpaused=0;
}

void m_pause()
{
 if (!romloaded) return;
 if (!CPURunning) {msg.error("CPU not running"); return;}
 CPUpaused=1;
 CPURunning=0;
 msg.printf(3,"CPU emulation paused");
}

void m_advanceframe()
{
 if (!romloaded) return;
 if (!CPURunning && !CPUpaused) m_reset();
 if (!CPUpaused) m_pause();
 if (!CPUpaused) return;
 CPUtickframe++;
}

void m_step()
{
 if (!romloaded) return;
 if (!CPURunning && !CPUpaused) m_reset();
 if (!CPUpaused) m_pause();
 if (!CPUpaused) return;
 CPUtrace=1;
}

//-----------------------------------------------------------
//-----------------------------------------------------------



