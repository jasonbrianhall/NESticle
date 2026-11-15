//Menus declared in guimenu.h (see guirect.cpp for why this file exists).
//
//A menu bar (GUIhmenu) opens drop-down menus (GUIvmenu) as its children;
//they can open their own submenus the same way. Click an item, or press
//and drag to it and let go. Shortcut keys: an item's key is either a scan
//code with bit 7 set (F1..F12, plain) or a letter (with Alt held).

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../types.h"
#include "../r2img.h"
#include "../font.h"
#include "../dd.h"
#include "../mouse.h"
#include "../gui.h"
#include "../guimenu.h"
#include "../guicolor.h"
#include "../keyb.h"

#define ITEMH 11 //height of a menu item
#define SEPH  5  //height of a separator ("-----")

static int isseparator(menuitem *i) {return i->text && i->text[0]=='-';}

//text for an item's shortcut key
static void keylabel(char k,char *s)
{
 s[0]=0;
 if (!k) return;
 if (k&0x80) strcpy(s,getkeyname(k&0x7F));
  else sprintf(s,"Alt+%c",toupper((unsigned char)k));
}

//----------------------------------------------------------------
//menuitem

int menuitem::width()
{
 if (isseparator(this)) return 8;
 int w=font[FNT_MENU]->getwidth(text)+10;
 char s[32];
 keylabel(key,s);
 if (s[0]) w+=font[FNT_MENUKEY]->getwidth(s)+12;
 if (submenu) w+=8;
 return w;
}

int menuitem::height() {return isseparator(this) ? SEPH : ITEMH;}

int menuitem::menufunc()
{
 if (!func) return 0;
 func();
 return 1;
}

//draw at x,y, xw wide (for vertical menus: the key goes on the right)
void menuitem::draw(int x,int y,int xw,int sel)
{
 if (isseparator(this))
  {
   drawhline(screen,CLR_BUTTONOUTER,x+2,y+2,x+xw-2);
   return;
  }
 if (sel) drawrect(screen,CLR_MENUSEL,x,y,xw,ITEMH);
 font[sel ? FNT_MENUSEL : FNT_MENU]->draw(text,screen,x+5,y+2);
 char s[32];
 keylabel(key,s);
 if (s[0]) font[FNT_MENUKEY]->draw(s,screen,x+xw-font[FNT_MENUKEY]->getwidth(s)-(submenu ? 12 : 4),y+2);
 if (submenu) font[sel ? FNT_MENUSEL : FNT_MENU]->draw(">",screen,x+xw-9,y+2);
}

//----------------------------------------------------------------
//menu

int menu::getnumitems()
{
 int n=0;
 while (n<32 && m[n].text) n++;
 return n;
}

int menu::getmaxwidth()
{
 int w=0;
 for (int i=0; i<getnumitems(); i++) if (m[i].width()>w) w=m[i].width();
 return w;
}

int menu::gettotalwidth()
{
 int w=0;
 for (int i=0; i<getnumitems(); i++) w+=m[i].width();
 return w;
}

int menu::getmaxheight()
{
 int h=0;
 for (int i=0; i<getnumitems(); i++) if (m[i].height()>h) h=m[i].height();
 return h;
}

int menu::gettotalheight()
{
 int h=0;
 for (int i=0; i<getnumitems(); i++) h+=m[i].height();
 return h;
}

//shortcut keys, searched through the submenus too
int menu::keyhit(char kbscan,char key)
{
 int n=getnumitems();
 for (int i=0; i<n; i++)
  {
   char k=m[i].key;
   if (k && m[i].func)
    {
     int hit;
     if (k&0x80) hit=((k&0x7F)==(kbscan&0x7F)) && !(kbscan&0x80);
      else hit=(kbstat&KB_ALT) && key && tolower((unsigned char)key)==tolower((unsigned char)k);
     if (hit) {m[i].menufunc(); return 1;}
    }
   if (m[i].submenu && m[i].submenu->keyhit(kbscan,key)) return 1;
  }
 return 0;
}

//----------------------------------------------------------------
//GUImenu

static GUImenu *parentmenu(GUIrect *g) {return g ? dynamic_cast<GUImenu *>(g) : 0;}

//the menu bar (top of the tree of open menus)
static GUImenu *topmenu(GUImenu *t)
{
 while (parentmenu(t->parent)) t=parentmenu(t->parent);
 return t;
}

//the open submenu of a menu (its child menu), if any
static GUImenu *opensub(GUIrect *t)
{
 for (GUIrect *g=t->child; g; g=g->next)
  if (parentmenu(g)) return parentmenu(g);
 return 0;
}

static void closesubs(GUIrect *t)
{
 GUImenu *s;
 while ((s=opensub(t))!=0) delete s;
}

GUImenu::GUImenu(GUIrect *p,menu *tm,int x,int y)
 :GUIrect(p,x,y,x,y)
{
 tmenu=tm;
 selmi=0;
}

void GUImenu::losefocus()
{
 GUIrect::losefocus();
 closesubs(this);
 selmi=0;
}

//our open submenu doesn't close because something inside us got the focus
void GUImenu::losechildfocus()
{
 for (GUIrect *g=child; g; g=g->next)
  if (g->focus) {g->focus=0; g->losechildfocus();}
}

//open item i's submenu at sx,sy (screen coords)
static void opensubmenu(GUImenu *t,menuitem *i,int sx,int sy)
{
 closesubs(t);
 GUIvmenu *v=new GUIvmenu(t,i->submenu,sx-t->x1,sy-t->y1);
 //keep it on screen
 int dx=0,dy=0;
 if (v->x2>SCREENX) dx=SCREENX-v->x2;
 if (v->y2>SCREENY) dy=SCREENY-v->y2;
 if (v->x1+dx<0) dx=-v->x1;
 if (v->y1+dy<0) dy=-v->y1;
 v->moverel(dx,dy);
}

GUIrect *GUImenu::click(mouse &mm)
{
 int sx,sy;
 menuitem *i=menuhittest(mm.x,mm.y,sx,sy);
 if (!i || isseparator(i)) return this;

 if (i->submenu)
  {
   if (selmi==i && opensub(this)) {closesubs(this); selmi=0; return this;}
   selmi=i;
   opensubmenu(this,i,sx,sy);
   return this;
  }

 domenuitem(i); //(may delete us)
 return 0;
}

//drag: follow the mouse through the open menus
int GUImenu::drag(mouse &mm)
{
 for (GUImenu *t=topmenu(this); t; t=opensub(t))
  {
   if (mm.x<t->x1 || mm.x>=t->x2 || mm.y<t->y1 || mm.y>=t->y2) continue;
   int sx,sy;
   menuitem *i=t->menuhittest(mm.x,mm.y,sx,sy);
   if (i && i->submenu && i!=t->selmi)
    {
     t->selmi=i;
     opensubmenu(t,i,sx,sy);
    }
   return 1;
  }
 return 1;
}

//release: let go over an item of an open menu runs it
int GUImenu::release(mouse &mm)
{
 GUImenu *hit=0;
 for (GUImenu *t=topmenu(this); t; t=opensub(t))
  if (mm.x>=t->x1 && mm.x<t->x2 && mm.y>=t->y1 && mm.y<t->y2) hit=t;
 if (!hit || hit==topmenu(this)) return 1;
 int sx,sy;
 menuitem *i=hit->menuhittest(mm.x,mm.y,sx,sy);
 if (i && !i->submenu && !isseparator(i) && i->func && hit!=this)
   hit->domenuitem(i); //(may delete us)
 return 1;
}

//run a menu item: close all the menus first (the item may open a dialog)
int GUImenu::domenuitem(menuitem *i)
{
 MENUFUNCPTR f=i->func;
 GUImenu *top=topmenu(this);
 closesubs(top); //(may delete us)
 top->selmi=0;
 if (f) f();
 return 1;
}

//----------------------------------------------------------------
//drop-down menu

GUIvmenu::GUIvmenu(GUImenu *p,menu *tm,int x,int y)
 :GUImenu(p,tm,x,y)
{
 x2=x1+getmenuwidth();
 y2=y1+getmenuheight()+2;
}

menuitem *GUIvmenu::menuhittest(int x,int y,int &sx,int &sy)
{
 if (x<x1 || x>=x2 || y<y1 || y>=y2) return 0;
 int iy=y1+1;
 int n=tmenu->getnumitems();
 for (int i=0; i<n; i++)
  {
   int h=tmenu->m[i].height();
   if (y>=iy && y<iy+h)
    {
     sx=x2-2; sy=iy-1;
     return &tmenu->m[i];
    }
   iy+=h;
  }
 return 0;
}

void GUIvmenu::draw(char *dest)
{
 fill(CLR_MENU);
 outline(CLR_BUTTONOUTERDOWN);
 int sx,sy;
 menuitem *hover=menuhittest(m.x,m.y,sx,sy);
 int iy=y1+1;
 int n=tmenu->getnumitems();
 for (int i=0; i<n; i++)
  {
   menuitem *it=&tmenu->m[i];
   int sel=(it==hover || (it==selmi && opensub(this))) && !isseparator(it);
   it->draw(x1+1,iy,width()-2,sel);
   iy+=it->height();
  }
 GUIrect::draw(dest);
}

//----------------------------------------------------------------
//menu bar

//(items of the bar: just the text, no key or submenu arrow)
static int barwidth(menuitem *i) {return font[FNT_MENU]->getwidth(i->text)+12;}

GUIhmenu::GUIhmenu(GUIrect *p,menu *tm,int x,int y)
 :GUImenu(p,tm,x,y)
{
 int w=0;
 for (int i=0; i<tm->getnumitems(); i++) w+=barwidth(&tm->m[i]);
 x2=x1+w;
 y2=y1+getmenuheight()+1;
}

menuitem *GUIhmenu::menuhittest(int x,int y,int &sx,int &sy)
{
 if (y<y1 || y>=y2) return 0;
 int ix=x1;
 int n=tmenu->getnumitems();
 for (int i=0; i<n; i++)
  {
   int w=barwidth(&tmenu->m[i]);
   if (x>=ix && x<ix+w)
    {
     sx=ix; sy=y2;
     return &tmenu->m[i];
    }
   ix+=w;
  }
 return 0;
}

int GUIhmenu::keyhit(char kbscan,char key)
{
 if (GUIrect::keyhit(kbscan,key)) return 1;
 if (kbscan==KB_ESC && opensub(this)) {closesubs(this); selmi=0; return 1;}
 return tmenu->keyhit(kbscan,key);
}

void GUIhmenu::draw(char *dest)
{
 //the bar goes across the screen
 if (parent) x2=parent->x2;
 fill(CLR_MENU);
 drawhline(dest,CLR_BUTTONOUTERDOWN,x1,y2-1,x2);
 int sx,sy;
 menuitem *hover=menuhittest(m.x,m.y,sx,sy);
 int ix=x1;
 int n=tmenu->getnumitems();
 for (int i=0; i<n; i++)
  {
   menuitem *it=&tmenu->m[i];
   int w=barwidth(it);
   int sel=(it==selmi && opensub(this)) || it==hover;
   if (sel) drawrect(dest,CLR_MENUSEL,ix,y1,w,y2-y1-1);
   font[sel ? FNT_MENUSEL : FNT_MENU]->draw(it->text,dest,ix+5,y1+2);
   ix+=w;
  }
 GUIrect::draw(dest);
}

//----------------------------------------------------------------
//popup menu: reports the number of the chosen item to treport

GUIpopupmenu::GUIpopupmenu(GUIrect *treport,menu *tm,int x,int y)
 :GUIvmenu((GUImenu *)guiroot,tm,x,y)
{
 report=treport;
 setfocus(this);
}

GUIpopupmenu::~GUIpopupmenu() {}

int GUIpopupmenu::domenuitem(menuitem *t)
{
 GUIrect *r=report;
 int idx=t-tmenu->m;
 delete this;
 if (r) r->sendmessage(0,idx);
 return 1;
}

int GUIpopupmenu::release(mouse &mm)
{
 int sx,sy;
 menuitem *i=menuhittest(mm.x,mm.y,sx,sy);
 if (i && !isseparator(i)) return domenuitem(i);
 if (!i) delete this;
 return 1;
}

void GUIpopupmenu::draw(char *dest) {GUIvmenu::draw(dest);}
