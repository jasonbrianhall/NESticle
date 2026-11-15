//Minimal dirent (opendir/readdir/closedir) for Microsoft C, which has none.
//(on the C library's _findfirst: no windows.h, whose names clash with ours)
#ifndef _NESTICLE_DIRENT_WIN_H_
#define _NESTICLE_DIRENT_WIN_H_

#include <io.h>
#include <stdlib.h>
#include <string.h>

struct dirent {char d_name[260];};

typedef struct
{
 intptr_t h;
 struct _finddata_t fd;
 int first;
 struct dirent e;
} DIR;

static inline DIR *opendir(const char *path)
{
 char pat[300];
 size_t n=strlen(path);
 if (n+3>=sizeof(pat)) return 0;
 strcpy(pat,path);
 if (n && pat[n-1]!='/' && pat[n-1]!='\\') strcat(pat,"\\");
 strcat(pat,"*");
 DIR *d=(DIR *)malloc(sizeof(DIR));
 if (!d) return 0;
 d->h=_findfirst(pat,&d->fd);
 if (d->h==-1) {free(d); return 0;}
 d->first=1;
 return d;
}

static inline struct dirent *readdir(DIR *d)
{
 if (!d->first && _findnext(d->h,&d->fd)) return 0;
 d->first=0;
 strncpy(d->e.d_name,d->fd.name,259);
 d->e.d_name[259]=0;
 return &d->e;
}

static inline int closedir(DIR *d)
{
 _findclose(d->h);
 free(d);
 return 0;
}

#endif
