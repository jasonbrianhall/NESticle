//GUI data: palette, font, mouse cursor and icons.
//
//NESticle loads these from GUI.VOL, which isn't in the source release.
//When there's no GUI.VOL, guivol_generate() builds a set here instead, in
//the same formats (see vol.h, r2img.h, font.h): a 6-bit palette, RLE IMGs,
//and a FONT whose glyphs use color 0xCA for the letter and 0xDB for its
//shadow (createfonts() in main.cpp recolors those for each font).
//
//The palette is laid out the way the GUI code's colors expect (guicolor.h,
//main.cpp): colors 0-15 a grey ramp from black to white, then ramps of
//hues 16 to a row, bright to dark. 224-255 are left for the NES palette.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../types.h"
#include "../r2img.h"
#include "../font.h"
#include "../guivol.h"

#include "font5x8.h"

#define TRANSPARENT 0xFF

//----------------------------------------------------------------
//palette

struct ramp {int r,g,b;}; //brightest color of the row (8-bit)

static const ramp rows[14]=
{
 {0,0,0},        //0  grey, black to white (special)
 {200,208,228},  //1  blue grey (buttons)
 {255,96,88},    //2  red
 {255,170,64},   //3  orange
 {255,240,96},   //4  yellow
 {176,240,80},   //5  lime
 {96,240,120},   //6  green
 {96,232,232},   //7  cyan
 {120,150,255},  //8  blue (dialog boxes)
 {150,200,212},  //9  slate (desktop, menus)
 {255,128,200},  //10 pink
 {200,128,255},  //11 purple
 {255,200,160},  //12 peach
 {200,150,100},  //13 brown
};

static void makepalette(PALETTE *p)
{
 memset(p,0,sizeof(PALETTE));
 for (int i=0; i<16; i++)
  {
   int v=i*255/15;
   p->c[i].r=p->c[i].g=p->c[i].b=v;
  }
 for (int row=1; row<14; row++)
  for (int i=0; i<16; i++)
   {
    //bright (0) to dark (15)
    int k=256-i*15;
    COLOR &c=p->c[row*16+i];
    c.r=rows[row].r*k/256;
    c.g=rows[row].g*k/256;
    c.b=rows[row].b*k/256;
   }
 //to 6 bits, as stored in .VOL files (initgame shifts them back)
 for (int i=0; i<256; i++)
  {
   p->c[i].r>>=2; p->c[i].g>>=2; p->c[i].b>>=2;
  }
}

//----------------------------------------------------------------
//images

//an IMG from a picture given as strings: each char is looked up in
//'keys', and gets the color at the same position in 'colors'; others
//are transparent
static IMG *imgfromstrings(const char **pic,int h,const char *keys,const unsigned char *colors)
{
 int w=strlen(pic[0]);
 SCR *s=SCR::newSCR(w,h);
 char *d=s->data();
 for (int y=0; y<h; y++)
  for (int x=0; x<w; x++)
   {
    const char *k=strchr(keys,pic[y][x]);
    d[y*w+x]=(k && pic[y][x]) ? colors[k-keys] : (char)TRANSPARENT;
   }
 return s->compress((char)TRANSPARENT);
}

//the mouse cursor: a severed hand pointing (the hot spot is the fingertip,
//top left), dripping blood
static IMG *mkcursor()
{
 static const char *pic[]=
 {
  ".XX.............",
  "XWSX............",
  "XWSX............",
  "XWSX............",
  "XWSXXX.XX.......",
  "XWSXWSXWSXX.....",
  "XWSXWSXWSXSX....",
  "XWSSWSSWSXSX....",
  "XWSSSSSSSSSDX...",
  "XWSSSSSSSSSDX...",
  "XWSSSSSSSSDDX...",
  ".XWSSSSSSSDDX...",
  ".XWSSSSSSDDX....",
  "..XWSSSSDDDX....",
  "..XRRRRRRRRX....",
  "..XrRRRRRRrX....",
  "...XXrRXrXX.....",
  ".....RX.R.......",
  ".....r..R.......",
  "........r.......",
 };
 static const unsigned char c[]={0,0xC1,0xC4,0xC8,0x21,0x26};
 return imgfromstrings(pic,20,"XWSDRr",c);
}

static IMG *mkcheckmark()
{
 static const char *pic[]=
 {
  "......X",
  ".....XX",
  "X...XX.",
  "XX.XX..",
  ".XXX...",
  "..X....",
  ".......",
 };
 static const unsigned char c[]={0x62};
 return imgfromstrings(pic,7,"X",c);
}

static IMG *mkarrow(int dir) //0=up 1=down 2=left 3=right
{
 static const char *up[]=   {"...X...","..XXX..",".XXXXX.","XXXXXXX"};
 static const char *down[]={"XXXXXXX",".XXXXX.","..XXX..","...X..."};
 static const char *left[]= {"...X","..XX",".XXX","XXXX",".XXX","..XX","...X"};
 static const char *right[]={"X...","XX..","XXX.","XXXX","XXX.","XX..","X..."};
 static const unsigned char c[]={15};
 switch (dir)
  {
   case 0: return imgfromstrings(up,4,"X",c);
   case 1: return imgfromstrings(down,4,"X",c);
   case 2: return imgfromstrings(left,7,"X",c);
   default: return imgfromstrings(right,7,"X",c);
  }
}

static IMG *mkxmark()
{
 static const char *pic[]=
 {
  "XX...XX",
  ".XX.XX.",
  "..XXX..",
  ".XX.XX.",
  "XX...XX",
 };
 static const unsigned char c[]={15};
 return imgfromstrings(pic,5,"X",c);
}

static IMG *mkicon(int type) //0=play 1=stop 2=playlooped 3=active
{
 static const char *play[]=  {"X.....","XXX...","XXXXX.","XXXXXX","XXXXX.","XXX...","X....."};
 static const char *stop[]=  {"XXXXXX","XXXXXX","XXXXXX","XXXXXX","XXXXXX","XXXXXX","......"};
 static const char *active[]={"..XX..",".XXXX.","XXXXXX","XXXXXX",".XXXX.","..XX..","......"};
 static const unsigned char c[]={15};
 static const unsigned char g[]={0x62};
 switch (type)
  {
   case 0: return imgfromstrings(play,7,"X",g);
   case 1: return imgfromstrings(stop,7,"X",c);
   case 2: {static const char *l[]={".XXXX..","X....X.","X...XXX","X....X.","X......",".XXXX..","......."}; return imgfromstrings(l,7,"X",c);}
   default: return imgfromstrings(active,7,"X",g);
  }
}

static IMG *mkshadowsel()
{
 static const char *pic[]={"XXXXXXXX","XXXXXXXX"};
 static const unsigned char c[]={2};
 return imgfromstrings(pic,2,"X",c);
}

//"Buddy" for the About box: a round fellow drawn from circles
static IMG *mkabout()
{
 const int w=56,h=52;
 SCR *s=SCR::newSCR(w,h);
 char *d=s->data();
 memset(d,TRANSPARENT,w*h);
 const float cx=28,cy=28,r=22;
 for (int y=0; y<h; y++)
  for (int x=0; x<w; x++)
   {
    float dx=x-cx,dy=y-cy;
    float dist=sqrtf(dx*dx+dy*dy);
    unsigned char c=TRANSPARENT;
    if (dist<=r+1.5f) c=0;                       //outline
    if (dist<=r) c=(dx+dy<-8) ? 0xC2 : 0xC5;     //face, lit from top left
    //hair
    if (dist<=r+1.5f && dy<-12+((x/3)&1)*2) c=(dist<=r) ? 0xD9 : 0;
    //eyes
    for (int e=-1; e<=1; e+=2)
     {
      float ex=x-(cx+e*8),ey=y-(cy-3);
      float ed=ex*ex/9+ey*ey/16;
      if (ed<=1) c=15;
      if ((ex-1)*(ex-1)+(ey-1)*(ey-1)<=3) c=0;
     }
    //grin
    {
     float my=y-(cy+4);
     float mr=sqrtf(dx*dx+my*my);
     if (my>0 && mr>=9 && mr<=11.5f && fabsf(dx)<=10) c=0;
     if (my>0 && mr<9 && fabsf(dx)<=8 && my>=2) c=(my<4) ? 15 : 0x26;
    }
    //cheeks
    for (int e=-1; e<=1; e+=2)
     {
      float ex=x-(cx+e*14),ey=y-(cy+5);
      if (ex*ex+ey*ey<=6 && c==0xC5) c=0xA6;
     }
    d[y*w+x]=c;
   }
 return s->compress((char)TRANSPARENT);
}

//----------------------------------------------------------------
//font: proportional, from the 5x8 bitmaps (X11 misc-fixed), with a drop shadow

#define FONTFG 0xCA
#define FONTSH 0xDB

static FONT *mkfont()
{
 FONT *f=(FONT *)malloc(sizeof(FONT));
 f->size=sizeof(FONT);
 for (int a=0; a<128; a++) f->i[a]=0;

 for (int ch=33; ch<127; ch++)
  {
   const unsigned char *g=font5x8[ch-33];
   int left=8,right=-1;
   for (int y=0; y<8; y++)
    for (int x=0; x<8; x++)
     if ((g[y]>>x)&1) {if (x<left) left=x; if (x>right) right=x;}
   if (right<0) continue;
   int gw=right-left+1;
   int w=gw+2; //shadow column + one pixel of space
   int h=9;    //8 + shadow row
   SCR *s=SCR::newSCR(w,h);
   char *d=s->data();
   memset(d,TRANSPARENT,w*h);
   for (int y=0; y<8; y++)
    for (int x=0; x<gw; x++)
     if ((g[y]>>(x+left))&1)
      {
       d[y*w+x]=FONTFG;
       char &sh=d[(y+1)*w+x+1];
       if (sh==(char)TRANSPARENT) sh=FONTSH;
      }
   //letters drawn over earlier shadows
   for (int y=0; y<8; y++)
    for (int x=0; x<gw; x++)
     if ((g[y]>>(x+left))&1) d[y*w+x]=FONTFG;
   IMG *img=s->compress((char)TRANSPARENT);
   f=FONT::addsymbol(f,img,(char)ch);
   free(img);
  }
 return f;
}

//----------------------------------------------------------------

int guivol_generate(GUIVOL &v)
{
 v.pal=(PALETTE *)malloc(sizeof(PALETTE));
 makepalette(v.pal);
 v.cursor=mkcursor();
 v.font=mkfont();
 v.checkmark=mkcheckmark();
 v.umark=mkarrow(0);
 v.dmark=mkarrow(1);
 v.lmark=mkarrow(2);
 v.rmark=mkarrow(3);
 v.about=mkabout();
 v.shadowsel=mkshadowsel();
 v.play=mkicon(0);
 v.stop=mkicon(1);
 v.playlooped=mkicon(2);
 v.active=mkicon(3);
 v.xmark=mkxmark();
 return 1;
}

//----------------------------------------------------------------
//a mouse cursor from CURSOR.BMP, if there is one: to use your own (the
//original NESticle hand, say) without GUI.VOL. Uncompressed 1, 4, 8, 24 or 32
//bit BMP, actual size (up to 64x64). The color of its top left pixel is
//transparent; the hot spot is its topmost pixel (the fingertip).
//Colors map to the nearest GUI palette color.

int cursorhotx=0,cursorhoty=0;

static unsigned rd16(const unsigned char *p) {return p[0]|(p[1]<<8);}
static unsigned rd32(const unsigned char *p) {return p[0]|(p[1]<<8)|(p[2]<<16)|((unsigned)p[3]<<24);}

int loadcursorbmp(GUIVOL &v,const char *filename)
{
 FILE *f=fopen(filename,"rb");
 if (!f) return 0;
 fseek(f,0,SEEK_END);
 long len=ftell(f);
 fseek(f,0,SEEK_SET);
 if (len<54 || len>1000000) {fclose(f); return 0;}
 unsigned char *b=(unsigned char *)malloc(len);
 int ok=fread(b,1,len,f)==(size_t)len;
 fclose(f);

 int w=0,h=0,bpp=0,topdown=0;
 unsigned off=0,dib=0;
 if (ok && b[0]=='B' && b[1]=='M')
  {
   off=rd32(b+10); dib=rd32(b+14);
   w=(int)rd32(b+18); h=(int)rd32(b+22);
   bpp=rd16(b+28);
   unsigned comp=rd32(b+30);
   if (h<0) {h=-h; topdown=1;}
   if (!(comp==0 || (comp==3 && bpp==32))) ok=0;
   if (bpp!=1 && bpp!=4 && bpp!=8 && bpp!=24 && bpp!=32) ok=0;
   if (w<1 || h<1 || w>64 || h>64) ok=0;
  }
 else ok=0;
 int pitch=((w*bpp+31)/32)*4;
 if (ok && off+(long)pitch*h>(unsigned long)len) ok=0;
 if (!ok) {free(b); return 0;}

 //pixel (x,y) as 8-bit RGB
 #define BMPPIX(x,y,r,g,bl) { \
   const unsigned char *row=b+off+(topdown ? (y) : h-1-(y))*pitch; \
   if (bpp<=8) { \
     int ix=bpp==8 ? row[x] : bpp==4 ? (row[(x)/2]>>((x)&1 ? 0 : 4))&15 : (row[(x)/8]>>(7-((x)&7)))&1; \
     const unsigned char *pe=b+14+dib+ix*4; bl=pe[0]; g=pe[1]; r=pe[2];} \
   else {const unsigned char *px=row+(x)*(bpp/8); bl=px[0]; g=px[1]; r=px[2];} }

 int tr,tg,tb;
 BMPPIX(0,0,tr,tg,tb);
 SCR *s=SCR::newSCR(w,h);
 char *d=s->data();
 int hot=0;
 for (int y=0; y<h; y++)
  for (int x=0; x<w; x++)
   {
    int r,g,bl;
    BMPPIX(x,y,r,g,bl);
    if (r==tr && g==tg && bl==tb) {d[y*w+x]=(char)TRANSPARENT; continue;}
    if (!hot) {hot=1; cursorhotx=x; cursorhoty=y;}
    //nearest GUI color (not the NES colors, 224-255); the palette is 6-bit here
    int best=0,bestd=0x7FFFFFFF;
    for (int i=0; i<224; i++)
     {
      int dr=v.pal->c[i].r-(r>>2),dg=v.pal->c[i].g-(g>>2),db=v.pal->c[i].b-(bl>>2);
      int dd=dr*dr*3+dg*dg*4+db*db*2;
      if (dd<bestd) {bestd=dd; best=i;}
     }
    if (best==TRANSPARENT) best=0;
    d[y*w+x]=(char)best;
   }
 #undef BMPPIX
 free(b);
 if (!hot) {free(s); return 0;}
 v.cursor=s->compress((char)TRANSPARENT);
 return 1;
}
