//GUI controls and windows declared in gui.h (see guirect.cpp for why this
//file exists): static text/images, buttons, check boxes, edit controls,
//dialog boxes, scroll bars and list boxes.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "../types.h"
#include "../r2img.h"
#include "../font.h"
#include "../dd.h"
#include "../mouse.h"
#include "../gui.h"
#include "../guimenu.h"
#include "../guicolor.h"
#include "../guivol.h"
#include "../keyb.h"

extern char *screen;

#define TITLEH 11  //height of a dialog box title bar
#define BARH   18  //height of the button bar of one/two button boxes
#define SBW    10  //width of a scroll bar

static inline int mouseover(GUIrect *g)
 {return m.x>=g->x1 && m.x<g->x2 && m.y>=g->y1 && m.y<g->y2;}

//----------------------------------------------------------------
//static stuff

void GUIbar::draw(char *d)
{
 fill(color);
 GUIrect::draw(d);
}

GUIstatictext::GUIstatictext(GUIrect *p,int f,char *str,int x,int y)
 :GUIrect(p,x,y,x,y+10)
{
 fontnum=f;
 text[0]=0;
 settext(str);
}

void GUIstatictext::settext(char *s)
{
 strncpy(text,s ? s : "",79); text[79]=0;
 x2=x1+font[fontnum]->getwidth(text);
}

void GUIstatictext::draw(char *dest)
{
 font[fontnum]->draw(text,dest,x1,y1);
}

void GUIstaticcenteredtext::draw(char *dest)
{
 font[fontnum]->drawcentered(text,dest,(x1+x2)/2,y1);
}

GUIstaticimage::GUIstaticimage(GUIrect *p,IMG *i,int x,int y)
 :GUIrect(p,x,y,x+(i ? i->xw : 0),y+(i ? i->yw : 0))
{
 img=i;
}

void GUIstaticimage::draw(char *dest)
{
 if (img) img->draw(dest,x1,y1);
}

//----------------------------------------------------------------
//buttons
//click: sends GUIMSG_HELDDOWN at once and repeatedly while held over it
//release over it: sends GUIMSG_PUSHED

GUIbutton::GUIbutton(GUIrect *p,int x,int y,int xw,int yw)
 :GUIrect(p,x,y,x+xw,y+yw)
{
 depressed=0;
}

GUIrect *GUIbutton::click(mouse &mm)
{
 depressed=1;
 btimer.set(18);
 if (parent) parent->sendmessage(this,GUIMSG_HELDDOWN);
 return this;
}

int GUIbutton::release(mouse &mm)
{
 int over=mouseover(this);
 depressed=0;
 //(parent may delete us)
 if (over && parent) parent->sendmessage(this,GUIMSG_PUSHED);
 return 1;
}

void GUIbutton::draw(char *dest)
{
 int down=depressed && mouseover(this);
 fill(down ? CLR_BUTTONOUTERDOWN : CLR_BUTTONOUTER);
 drawrect(dest,down ? CLR_BUTTONINNERDOWN : CLR_BUTTONINNER,x1+1,y1+1,width()-2,height()-2);
 drawhline(dest,down ? CLR_BUTTONINNER : CLR_BUTTONOUTERDOWN,x1+1,y1+1,x2-1);
 GUIrect::draw(dest);

 //held down: repeat
 if (depressed && down && btimer.check())
  {
   btimer.set(3);
   if (parent) parent->sendmessage(this,GUIMSG_HELDDOWN);
  }
}

//x,y is the center of the button
GUItextbutton::GUItextbutton(GUIrect *p,char *s,int x,int y)
 :GUIbutton(p,x,y,0,13)
{
 text[0]=0;
 settext(s);
 moverel(-width()/2,-height()/2);
}

void GUItextbutton::settext(char *s)
{
 strncpy(text,s ? s : "",79); text[79]=0;
 bwidth=font[FNT_TEXTBUTTON]->getwidth(text)+12;
 x2=x1+bwidth;
}

void GUItextbutton::draw(char *dest)
{
 GUIbutton::draw(dest);
 int down=depressed && mouseover(this);
 font[down ? FNT_TEXTBUTTONDOWN : FNT_TEXTBUTTON]->drawcentered(text,dest,(x1+x2)/2+down,y1+3+down);
}

GUIimagebutton::GUIimagebutton(GUIrect *p,IMG *i,int x,int y)
 :GUIbutton(p,x,y,(i ? i->xw : 0)+4,(i ? i->yw : 0)+4)
{
 img=i;
}

void GUIimagebutton::draw(char *dest)
{
 GUIbutton::draw(dest);
 if (!img) return;
 int down=depressed && mouseover(this);
 img->draw(dest,x1+(width()-img->xw)/2+down,y1+(height()-img->yw)/2+down);
}

//----------------------------------------------------------------
//check box

GUIcheckbox::GUIcheckbox(GUIrect *p,char *s,int x,int y,int state)
 :GUIrect(p,x,y,x,y+10)
{
 strncpy(text,s ? s : "",79); text[79]=0;
 x2=x1+12+font[FNT_CHECKBOX]->getwidth(text);
 checked=state;
 depressed=0;
}

int GUIcheckbox::keyhit(char kbscan,char key)
{
 if (kbscan==KB_SPACE || kbscan==KB_ENTER)
  {
   checked^=1;
   if (parent) parent->sendmessage(this,checked ? GUIMSG_CHECKED : GUIMSG_UNCHECKED);
   return 1;
  }
 return 0;
}

GUIrect *GUIcheckbox::click(mouse &mm) {depressed=1; return this;}

int GUIcheckbox::release(mouse &mm)
{
 depressed=0;
 if (mouseover(this))
  {
   checked^=1;
   if (parent) parent->sendmessage(this,checked ? GUIMSG_CHECKED : GUIMSG_UNCHECKED);
  }
 return 1;
}

void GUIcheckbox::draw(char *dest)
{
 drawrect(dest,CLR_CHECKBOXOUTER,x1,y1+1,9,9);
 drawrect(dest,CLR_CHECKBOXINNER,x1+1,y1+2,7,7);
 if (checked && guivol.checkmark) guivol.checkmark->draw(dest,x1+1,y1+2);
 font[(focus || depressed) ? FNT_CHECKBOXDOWN : FNT_CHECKBOX]->draw(text,dest,x1+12,y1+1);
}

//----------------------------------------------------------------
//edit controls: prompt, then an input area inputmaxw wide

GUIedit::GUIedit(GUIrect *p,char *pmpt,int x,int y,int xw)
 :GUIrect(p,x,y,x,y+10)
{
 strncpy(prompt,pmpt ? pmpt : "",79); prompt[79]=0;
 promptw=font[FNT_EDITPROMPT]->getwidth(prompt);
 inputmaxw=xw;
 x2=x1+promptw+inputmaxw;
 disabled=0;
}

GUIrect *GUIedit::hittest(int x,int y) {return GUIrect::hittest(x,y);}

GUIrect *GUIedit::click(mouse &mm)
{
 if (!disabled) setfocus(this);
 return 0;
}

void GUIedit::draw(char *dest)
{
 font[FNT_EDITPROMPT]->draw(prompt,dest,x1,y1+1);
 int ix=x1+promptw;
 drawrect(dest,CLR_EDITINPUT,ix,y1,inputmaxw,10);
 if (focus) drawbox(dest,CLR_BUTTONINNER,ix,y1,ix+inputmaxw,y1+10);
 drawdata(dest,ix+2,y1+1,inputmaxw-4);
 GUIrect::draw(dest);
}

//text edit
GUItextedit::GUItextedit(GUIrect *p,char *pmpt,char *inp,int x,int y,int xw,int tmaxinputlen)
 :GUIedit(p,pmpt,x,y,xw)
{
 maxinputlen=tmaxinputlen;
 if (maxinputlen>127) maxinputlen=127;
 input[0]=0;
 setinput(inp);
}

void GUItextedit::getinputw()
{
 inputlen=strlen(input);
 inputw=font[FNT_EDITINPUT]->getwidth(input);
}

void GUItextedit::setinput(char *inp)
{
 strncpy(input,inp ? inp : "",maxinputlen); input[maxinputlen]=0;
 getinputw();
}

void GUItextedit::backspace()
{
 if (inputlen>0) input[--inputlen]=0;
 getinputw();
}

void GUItextedit::addchar(char c)
{
 if (inputlen>=maxinputlen) return;
 input[inputlen++]=c; input[inputlen]=0;
 getinputw();
}

char *getclipboardtext();
void GUItextedit::paste()
{
 char *s=getclipboardtext();
 if (s) for (; *s; s++) if (isvalidkey(*s)) addchar(*s);
}

int GUItextedit::isvalidkey(char key) {return (unsigned char)key>=' ' && (unsigned char)key<127;}

int GUItextedit::keyhit(char kbscan,char key)
{
 if (disabled) return 0;
 if (kbscan==KB_BKSP) {backspace(); if (parent) parent->sendmessage(this,GUIMSG_EDITCHANGED); return 1;}
 if (kbscan==KB_ENTER) {if (parent) parent->sendmessage(this,GUIMSG_OK); return 1;}
 if (key && isvalidkey(key))
  {
   addchar(key);
   if (parent) parent->sendmessage(this,GUIMSG_EDITCHANGED);
   return 1;
  }
 return 0;
}

void GUItextedit::drawdata(char *dest,int x,int y,int xw)
{
 char *s=input;
 //show the end of it if it's too wide
 while (*s && font[FNT_EDITINPUT]->getwidth(s)>xw-4) s++;
 font[FNT_EDITINPUT]->draw(s,dest,x,y);
 if (focus && (uu&16)) drawvline(dest,15,x+font[FNT_EDITINPUT]->getwidth(s)+1,y,y+8);
}

GUInumbertextedit::GUInumbertextedit(GUIrect *p,char *pmpt,int inp,int x,int y,int xw,int tmaxinputlen)
 :GUItextedit(p,pmpt,"",x,y,xw,tmaxinputlen)
{
 char s[32];
 sprintf(s,"%d",inp);
 setinput(s);
}

int GUInumbertextedit::isvalidkey(char key) {return (key>='0' && key<='9') || key=='-';}
int GUInumbertextedit::getstate() {return atoi(input);}

//number edit with up/down buttons
GUInumberedit::GUInumberedit(GUIrect *p,char *pmpt,int x,int y,int xw)
 :GUIedit(p,pmpt,x,y,xw)
{
 x2+=9;
 up=new GUIimagebutton(this,guivol.umark,promptw+inputmaxw+1,0);
 down=new GUIimagebutton(this,guivol.dmark,promptw+inputmaxw+1,5);
 up->resize(8,5);
 down->resize(8,5);
}

int GUInumberedit::keyhit(char kbscan,char key)
{
 if (disabled) return 0;
 switch (kbscan)
  {
   case KB_UP:   sendmessage(up,GUIMSG_HELDDOWN); return 1;
   case KB_DOWN: sendmessage(down,GUIMSG_HELDDOWN); return 1;
   case KB_HOME: clear(); return 1;
   case KB_END:  setmax(); return 1;
  }
 if (key=='+') {sendmessage(up,GUIMSG_HELDDOWN); return 1;}
 if (key=='-') {sendmessage(down,GUIMSG_HELDDOWN); return 1;}
 return 0;
}

void GUInumberedit::draw(char *dest) {GUIedit::draw(dest);}

//integer edit
GUIintedit::GUIintedit(GUIrect *p,char *pmpt,int x,int y,int xw,int num,int tmin,int tmax)
 :GUInumberedit(p,pmpt,x,y,xw)
{
 min=tmin; max=tmax; n=num;
 clip();
}

void GUIintedit::clip() {if (n<min) n=min; if (n>max) n=max;}
void GUIintedit::clear() {set(min);}
void GUIintedit::setmax() {set(max);}
void GUIintedit::add(int num) {set(n+num);}
void GUIintedit::minus(int num) {set(n-num);}

void GUIintedit::drawdata(char *dest,int x,int y,int xw)
{
 font[FNT_EDITINPUT]->printfile(x,y,"%d",n);
}

int GUIintedit::sendmessage(GUIrect *c,int msg)
{
 if (msg!=GUIMSG_HELDDOWN) return 1;
 int step=(kbstat&KB_SHIFT) ? 10 : 1;
 if (c==up) add(step);
 if (c==down) minus(step);
 return 1;
}

//float edit
GUIfloatedit::GUIfloatedit(GUIrect *p,char *pmpt,int x,int y,int xw,float num,float tmin,float tmax)
 :GUInumberedit(p,pmpt,x,y,xw)
{
 min=tmin; max=tmax; n=num;
 clip();
}

void GUIfloatedit::clip() {if (n<min) n=min; if (n>max) n=max;}
void GUIfloatedit::clear() {set(min);}
void GUIfloatedit::setmax() {set(max);}
void GUIfloatedit::add(float num) {set(n+num);}
void GUIfloatedit::minus(float num) {set(n-num);}

void GUIfloatedit::drawdata(char *dest,int x,int y,int xw)
{
 font[FNT_EDITINPUT]->printfile(x,y,"%.2f",n);
}

int GUIfloatedit::sendmessage(GUIrect *c,int msg)
{
 if (msg!=GUIMSG_HELDDOWN) return 1;
 float step=(kbstat&KB_SHIFT) ? 1.0f : 0.1f;
 if (c==up) add(step);
 if (c==down) minus(step);
 return 1;
}

//----------------------------------------------------------------
//dialog box: title bar, a 1 pixel frame and the contents

static int dragging=0;

GUIbox::GUIbox(GUIrect *p,char *titlestr,GUIcontents *c,int x,int y)
 :GUIrect(p,x,y,x+(c ? c->width() : 100)+2,y+(c ? c->height() : 50)+TITLEH+1)
{
 title[0]=0;
 settitle(titlestr);
 contents=c;
 if (contents)
  {
   contents->reparent(this);
   contents->moveto(x1+1,y1+TITLEH);
  }
 close=new GUIimagebutton(this,guivol.xmark,0,0);
 close->resize(10,9);
 reposclosebutton();
 //a new window gets the focus
 setfocus(this);
}

void GUIbox::settitle(char *s)
{
 strncpy(title,s ? s : "",79); title[79]=0;
}

void GUIbox::reposclosebutton()
{
 if (close) close->moveto(x2-11,y1+1);
}

GUIrect *GUIbox::click(mouse &mm)
{
 //drag by the title bar
 if (mm.y<y1+TITLEH && mm.y>=y1)
  {
   dragging=1;
   return this;
  }
 return 0;
}

int GUIbox::drag(mouse &mm)
{
 if (!dragging) return 0;
 int dx=mm.x-mm.oldx,dy=mm.y-mm.oldy;
 //keep the title bar on screen
 if (y1+dy<0) dy=-y1;
 if (y1+dy>SCREENY-TITLEH) dy=SCREENY-TITLEH-y1;
 if (x2+dx<20) dx=20-x2;
 if (x1+dx>SCREENX-20) dx=SCREENX-20-x1;
 moverel(dx,dy);
 return 1;
}

int GUIbox::release(mouse &mm)
{
 dragging=0;
 return 1;
}

void GUIbox::draw(char *dest)
{
 //frame and title bar
 outline(focus ? CLR_BOXTITLE-4 : CLR_BOXTITLE);
 drawrect(dest,focus ? CLR_BOXTITLE-4 : CLR_BOXTITLE,x1,y1,width(),TITLEH);
 drawrect(dest,CLR_BOX,x1+1,y1+TITLEH,width()-2,height()-TITLEH-1);
 font[focus ? FNT_BOXTITLE : 3]->draw(title,dest,x1+3,y1+2);
 GUIrect::draw(dest);
}

int GUIbox::keyhit(char kbscan,char key)
{
 if (lastfocus && lastfocus->focus) return lastfocus->keyhit(kbscan,key);
 if (contents) return contents->keyhit(kbscan,key);
 return 0;
}

int GUIbox::sendmessage(GUIrect *c,int msg)
{
 if (c==close && close && msg==GUIMSG_PUSHED)
  {
   if (contents) contents->sendmessage(this,GUIMSG_CLOSE);
   delete this;
   return 0;
  }
 return 1;
}

//----------------------------------------------------------------
//box with a maximize button (the NES screen)

GUImaximizebox::GUImaximizebox(GUIrect *p,char *titlestr,GUIcontents *c,int x,int y)
 :GUIbox(p,titlestr,c,x,y)
{
 maximized=0;
 max=new GUIimagebutton(this,guivol.umark,0,0);
 max->resize(10,9);
 reposmaxbutton();
}

void GUImaximizebox::reposmaxbutton()
{
 if (!max) return;
 if (maximized)
  {
   //top right corner of the screen, under the menu bar
   extern GUImenu *gmenu;
   int y=(gmenu && gmenu->y2>y1) ? gmenu->y2+1 : y1+1;
   close->moveto(x2-11,y);
   max->moveto(x2-22,y);
  }
  else
  {
   reposclosebutton();
   max->moveto(x2-22,y1+1);
  }
}

static int restorex,restorey;

void GUImaximizebox::maximize()
{
 if (!maximized)
  {
   restorex=x1; restorey=y1;
  }
 maximized=1;
 moveto(0,0);
 x2=SCREENX; y2=SCREENY;
 if (contents) contents->maximize();
 reposmaxbutton();
}

void GUImaximizebox::restore()
{
 if (!maximized) return;
 maximized=0;
 if (contents) contents->restore();
 //wrap ourself around the contents again
 int cw=contents ? contents->width() : 100;
 int ch=contents ? contents->height() : 50;
 x1=restorex; y1=restorey;
 if (x1+cw+2>SCREENX) x1=SCREENX-cw-2;
 if (y1+ch+TITLEH+1>SCREENY) y1=SCREENY-ch-TITLEH-1;
 if (x1<0) x1=0;
 if (y1<0) y1=0;
 x2=x1+cw+2; y2=y1+ch+TITLEH+1;
 if (contents) contents->moveto(x1+1,y1+TITLEH);
 reposmaxbutton();
}

GUIrect *GUImaximizebox::hittest(int x,int y)
{
 return GUIbox::hittest(x,y);
}

void GUImaximizebox::draw(char *dest)
{
 if (maximized)
  {
   //the contents fill the screen; buttons only with the GUI showing
   if (contents) contents->draw(dest);
   extern int guienabled;
   if (guienabled) {max->draw(dest); close->draw(dest);}
   return;
  }
 GUIbox::draw(dest);
}

int GUImaximizebox::sendmessage(GUIrect *c,int msg)
{
 if (c==max && max && msg==GUIMSG_PUSHED)
  {
   if (maximized) restore(); else maximize();
   return 1;
  }
 return GUIbox::sendmessage(c,msg);
}

//----------------------------------------------------------------
//boxes with buttons along the bottom

GUIonebuttonbox::GUIonebuttonbox(GUIrect *p,char *str,GUIcontents *c,char *b1name,int x,int y)
 :GUIbox(p,str,c,x,y)
{
 y2+=BARH;
 bar=new GUIbar(this,CLR_BOX,1,height()-BARH-1,width()-2,BARH);
 b1=new GUItextbutton(this,b1name,width()/2,height()-BARH/2-1);
}

int GUIonebuttonbox::sendmessage(GUIrect *c,int msg)
{
 if (c==b1 && msg==GUIMSG_PUSHED)
  {
   if (!contents || contents->sendmessage(this,GUIMSG_B1)) delete this;
   return 0;
  }
 return GUIbox::sendmessage(c,msg);
}

int GUIonebuttonbox::keyhit(char kbscan,char key)
{
 if (GUIbox::keyhit(kbscan,key)) return 1;
 if (kbscan==KB_ENTER) return sendmessage(b1,GUIMSG_PUSHED),1;
 return 0;
}

GUItwobuttonbox::GUItwobuttonbox(GUIrect *p,char *str,GUIcontents *c,char *b1name,char *b2name,int x,int y)
 :GUIbox(p,str,c,x,y)
{
 y2+=BARH;
 bar=new GUIbar(this,CLR_BOX,1,height()-BARH-1,width()-2,BARH);
 b1=new GUItextbutton(this,b1name,width()/3,height()-BARH/2-1);
 b2=new GUItextbutton(this,b2name,width()*2/3,height()-BARH/2-1);
}

int GUItwobuttonbox::sendmessage(GUIrect *c,int msg)
{
 if ((c==b1 || c==b2) && msg==GUIMSG_PUSHED)
  {
   if (!contents || contents->sendmessage(this,c==b1 ? GUIMSG_B1 : GUIMSG_B2)) delete this;
   return 0;
  }
 return GUIbox::sendmessage(c,msg);
}

int GUItwobuttonbox::keyhit(char kbscan,char key)
{
 if (GUIbox::keyhit(kbscan,key)) return 1;
 if (kbscan==KB_ENTER) return sendmessage(b1,GUIMSG_PUSHED),1;
 if (kbscan==KB_ESC) return sendmessage(b2,GUIMSG_PUSHED),1;
 return 0;
}

//----------------------------------------------------------------
//scroll bars
//a scroll bar holds pos in [min,max]; the track's thumb shows it

GUItrackbutton::GUItrackbutton(GUItrack *p,int size)
 :GUIbutton(p,0,0,size,size)
{
 ptrack=p;
 tpos=0;
}

GUIvtrackbutton::GUIvtrackbutton(GUIvtrack *p,int size):GUItrackbutton(p,size) {}
GUIhtrackbutton::GUIhtrackbutton(GUIhtrack *p,int size):GUItrackbutton(p,size) {}

GUIrect *GUIvtrackbutton::click(mouse &mm)
{
 depressed=1;
 tpos=mm.y-y1;
 return this;
}

int GUIvtrackbutton::drag(mouse &mm)
{
 ptrack->movethumb(mm.y-tpos-ptrack->y1);
 return 1;
}

GUIrect *GUIhtrackbutton::click(mouse &mm)
{
 depressed=1;
 tpos=mm.x-x1;
 return this;
}

int GUIhtrackbutton::drag(mouse &mm)
{
 ptrack->movethumb(mm.x-tpos-ptrack->x1);
 return 1;
}

GUItrack::GUItrack(GUIscrollbar *p,int x,int y,int xw,int yw)
 :GUIrect(p,x,y,x+xw,y+yw)
{
 pscroll=p;
 thumb=0;
 tracklen=0;
 trackpos=0;
}

//thumb moved to tpos (pixels from the start of the track)
void GUItrack::movethumb(int tpos)
{
 if (tpos>tracklen) tpos=tracklen;
 if (tpos<0) tpos=0;
 trackpos=tpos;
 positionthumb();
 pscroll->setposfromtrack(trackpos,tracklen);
}

void GUItrack::settrackfrompos(int pos,int min,int max)
{
 if (max<=min) trackpos=0;
  else trackpos=(pos-min)*tracklen/(max-min);
 positionthumb();
}

void GUItrack::setthumb() {positionthumb();}

void GUItrack::draw(char *dest)
{
 fill(CLR_BUTTONOUTERDOWN-8);
 GUIrect::draw(dest);
}

GUIvtrack::GUIvtrack(GUIvscrollbar *p)
 :GUItrack(p,0,SBW,SBW,p->height()-SBW*2)
{
 thumb=new GUIvtrackbutton(this,SBW);
 setthumbsize(0);
}

void GUIvtrack::setthumbsize(int posrange)
{
 int len=height()/(posrange+1);
 if (len<6) len=6;
 if (len>height()) len=height();
 thumb->resize(SBW,len);
 tracklen=height()-len;
 positionthumb();
}

void GUIvtrack::positionthumb() {thumb->moveto(x1,y1+trackpos);}

GUIhtrack::GUIhtrack(GUIhscrollbar *p)
 :GUItrack(p,SBW,0,p->width()-SBW*2,SBW)
{
 thumb=new GUIhtrackbutton(this,SBW);
 setthumbsize(0);
}

void GUIhtrack::setthumbsize(int posrange)
{
 int len=width()/(posrange+1);
 if (len<6) len=6;
 if (len>width()) len=width();
 thumb->resize(len,SBW);
 tracklen=width()-len;
 positionthumb();
}

void GUIhtrack::positionthumb() {thumb->moveto(x1+trackpos,y1);}

GUIscrollbar::GUIscrollbar(GUIrect *p,int x,int y,int xw,int yw)
 :GUIrect(p,x,y,x+xw,y+yw)
{
 pos=min=max=0;
 up=down=0;
 track=0;
}

void GUIscrollbar::clip()
{
 if (pos>max) pos=max;
 if (pos<min) pos=min;
}

void GUIscrollbar::setrange(int tmin,int tmax)
{
 min=tmin; max=tmax;
 if (max<min) max=min;
 clip();
 if (track)
  {
   track->setthumbsize(max-min);
   track->settrackfrompos(pos,min,max);
  }
}

void GUIscrollbar::setpos(int p)
{
 pos=p;
 clip();
 if (track) track->settrackfrompos(pos,min,max);
}

void GUIscrollbar::setposfromtrack(int tpos,int tlen)
{
 if (tlen<=0) pos=min;
  else pos=min+(tpos*(max-min)+tlen/2)/tlen;
 clip();
}

void GUIscrollbar::draw(char *dest) {GUIrect::draw(dest);}

int GUIscrollbar::sendmessage(GUIrect *c,int msg)
{
 if (msg!=GUIMSG_HELDDOWN) return 1;
 if (c==up) scrollup();
 if (c==down) scrolldown();
 return 1;
}

GUIvscrollbar::GUIvscrollbar(GUIrect *p,int x,int y,int height)
 :GUIscrollbar(p,x,y,SBW,height)
{
 up=new GUIimagebutton(this,guivol.umark,0,0);
 up->resize(SBW,SBW);
 down=new GUIimagebutton(this,guivol.dmark,0,height-SBW);
 down->resize(SBW,SBW);
 track=new GUIvtrack(this);
}

int GUIvscrollbar::keyhit(char kbscan,char key)
{
 if (kbscan==KB_UP) {scrollup(); return 1;}
 if (kbscan==KB_DOWN) {scrolldown(); return 1;}
 return 0;
}

GUIhscrollbar::GUIhscrollbar(GUIrect *p,int x,int y,int width)
 :GUIscrollbar(p,x,y,width,SBW)
{
 up=new GUIimagebutton(this,guivol.lmark,0,0);
 up->resize(SBW,SBW);
 down=new GUIimagebutton(this,guivol.rmark,width-SBW,0);
 down->resize(SBW,SBW);
 track=new GUIhtrack(this);
}

int GUIhscrollbar::keyhit(char kbscan,char key)
{
 if (kbscan==KB_LEFT) {scrollup(); return 1;}
 if (kbscan==KB_RIGHT) {scrolldown(); return 1;}
 return 0;
}

//----------------------------------------------------------------
//list box: iy items of iheight pixels, with a scroll bar

GUIlistbox::GUIlistbox(GUIrect *p,int x,int y,int xw,int iy,int iheight)
 :GUIrect(p,x,y,x+xw,y+iy*iheight+2)
{
 numitems=0;
 items=0;
 sel=-1;
 itemheight=iheight;
 itemv=iy;
 depressed=0;
 scroll=new GUIvscrollbar(this,xw-SBW,0,height());
 scroll->setrange(0,0);
}

void GUIlistbox::freeitems()
{
 if (items) free(items);
 items=0;
 numitems=0;
 sel=-1;
 if (scroll) scroll->setrange(0,0);
}

ITEMPTR *GUIlistbox::resizeitems(int n)
{
 items=(ITEMPTR *)realloc(items,(n>0 ? n : 1)*sizeof(ITEMPTR));
 for (int i=numitems; i<n; i++) items[i]=0;
 numitems=n;
 if (sel>=numitems) sel=numitems-1;
 int r=numitems-itemv;
 scroll->setrange(0,r>0 ? r : 0);
 return items;
}

void GUIlistbox::clearsel() {sel=-1;}

void GUIlistbox::setsel(int s)
{
 if (s>=numitems) s=numitems-1;
 if (s<0) s=numitems ? 0 : -1;
 //keep it visible
 if (s>=0)
  {
   int first=scroll->getpos();
   if (s<first) scroll->setpos(s);
   if (s>=first+itemv) scroll->setpos(s-itemv+1);
  }
 if (s==sel) return;
 sel=s;
 if (parent) parent->sendmessage(this,GUIMSG_LISTBOXSELCHANGED);
}

//which item is at screen y
static int listitemat(int y,int y1,int first,int itemheight) {return first+(y-y1-1)/itemheight;}

GUIrect *GUIlistbox::click(mouse &mm)
{
 setfocus(this);
 if (mm.x>=x2-SBW) return 0;
 int i=listitemat(mm.y,y1,scroll->getpos(),itemheight);
 if (i<0 || i>=numitems) return 0;
 if (i==sel && dblclick.dur && !dblclick.check())
  {
   dblclick.clear();
   //(parent may delete us)
   parent->sendmessage(this,GUIMSG_LISTBOXDBLCLICKED);
   return 0;
  }
 setsel(i);
 dblclick.set(20);
 depressed=1;
 return this;
}

int GUIlistbox::drag(mouse &mm)
{
 if (!depressed) return 0;
 int first=scroll->getpos();
 if (mm.y<y1) {setsel(first-1); return 1;}
 if (mm.y>=y2) {setsel(first+itemv); return 1;}
 int i=listitemat(mm.y,y1,first,itemheight);
 if (i>=0 && i<numitems) setsel(i);
 return 1;
}

int GUIlistbox::release(mouse &mm)
{
 depressed=0;
 return 1;
}

int GUIlistbox::keyhit(char kbscan,char key)
{
 switch (kbscan)
  {
   case KB_UP:   setsel(sel-1); return 1;
   case KB_DOWN: setsel(sel+1); return 1;
   case KB_PGUP: setsel(sel-itemv); return 1;
   case KB_PGDN: setsel(sel+itemv); return 1;
   case KB_HOME: setsel(0); return 1;
   case KB_END:  setsel(numitems-1); return 1;
   case KB_ENTER:
    if (sel>=0 && parent) parent->sendmessage(this,GUIMSG_LISTBOXDBLCLICKED);
    return 1;
  }
 //jump to the next item starting with that letter
 if (key>' ' && numitems)
  {
   for (int k=1; k<=numitems; k++)
    {
     int i=(sel+k)%numitems;
     char *s=(char *)items[i];
     if (s && toupper((unsigned char)s[0])==toupper((unsigned char)key)) {setsel(i); return 1;}
    }
  }
 return 0;
}

void GUIlistbox::draw(char *dest)
{
 fill(CLR_EDITINPUT);
 outline(focus ? CLR_BUTTONINNER : CLR_BUTTONOUTER);
 {
  char *d=dest;
  CLIP clip(d,x1+1,y1+1,x2-SBW,y2-1);
  drawitems(d,0,0);
 }
 GUIrect::draw(dest);
}

//dest has been clipped to the item area (see CLIP): draw at 0,0
void GUIstringlistbox::drawitems(char *dest,int x,int y)
{
 int first=scroll->getpos();
 for (int i=0; i<itemv && first+i<numitems; i++)
  {
   int idx=first+i;
   if (idx==sel) drawrect(dest,focus ? CLR_MENUSEL : CLR_BUTTONINNERDOWN,x,y+i*itemheight,SCREENX,itemheight);
   char *s=(char *)items[idx];
   if (s) font[idx==sel ? FNT_MENUSEL : FNT_EDITINPUT]->draw(s,dest,x+2,y+i*itemheight+(itemheight-8)/2);
  }
}
