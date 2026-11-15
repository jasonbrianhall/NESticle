//C++ versions of NESticle's x86 assembly drawing routines.
//
//Replaces TILE.ASM, SPRITE.ASM, SPRITEBG.ASM (NES pattern drawing) and
//R2IMGASM.ASM (RLE images, lines, rectangles), which were TASM sources.
//Same calling interface, same clipping against SCREENX/SCREENY, same PITCH.

#include <string.h>

#include "../types.h"
#include "../r2img.h"
#include "../ppu.h"
#include "../nesvideo.h"

extern "C" int PITCH,SCREENX,SCREENY,BANKED;

//----------------------------------------------------------------
//NES patterns (8x8, one byte per pixel holding the 2-bit color)

//colors 224-239 are the background palettes, 240-255 the sprite palettes
#define BGCOLORBASE 224
#define SPCOLORBASE (224+16)

static inline int clip8(int &x,int &y,int &sx,int &sy,int &w,int &h)
{
 sx=0; sy=0; w=8; h=8;
 if (x<0) {sx=-x; w+=x; x=0;}
 if (y<0) {sy=-y; h+=y; y=0;}
 if (x+w>SCREENX) w=SCREENX-x;
 if (y+h>SCREENY) h=SCREENY-y;
 return w>0 && h>0;
}

extern "C" void draw_tile_asm(bitmap8x8 *b,char *dest,int x,int y,byte pal)
{
 int sx,sy,w,h;
 if (!clip8(x,y,sx,sy,w,h)) return;
 byte c=(byte)((pal<<2)|BGCOLORBASE);
 byte *d=(byte *)dest+y*PITCH+x;
 for (int j=0; j<h; j++,d+=PITCH)
  {
   const byte *s=&b->s[sy+j][sx];
   for (int i=0; i<w; i++) d[i]=s[i]|c;
  }
}

//o: bit0=flip x, bit1=flip y
//behind: only draw over pixels that are background color 0 (not sprites, not
//opaque background); the asm tested dest&0x13
static inline void drawsprite(bitmap8x8 *b,char *dest,int x,int y,int o,byte pal,int behind)
{
 int sx,sy,w,h;
 int ox=x,oy=y;
 if (!clip8(x,y,sx,sy,w,h)) return;
 byte c=(byte)((pal<<2)|SPCOLORBASE);
 byte *d=(byte *)dest+y*PITCH+x;
 for (int j=0; j<h; j++,d+=PITCH)
  {
   int srow=(y+j)-oy;  if (o&2) srow=7-srow;
   for (int i=0; i<w; i++)
    {
     int scol=(x+i)-ox; if (o&1) scol=7-scol;
     byte p=b->s[srow][scol];
     if (!p) continue;
     if (behind && (d[i]&0x13)) continue;
     d[i]=p|c;
    }
  }
}

extern "C" void draw_sprite_asm(bitmap8x8 *b,char *dest,int x,int y,int o,byte pal)
 {drawsprite(b,dest,x,y,o,pal,0);}

extern "C" void draw_sprite_behind_asm(bitmap8x8 *b,char *dest,int x,int y,int o,byte pal)
 {drawsprite(b,dest,x,y,o,pal,1);}


//----------------------------------------------------------------
//RLE images (IMG, see r2img.cpp for the format)
//each scanline: [transparent run][opaque run][opaque bytes...] repeated
//o&2 (IMG_FLIPX): mirror horizontally

extern "C" void drawimager2(IMG *s,char *d,int x,int y,int o)
{
 if (!s) return;
 int *yd=s->ydisp();
 int xw=s->xw;
 for (int j=0; j<s->yw; j++)
  {
   int dy=y+j;
   if (dy<0) continue;
   if (dy>=SCREENY) break;
   unsigned char *src=((unsigned char *)s)+yd[j];
   char *line=d+dy*PITCH;
   int px=0;
   while (px<xw)
    {
     int run=*src++;
     px+=run;
     if (px>=xw) break;
     run=*src++;
     for (int k=0; k<run; k++,px++)
      {
       int dx=(o&IMG_FLIPX) ? x+xw-1-px : x+px;
       if (dx>=0 && dx<SCREENX) line[dx]=src[k];
      }
     src+=run;
    }
  }
}

//uncompressed copy, src rows are 'pitch' apart; already clipped by caller
extern "C" void drawscr(char *s,char *d,int xw,int yw,int pitch)
{
 if (xw<=0) return;
 for (; yw>0; yw--,d+=PITCH,s+=pitch) memcpy(d,s,xw);
}

//palettes go to the VGA DAC in the original; here the platform layer owns
//the palette (setpalette), so these are no-ops
extern "C" void loadpalette8(struct VBE_palette *c,int first,int num) {}
extern "C" void loadpalette6(struct VBE_palette *c,int first,int num) {}

//----------------------------------------------------------------
//lines and rectangles

//horizontal line from x to x2 (x2 not drawn)
extern "C" void drawhline(char *d,int color,int x,int y,int x2)
{
 if (y<0 || y>=SCREENY) return;
 if (x>=SCREENX) return;
 if (x<0) x=0;
 if (x2<0) return;
 if (x2>=SCREENX) x2=SCREENX-1;
 if (x2<=x) return;
 memset(d+y*PITCH+x,color,x2-x);
}

//vertical line from y to y2 (y2 not drawn)
extern "C" void drawvline(char *d,int color,int x,int y,int y2)
{
 if (x<0 || x>=SCREENX) return;
 if (y>SCREENY) return;
 if (y<0) y=0;
 if (y2<0) return;
 if (y2>=SCREENY) y2=SCREENY-1;
 for (d+=y*PITCH+x; y<y2; y++,d+=PITCH) *d=(char)color;
}

static inline int cliprect(int &x,int &y,int &xw,int &yw)
{
 if (x<0) {xw+=x; x=0;}
 if (y<0) {yw+=y; y=0;}
 if (x+xw>SCREENX) xw=SCREENX-x;
 if (y+yw>SCREENY) yw=SCREENY-y;
 return xw>0 && yw>0;
}

extern "C" void drawrect(char *d,int color,int x,int y,int xw,int yw)
{
 if (!cliprect(x,y,xw,yw)) return;
 for (d+=y*PITCH+x; yw>0; yw--,d+=PITCH) memset(d,color,xw);
}

extern "C" void drawrectmap(char *d,COLORMAP *c,int x,int y,int xw,int yw)
{
 if (!cliprect(x,y,xw,yw)) return;
 for (d+=y*PITCH+x; yw>0; yw--,d+=PITCH)
  for (int i=0; i<xw; i++) d[i]=c->c[(unsigned char)d[i]];
}

