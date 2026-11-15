//GUI base classes: GUIrect, GUIcontents, GUIroot, dialog placement.
//
//NESticle's GUI library (GUIRECT.CPP, GUI.CPP, GUIMENU.CPP) was not part of
//the leaked source; only its headers (guirect.h, gui.h, guimenu.h,
//guiroot.h) were. This is a new implementation of those headers, written
//from how the rest of the program uses them.
//
//Conventions (from the callers):
// - child coordinates given to constructors are relative to the parent;
//   x1,y1,x2,y2 are stored as absolute screen coordinates
// - children are a linked list, topmost first; drawn bottom to top
// - focus: a chain from the root down; lastfocus points to the child of
//   ours that has (or last had) the focus

#include <string.h>
#include <stdlib.h>

#include "../types.h"
#include "../r2img.h"
#include "../font.h"
#include "../dd.h"
#include "../mouse.h"
#include "../gui.h"
#include "../guimenu.h"
#include "../guicolor.h"
#include "../keyb.h"

extern char *screen;

//----------------------------------------------------------------
//placement of new dialog boxes (cascading)

int GUIdefx=16,GUIdefy=20;
void resetGUIdef() {GUIdefx=16; GUIdefy=20;}
void nextGUIdef()
{
 GUIdefx+=12; GUIdefy+=12;
 if (GUIdefx>SCREENX/2 || GUIdefy>SCREENY/2) resetGUIdef();
}

//----------------------------------------------------------------

GUIrect *GUIrect::modal=0;

void GUIrect::setmodal(GUIrect *m) {modal=m;}

GUIrect::GUIrect(GUIrect *p,int tx1,int ty1,int tx2,int ty2)
{
 parent=0;
 child=lastchild=0;
 prev=next=0;
 focus=0;
 lastfocus=0;
 x1=tx1; y1=ty1; x2=tx2; y2=ty2;
 if (p)
  {
   x1+=p->x1; x2+=p->x1;
   y1+=p->y1; y2+=p->y1;
   link(p);
  }
}

extern GUImenu *gmenu;

//link ourself into t's list of children (as the topmost child)
void GUIrect::link(GUIrect *t)
{
 parent=t;
 if (!t) return;
 prev=0;
 next=t->child;
 if (t->child) t->child->prev=this; else t->lastchild=this;
 t->child=this;
 //the main menu stays above every window
 if (gmenu && gmenu!=this && gmenu->parent==t)
  {
   GUIrect *lf=t->lastfocus;
   gmenu->unlink();
   gmenu->link(t);
   t->lastfocus=lf;
  }
}

//remove ourself from our parent's list of children
void GUIrect::unlink()
{
 if (!parent) return;
 if (prev) prev->next=next; else parent->child=next;
 if (next) next->prev=prev; else parent->lastchild=prev;
 if (parent->lastfocus==this) parent->lastfocus=0;
 prev=next=0;
 parent=0;
}

void GUIrect::reparent(GUIrect *p)
{
 unlink();
 link(p);
}

GUIrect::~GUIrect()
{
 //children first
 while (child) delete child;

 if (modal==this) modal=0;
 if (m.capture==this) m.capture=0;
 unlink();
}

//children are tested even outside our own rect: menus open their submenus
//(their children) outside of themselves
GUIrect *GUIrect::hittest(int x,int y)
{
 for (GUIrect *g=child; g; g=g->next)
  {
   GUIrect *r=g->hittest(x,y);
   if (r) return r;
  }
 if (x>=x1 && x<x2 && y>=y1 && y<y2) return this;
 return 0;
}

void GUIrect::bringtofront()
{
 if (!parent || parent->child==this) return;
 GUIrect *p=parent,*lf=parent->lastfocus;
 unlink();
 link(p);
 p->lastfocus=lf;
}

void GUIrect::sendtoback()
{
 if (!parent || parent->lastchild==this) return;
 GUIrect *p=parent,*lf=parent->lastfocus;
 unlink();
 p->lastfocus=lf;
 parent=p;
 prev=p->lastchild;
 next=0;
 if (p->lastchild) p->lastchild->next=this; else p->child=this;
 p->lastchild=this;
}

void GUIrect::moverel(int dx,int dy)
{
 x1+=dx; x2+=dx;
 y1+=dy; y2+=dy;
 for (GUIrect *g=child; g; g=g->next) g->moverel(dx,dy);
}

void GUIrect::moveto(int x,int y) {moverel(x-x1,y-y1);}

void GUIrect::fill(char color)
{
 drawrect(screen,(unsigned char)color,x1,y1,width(),height());
}

void GUIrect::outline(char color)
{
 int c=(unsigned char)color;
 drawhline(screen,c,x1,y1,x2);
 drawhline(screen,c,x1,y2-1,x2);
 drawvline(screen,c,x1,y1,y2);
 drawvline(screen,c,x2-1,y1,y2);
}

//draw children, bottom first
void GUIrect::draw(char *dest)
{
 for (GUIrect *g=lastchild; g; )
  {
   GUIrect *p=g->prev; //(in case drawing deletes it)
   g->draw(dest);
   g=p;
  }
}

//----------------------------------------------------------------
//focus

void GUIrect::receivefocus() {focus=1;}

void GUIrect::losefocus()
{
 focus=0;
 losechildfocus();
}

void GUIrect::losechildfocus()
{
 for (GUIrect *g=child; g; )
  {
   GUIrect *n=g->next;
   if (g->focus) g->losefocus();
   g=n;
  }
}

//give the focus to f (or the first of its ancestors that takes focus)
int GUIrect::setfocus(GUIrect *f)
{
 while (f && !f->acceptfocus()) f=f->parent;
 if (!f) return 0;

 //chain from the root down to f
 GUIrect *chain[64];
 int n=0;
 for (GUIrect *c=f; c && n<64; c=c->parent) chain[n++]=c;

 //everyone off the chain loses focus, top down
 for (int i=n-1; i>0; i--)
  {
   GUIrect *p=chain[i],*c=chain[i-1];
   for (GUIrect *s=p->child; s; )
    {
     GUIrect *nx=s->next;
     if (s!=c && s->focus) s->losefocus();
     s=nx;
    }
   p->lastfocus=c;
  }
 //f's children lose it too (f is the new focus)
 f->losechildfocus();

 //the chain receives it, top down
 for (int i=n-1; i>=0; i--)
  if (!chain[i]->focus) chain[i]->receivefocus();
 return 1;
}

//move the focus to the next (dir>=0) or previous child that takes focus
void GUIrect::cyclefocus(int dir)
{
 GUIrect *list[128];
 int n=0,cur=-1;
 //in on-screen order: bottom (first created) to top
 for (GUIrect *g=lastchild; g && n<128; g=g->prev)
  if (g->acceptfocus())
   {
    if (g==lastfocus && g->focus) cur=n;
    list[n++]=g;
   }
 if (!n) return;
 int i=(cur<0) ? 0 : (cur+(dir<0 ? n-1 : 1))%n;
 setfocus(list[i]);
}

//keys go down the focus chain
int GUIrect::keyhit(char kbscan,char key)
{
 if (lastfocus && lastfocus->focus) return lastfocus->keyhit(kbscan,key);
 return 0;
}

//----------------------------------------------------------------
//contents of a GUIbox

void GUIcontents::resize(int xw,int yw)
{
 GUIrect::resize(xw,yw);
}

int GUIcontents::keyhit(char kbscan,char key)
{
 if (GUIrect::keyhit(kbscan,key)) return 1;
 if (kbscan==KB_TAB) {cyclefocus((kbstat&KB_SHIFT) ? -1 : 1); return 1;}
 return 0;
}

//----------------------------------------------------------------
//root of the GUI elements (child of ROOT, see main.cpp)

GUIroot::GUIroot(ROOT *p):GUIrect(p,0,0,p->width(),p->height())
{
 guiroot=this;
}

GUIroot::~GUIroot()
{
 if (guiroot==this) guiroot=0;
}

int GUIroot::keyhit(char kbscan,char key)
{
 //the focused window first
 if (GUIrect::keyhit(kbscan,key)) return 1;
 //then the main menu's shortcut keys
 if (gmenu && !(lastfocus==gmenu && gmenu->focus)) return gmenu->keyhit(kbscan,key);
 return 0;
}
