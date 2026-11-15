//NESticle platform layer for MS-DOS (DJGPP), on SDL3's DOS port.
//
//Takes the place of the original DOS\DOS.CPP, DOS\KEYB.CPP, DOS\MODEX.CPP,
//DOS\TTIMER.CPP, DOS\SMIX.CPP and the TASM joystick/mouse routines, which
//talked to the VGA/VESA BIOS, the keyboard IRQ, the PIT and the Sound
//Blaster directly with Watcom-only code. SDL3 does all of that here (VESA
//or VGA video, keyboard, INT 33h mouse, gameport, Sound Blaster), and this
//file provides what the rest of NESticle expects from the platform (dd.h,
//keyb.h, the sound output functions).
//
//The emulator draws into an 8-bit buffer (screen) of SCREENX x SCREENY;
//this shows it on a VESA 8-bit mode of that size if there is one, else on
//the closest mode, scaled by a whole number (8-bit or hicolor).
//
//Environment: NESTICLE_VIDEO=WxHxBPP picks the video mode (e.g. 640x480x8).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "types.h"
#include "r2img.h"
#include "mouse.h"
#include "input.h"
#include "config.h"
#include "message.h"
#include "dd.h"
#include "keyb.h"
#include "timing.h"
#include "command.h"
#include "prof.h"
#include "sound.h"
#include "nessound.h"
#include "nesvideo.h"
#include "nes.h"

//----------------------------------------------------------------
//globals the rest of NESticle expects from the platform

extern "C" {
int PITCH=320;
int BANKED=0;
}
char *video=0;  //not used: SDL owns the display
char *screen=0; //the 8-bit virtual screen
mouse m;
char errstr[80];
commandline *cline;
extern char configfile[];
extern char waitvsync;
extern int SOUNDRATE;

unsigned ticks=0; //timer ticks (TIMERSPEED a second), for prof.cpp

static int quit=0;
void quitgame() {quit=1;}

void disable() {}
void enable() {}

char *getclipboardtext()
{
 static char *t=0;
 if (t) SDL_free(t);
 t=SDL_GetClipboardText();
 return t;
}

void *loadresource(char *name) {return 0;}

//sb settings: SDL takes them from the BLASTER variable
int option_sb(char *p) {return 1;}
int option_sb16(char *p) {return 1;}

static void shutdown();

void cleanexit(int x)
{
 shutdown();
 printf("Error: %s\n",errstr);
 exit(x);
}

void settimerspeed(int x)
{
 if (x<1) x=1;
 TIMERSPEED=x;
}

//----------------------------------------------------------------
//video

static SDL_Window *window=0;
static SDL_Surface *winsurf=0;  //the window surface (the display)
static int indexed=0;           //is the display 8-bit?
static int scale=1;             //whole number scale of our screen on the display
static SDL_Rect dest;           //where our screen goes on the display
static SDL_Color pal[256];      //current palette (8-bit components)
static Uint32 palpix[256];      //palette as display pixels (hicolor)
static int palchanged=1;

//pick the display mode for a w x h screen
static int pickmode(int w,int h,SDL_DisplayMode *out)
{
 int count=0;
 SDL_DisplayMode **modes=SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(),&count);
 if (!modes || !count) {if (modes) SDL_free(modes); return 0;}

 int ww=0,wh=0,wb=0;
 const char *env=getenv("NESTICLE_VIDEO");
 if (env) sscanf(env,"%dx%dx%d",&ww,&wh,&wb);

 long best=-1;
 for (int i=0; i<count; i++)
  {
   SDL_DisplayMode *md=modes[i];
   int bpp=SDL_BITSPERPIXEL(md->format);
   int isidx=SDL_ISPIXELFORMAT_INDEXED(md->format);
   if (isidx && bpp!=8) continue;
   int s=SDL_min(md->w/w,md->h/h);
   long score;
   if (ww)
    {
     score=1000000-labs((long)md->w*md->h-(long)ww*wh)/16;
     if (md->w==ww && md->h==wh) score+=2000000;
     if (wb && bpp==wb) score+=1000000;
    }
   else
    {
     if (s<1) score=0; //too small: shown cut off
      else
      {
       //8-bit first (no conversion), then a mode the screen fits
       //exactly, then the least to copy
       score=1000000;
       if (isidx) score+=4000000;
       if (md->w==w && md->h==h) score+=2000000;
       score-=((long)md->w*md->h)/4;
       if (!isidx) score+=(bpp==16 ? 300 : bpp==32 ? 200 : bpp==15 ? 100 : 0);
      }
    }
   if (score>best) {best=score; *out=*md;}
  }
 SDL_free(modes);
 return best>=0;
}

static void layout()
{
 scale=SDL_min(winsurf->w/SCREENX,winsurf->h/SCREENY);
 if (scale<1) scale=1;
 dest.w=SCREENX*scale; dest.h=SCREENY*scale;
 dest.x=(winsurf->w-dest.w)/2; if (dest.x<0) dest.x=0;
 dest.y=(winsurf->h-dest.h)/2; if (dest.y<0) dest.y=0;
 SDL_FillSurfaceRect(winsurf,0,0);
}

static void applypalette()
{
 if (indexed)
  {
   SDL_Palette *p=SDL_GetSurfacePalette(winsurf);
   if (p) SDL_SetPaletteColors(p,pal,0,256);
  }
 else
  {
   const SDL_PixelFormatDetails *fd=SDL_GetPixelFormatDetails(winsurf->format);
   for (int i=0; i<256; i++) palpix[i]=SDL_MapRGB(fd,0,pal[i].r,pal[i].g,pal[i].b);
  }
 palchanged=0;
}

//set the display mode for our screen size
static int setvideomode()
{
 SDL_DisplayMode md;
 int havemode=pickmode(SCREENX,SCREENY,&md);
 if (!window)
  {
   window=SDL_CreateWindow("NESticle",havemode ? md.w : 640,havemode ? md.h : 480,SDL_WINDOW_FULLSCREEN);
   if (!window) {sprintf(errstr,"SDL_CreateWindow: %.50s",SDL_GetError()); return 0;}
  }
 if (havemode) SDL_SetWindowFullscreenMode(window,&md);
 SDL_SyncWindow(window);
 winsurf=SDL_GetWindowSurface(window);
 if (!winsurf) {sprintf(errstr,"SDL_GetWindowSurface: %.50s",SDL_GetError()); return 0;}

 indexed=SDL_ISPIXELFORMAT_INDEXED(winsurf->format);
 if (indexed)
  {
   //share the palette of the driver's surface: the one that goes to the DAC
   SDL_Surface *internal=(SDL_Surface *)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                                        "SDL.internal.window.surface",NULL);
   SDL_Palette *p=internal ? SDL_GetSurfacePalette(internal) : NULL;
   if (p) SDL_SetSurfacePalette(winsurf,p);
    else if (!SDL_GetSurfacePalette(winsurf)) SDL_CreateSurfacePalette(winsurf);
  }
 SDL_SetWindowSurfaceVSync(window,waitvsync ? 1 : SDL_WINDOW_SURFACE_VSYNC_DISABLED);
 SDL_HideCursor();
 layout();
 applypalette();
 msg.printf(2,"Video: %dx%d %d-bit, %dx%d screen x%d",winsurf->w,winsurf->h,
   SDL_BITSPERPIXEL(winsurf->format),SCREENX,SCREENY,scale);
 return 1;
}

void changeresolution(int x,int y)
{
 if (x<256) x=256;
 if (y<200) y=200;
 SCREENX=x; SCREENY=y; PITCH=SCREENX;
 if (screen) free(screen);
 screen=(char *)malloc(SCREENY*PITCH+5000);
 memset(screen,0,SCREENY*PITCH);
 setvideomode();
}

void setpalette(PALETTE *p) //8-bit palette
{
 for (int i=0; i<256; i++)
  {
   pal[i].r=p->c[i].r; pal[i].g=p->c[i].g; pal[i].b=p->c[i].b; pal[i].a=255;
  }
 palchanged=1;
}

void setpalettenum(int index,COLOR *c)
{
 index&=255;
 pal[index].r=c->r; pal[index].g=c->g; pal[index].b=c->b; pal[index].a=255;
 palchanged=1;
}

//our screen to the display
static void present()
{
 if (!winsurf) return;
 if (palchanged) applypalette();
 if (SDL_MUSTLOCK(winsurf)) SDL_LockSurface(winsurf);

 int w=SDL_min(SCREENX,(winsurf->w-dest.x)/scale);
 int h=SDL_min(SCREENY,(winsurf->h-dest.y)/scale);
 int bpp=SDL_BYTESPERPIXEL(winsurf->format);
 Uint8 *drow=(Uint8 *)winsurf->pixels+dest.y*winsurf->pitch+dest.x*bpp;
 const Uint8 *srow=(const Uint8 *)screen;

 for (int y=0; y<h; y++,srow+=PITCH)
  {
   Uint8 *d=drow;
   const Uint8 *s=srow;
   if (indexed)
    {
     if (scale==1) memcpy(d,s,w);
      else for (int x=0; x<w; x++) for (int k=0; k<scale; k++) *d++=s[x];
    }
   else switch (bpp)
    {
     case 2:
      {
       Uint16 *o=(Uint16 *)d;
       for (int x=0; x<w; x++) {Uint16 v=(Uint16)palpix[s[x]]; for (int k=0; k<scale; k++) *o++=v;}
      }
      break;
     case 4:
      {
       Uint32 *o=(Uint32 *)d;
       for (int x=0; x<w; x++) {Uint32 v=palpix[s[x]]; for (int k=0; k<scale; k++) *o++=v;}
      }
      break;
     case 3:
      for (int x=0; x<w; x++)
       {
        Uint32 v=palpix[s[x]];
        for (int k=0; k<scale; k++) {d[0]=v; d[1]=v>>8; d[2]=v>>16; d+=3;}
       }
      break;
    }
   for (int k=1; k<scale; k++) memcpy(drow+k*winsurf->pitch,drow,w*scale*bpp);
   drow+=scale*winsurf->pitch;
  }

 if (SDL_MUSTLOCK(winsurf)) SDL_UnlockSurface(winsurf);
 SDL_UpdateWindowSurface(window);
}

//----------------------------------------------------------------
//surfaces (offscreen 8-bit buffers; the name table viewer uses them)

surface::surface(int txw,int tyw) {s=SCR::newSCR(txw,tyw);}
surface::~surface() {free(s);}

char *surface::lock()
{
 if (!s) return 0;
 oldscrx=SCREENX; oldscry=SCREENY; oldpitch=PITCH;
 SCREENX=s->xw;
 SCREENY=s->yw;
 PITCH=s->xw;
 return s->data();
}

void surface::unlock()
{
 if (!s) return;
 SCREENX=oldscrx; SCREENY=oldscry; PITCH=oldpitch;
}

int surface::blt(char *dest,int x,int y)
{
 if (s) {s->draw(dest,x,y); return 1;}
 return 0;
}

int surface::blt(int sx,int sy,int sx2,int sy2,char *dest,int x,int y)
{
 if (!s) return 0;
 if (sx2<=sx || sy2<=sy) return 1;
 if (sx>=s->xw || sy>=s->yw) return 1;
 if (x>=SCREENX || y>=SCREENY) return 1;
 CLIP clip(dest,x,y,x+sx2-sx,y+sy2-sy);
 s->draw(dest,-sx,-sy);
 return 1;
}

//----------------------------------------------------------------
//keyboard (as DOS\KEYB.CPP: XT scan codes, bit 7 set on release)

volatile char kbscan=0;
volatile char kbstat=0;
volatile char keydown[128];
volatile char kbint=0;
int keyqueue=1,keybios=0,keyrepeat=1;
int (*keyboardhandler)(void)=0;

void set_keyboard_func(int (*kfunc)()) {keyboardhandler=kfunc;}
void init_keyboard() {}
void terminate_keyboard() {}

static volatile char kbscanbuf[64];
static volatile int kbhead=0,kbtail=0;

void pushkey(char k)
{
 int n=(kbtail+1)&63;
 if (n==kbhead) return; //full
 kbscanbuf[kbtail]=k;
 kbtail=n;
}

char getkey()
{
 if (kbhead==kbtail) return 0;
 char k=kbscanbuf[kbhead];
 kbhead=(kbhead+1)&63;
 return k;
}

int keyhit() {return kbtail!=kbhead;}

char waitkey()
{
 while (!keyhit()) {SDL_PumpEvents(); SDL_Delay(1);}
 return getkey();
}

static const char s2a[]=
{
0,27,49,50,51,52,53,54,55,56,57,48,45,61,8,9,113,119,101,114,116,121,117,105,111,
112,91,93,13,0,97,115,100,102,103,104,106,107,108,59,39,96,0,92,122,120,99,118, //47
98,110,109,44,46,47,0,42,0,32,(char)128,(char)129,(char)130,(char)131,(char)132,(char)133,(char)134,(char)135,(char)136,(char)137,0,0,0,0,0,45, //74
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const char s2ashift[]=
{
0,27,33,64,35,36,37,94,38,42,40,41,95,43,8,9, 81, 87, 69, 82, 84, 89, 85, 73, 79,
 80,123,125,13,0,65, 83, 68, 70, 71, 72, 74, 75, 76,58,34,126,0,124,90, 88,67, 86, //47
66, 78, 77,60,62,63,0,42,0,32,(char)128,(char)129,(char)130,(char)131,(char)132,(char)133,(char)134,(char)135,(char)136,(char)137,0,0,0,0,0,45, //74
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

char scan2ascii(char sc)
{
 unsigned char s=(unsigned char)sc;
 if (s>=128) return 0;
 if (kbstat&KB_SHIFT) return s2ashift[s];
 return s2a[s];
}

struct keyname {unsigned char scancode; char *name;};
static keyname keynames[]=
{
 {0x48,"Up"},{0x50,"Down"},{0x4B,"Left"},{0x4D,"Right"},
 {0x47,"Home"},{0x4F,"End"},{0x49,"Page Up"},{0x51,"Page Down"},
 {0x52,"Insert"},{0x53,"Delete"},{0x37,"*"},{0x4A,"-"},{0x4E,"+"},
 {0x29,"`"},{0x02,"1"},{0x03,"2"},{0x04,"3"},{0x05,"4"},{0x06,"5"},
 {0x07,"6"},{0x08,"7"},{0x09,"8"},{0x0A,"9"},{0x0B,"0"},{0x0C,"-"},{0x0D,"="},
 {0x1a,"["},{0x1b,"]"},{0x27,";"},{0x28,"'"},{0x2b,"\\"},{0x33,","},{0x34,"."},{0x35,"/"},
 {0x10,"q"},{0x11,"w"},{0x12,"e"},{0x13,"r"},{0x14,"t"},{0x15,"y"},{0x16,"u"},
 {0x17,"i"},{0x18,"o"},{0x19,"p"},
 {0x1E,"a"},{0x1F,"s"},{0x20,"d"},{0x21,"f"},{0x22,"g"},{0x23,"h"},{0x24,"j"},
 {0x25,"k"},{0x26,"l"},
 {0x2c,"z"},{0x2d,"x"},{0x2e,"c"},{0x2f,"v"},{0x30,"b"},{0x31,"n"},{0x32,"m"},
 {0x39,"Space"},{0x1c,"Enter"},{0x0f,"Tab"},{0x0e,"Backspace"},
 {0x1d,"Ctrl"},{0x38,"Alt"},{0x2a,"LShift"},{0x36,"RShift"},
 {KB_F1,"F1"},{KB_F2,"F2"},{KB_F3,"F3"},{KB_F4,"F4"},{KB_F5,"F5"},{KB_F6,"F6"},
 {KB_F7,"F7"},{KB_F8,"F8"},{KB_F9,"F9"},{KB_F10,"F10"},{0x57,"F11"},{0x58,"F12"},
 {0,"<none>"}
};

char *getkeyname(char key)
{
 int i;
 for (i=0; keynames[i].scancode; i++)
  if (keynames[i].scancode==(unsigned char)key) break;
 return keynames[i].name;
}

//SDL scancode -> XT (set 1) scan code
static int xtscan(SDL_Scancode sc)
{
 if (sc>=SDL_SCANCODE_A && sc<=SDL_SCANCODE_Z)
  {
   static const unsigned char az[26]=
    {0x1E,0x30,0x2E,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,
     0x31,0x18,0x19,0x10,0x13,0x1F,0x14,0x16,0x2F,0x11,0x2D,0x15,0x2C};
   return az[sc-SDL_SCANCODE_A];
  }
 if (sc>=SDL_SCANCODE_1 && sc<=SDL_SCANCODE_0) return 0x02+(sc-SDL_SCANCODE_1);
 if (sc>=SDL_SCANCODE_F1 && sc<=SDL_SCANCODE_F10) return 0x3B+(sc-SDL_SCANCODE_F1);
 switch (sc)
  {
   case SDL_SCANCODE_RETURN:      return 0x1C;
   case SDL_SCANCODE_KP_ENTER:    return 0x1C;
   case SDL_SCANCODE_ESCAPE:      return 0x01;
   case SDL_SCANCODE_BACKSPACE:   return 0x0E;
   case SDL_SCANCODE_TAB:         return 0x0F;
   case SDL_SCANCODE_SPACE:       return 0x39;
   case SDL_SCANCODE_MINUS:       return 0x0C;
   case SDL_SCANCODE_EQUALS:      return 0x0D;
   case SDL_SCANCODE_LEFTBRACKET: return 0x1A;
   case SDL_SCANCODE_RIGHTBRACKET:return 0x1B;
   case SDL_SCANCODE_BACKSLASH:   return 0x2B;
   case SDL_SCANCODE_SEMICOLON:   return 0x27;
   case SDL_SCANCODE_APOSTROPHE:  return 0x28;
   case SDL_SCANCODE_GRAVE:       return 0x29;
   case SDL_SCANCODE_COMMA:       return 0x33;
   case SDL_SCANCODE_PERIOD:      return 0x34;
   case SDL_SCANCODE_SLASH:       return 0x35;
   case SDL_SCANCODE_CAPSLOCK:    return 0x3A;
   case SDL_SCANCODE_F11:         return 0x57;
   case SDL_SCANCODE_F12:         return 0x58;
   case SDL_SCANCODE_LCTRL:  case SDL_SCANCODE_RCTRL:  return 0x1D;
   case SDL_SCANCODE_LSHIFT:      return 0x2A;
   case SDL_SCANCODE_RSHIFT:      return 0x36;
   case SDL_SCANCODE_LALT:   case SDL_SCANCODE_RALT:   return 0x38;
   case SDL_SCANCODE_KP_MULTIPLY: return 0x37;
   case SDL_SCANCODE_KP_MINUS:    return 0x4A;
   case SDL_SCANCODE_KP_PLUS:     return 0x4E;
   case SDL_SCANCODE_HOME:   case SDL_SCANCODE_KP_7: return 0x47;
   case SDL_SCANCODE_UP:     case SDL_SCANCODE_KP_8: return 0x48;
   case SDL_SCANCODE_PAGEUP: case SDL_SCANCODE_KP_9: return 0x49;
   case SDL_SCANCODE_LEFT:   case SDL_SCANCODE_KP_4: return 0x4B;
   case SDL_SCANCODE_KP_5:        return 0x4C;
   case SDL_SCANCODE_RIGHT:  case SDL_SCANCODE_KP_6: return 0x4D;
   case SDL_SCANCODE_END:    case SDL_SCANCODE_KP_1: return 0x4F;
   case SDL_SCANCODE_DOWN:   case SDL_SCANCODE_KP_2: return 0x50;
   case SDL_SCANCODE_PAGEDOWN:case SDL_SCANCODE_KP_3:return 0x51;
   case SDL_SCANCODE_INSERT: case SDL_SCANCODE_KP_0: return 0x52;
   case SDL_SCANCODE_DELETE: case SDL_SCANCODE_KP_PERIOD: return 0x53;
   default: return 0;
  }
}

static void keyevent(SDL_Scancode sc,int down,int repeat)
{
 int k=xtscan(sc);
 if (!k) return;
 if (repeat && !keyrepeat) return;
 keydown[k&0x7F]=down ? 0x80 : 0;
 //shift/ctrl/alt only change kbstat
 if (k==0x2A || k==0x36) {if (down) kbstat|=KB_SHIFT; else kbstat&=~KB_SHIFT; return;}
 if (k==0x1D) {if (down) kbstat|=KB_CTRL; else kbstat&=~KB_CTRL; return;}
 if (k==0x38) {if (down) kbstat|=KB_ALT; else kbstat&=~KB_ALT; return;}
 pushkey((char)(down ? k : (k|0x80)));
}

//----------------------------------------------------------------
//mouse

static int mousex=0,mousey=0,mouseb=0;

static void mousemove(float wx,float wy)
{
 int x=((int)wx-dest.x)/scale;
 int y=((int)wy-dest.y)/scale;
 if (x<0) x=0;
 if (y<0) y=0;
 if (x>=SCREENX) x=SCREENX-1;
 if (y>=SCREENY) y=SCREENY-1;
 mousex=x; mousey=y;
}

static int mousebit(int sdlbutton)
{
 switch (sdlbutton)
  {
   case SDL_BUTTON_LEFT:   return MBLEFT;
   case SDL_BUTTON_RIGHT:  return MBRIGHT;
   case SDL_BUTTON_MIDDLE: return MBMIDDLE;
  }
 return 0;
}

//----------------------------------------------------------------
//joystick (the gameport)

static SDL_Joystick *joy=0;

extern "C" void ReadAnalogJoyPos(int *pos,int max)
{
 //centre 512, like a calibrated gameport read
 for (int i=0; i<4; i++)
  {
   int v=0;
   if (joy && i<SDL_GetNumJoystickAxes(joy)) v=SDL_GetJoystickAxis(joy,i);
   pos[i]=512+v*400/32768;
  }
}

extern "C" int ReadAnalogJoyButtons()
{
 int b=0;
 if (joy)
  for (int i=0; i<4 && i<SDL_GetNumJoystickButtons(joy); i++)
   if (SDL_GetJoystickButton(joy,i)) b|=1<<i;
 return b;
}

//----------------------------------------------------------------
//sound: NESticle mixes 16-bit mono blocks; they go to an SDL audio stream

byte soundinstalled=0;
static SDL_AudioStream *stream=0;

void clearprimary()
{
 if (stream) SDL_ClearAudioStream(stream);
}

void primaryoutput(short *buf,int nums)
{
 if (stream) SDL_PutAudioStreamData(stream,buf,nums*2);
}

void primaryskip(int nums) {}

void playsound(SOUND *s) {}

static void initsound()
{
 if (getoption("nosound")) {msg.printf(2,"Sound disabled (-nosound)"); return;}
 SDL_AudioSpec spec;
 spec.format=SDL_AUDIO_S16;
 spec.channels=1;
 spec.freq=SOUNDRATE;
 stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,NULL,NULL);
 if (!stream)
  {
   msg.printf(2,"No sound: %s",SDL_GetError());
   return;
  }
 SDL_ResumeAudioStreamDevice(stream);
 soundinstalled=1;
 msg.printf(2,"Sound: %s, %d Hz",SDL_GetCurrentAudioDriver(),SOUNDRATE);
}

//keep about 3 frames of sound queued
static void updatesound()
{
 if (!stream || !ns) return;
 if (!CPURunning) return;
 int frame=SOUNDRATE/TIMERSPEED;
 int queued=SDL_GetAudioStreamQueued(stream)/2;
 if (queued>frame*6) {SDL_ClearAudioStream(stream); queued=0;}
 int need=frame*3-queued;
 if (need<=0) return;
 if (need>4096) need=4096;
 ns->update(need);
}

//----------------------------------------------------------------
//main

int  initgame();
void updatescreen();
void terminategame();
void gametimer();

static int sdlstarted=0;
static void shutdown()
{
 if (!sdlstarted) return;
 sdlstarted=0;
 if (stream) SDL_DestroyAudioStream(stream);
 stream=0;
 if (joy) SDL_CloseJoystick(joy);
 joy=0;
 if (window) SDL_DestroyWindow(window);
 window=0;
 SDL_Quit();
}

int main(int argc,char *argv[])
{
 //get configuration
 cfg=new config();
 cfg->load(configfile);

 //execute command line (options)
 cline=new commandline;
 cline->parse(argc-1,argv+1);
 cline->execute(0);

 SDL_SetHint(SDL_HINT_APP_NAME,"NESticle");
 if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|(getoption("nosound") ? 0 : SDL_INIT_AUDIO)))
  {
   printf("SDL_Init: %s\n",SDL_GetError());
   if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
  }
 sdlstarted=1;

 int njoy=0;
 SDL_JoystickID *joys=SDL_GetJoysticks(&njoy);
 if (joys && njoy>0) joy=SDL_OpenJoystick(joys[0]);
 if (joys) SDL_free(joys);

 //video
 PITCH=SCREENX;
 screen=(char *)malloc(SCREENY*PITCH+5000);
 memset(screen,0,SCREENY*PITCH);
 if (!setvideomode())
  {
   shutdown();
   printf("Error: %s\n",errstr);
   return 1;
  }

 initsound();

 //initialize game data
 if (initgame())
  {
   shutdown();
   printf("Error initializing game\n");
   return 1;
  }

 //execute post system startup commands (-load, -run...)
 cline->execute(1);

 Uint64 start=SDL_GetTicksNS();
 unsigned lasttime=0;
 int lastspeed=TIMERSPEED;

 do
 {
  pf.input_timer.enter();
  //events
  SDL_Event e;
  m.reset();
  while (SDL_PollEvent(&e))
   switch (e.type)
    {
     case SDL_EVENT_QUIT: quit=1; break;
     case SDL_EVENT_KEY_DOWN: keyevent(e.key.scancode,1,e.key.repeat); break;
     case SDL_EVENT_KEY_UP:   keyevent(e.key.scancode,0,0); break;
     case SDL_EVENT_MOUSE_MOTION: mousemove(e.motion.x,e.motion.y); break;
     case SDL_EVENT_MOUSE_BUTTON_DOWN: mousemove(e.button.x,e.button.y); mouseb|=mousebit(e.button.button); break;
     case SDL_EVENT_MOUSE_BUTTON_UP:   mousemove(e.button.x,e.button.y); mouseb&=~mousebit(e.button.button); break;
    }
  //(position first: a click goes where the mouse is now)
  m.updatexy(mousex,mousey);
  m.updatebut(mouseb);

  //process keys
  while (keyhit())
   {
    kbscan=getkey();
    if (keyboardhandler) keyboardhandler();
   }
  pf.input_timer.leave();

  pf.pageclear_timer.enter();
  if (!cfg->get(CFG_NOFILLEDDESKTOP))
      memset(screen,9*16+14,SCREENY*PITCH);
  pf.pageclear_timer.leave();

  //update screen
  updatescreen();

  //refresh palette
  if (nv) nv->refreshpalette();

  pf.pageflip_timer.enter();
  present();
  pf.pageflip_timer.leave();

  //run the emulation: one gametimer() per timer tick gone by
  if (TIMERSPEED!=lastspeed) {start=SDL_GetTicksNS(); lasttime=0; lastspeed=TIMERSPEED;}
  unsigned now=(unsigned)((SDL_GetTicksNS()-start)*TIMERSPEED/1000000000ull);
  if (now<=lasttime && !quit)
   {
    //early: wait for the next tick
    while (now<=lasttime)
     {
      SDL_Delay(1);
      now=(unsigned)((SDL_GetTicksNS()-start)*TIMERSPEED/1000000000ull);
     }
   }
  int numtimer=0;
  while (now>lasttime)
   {
    if (++numtimer<5) gametimer();
    lasttime++;
    ticks++;
   }
  updatesound();

 } while (!quit);

 set_keyboard_func(0);
 terminategame();
 shutdown();
 return 0;
}
