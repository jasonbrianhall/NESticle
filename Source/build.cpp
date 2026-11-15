//build info
char buildtime[]=__TIME__;
char builddate[]=__DATE__;

#ifdef __WATCOMC__
char buildcompiler[]="Watcom C++";
int buildcompilerversionhigh=__WATCOMC__/100;
int buildcompilerversionlow=__WATCOMC__%100;
#endif
#ifdef __BORLANDC__
char buildcompiler[]="Borland C++";
int buildcompilerversionhigh=__BORLANDC__/256;
int buildcompilerversionlow=__BORLANDC__%256;
#endif

#if defined(__GNUC__) && !defined(__WATCOMC__)
#ifdef __DJGPP__
char buildcompiler[]="DJGPP GCC";
#else
char buildcompiler[]="GCC";
#endif
int buildcompilerversionhigh=__GNUC__;
int buildcompilerversionlow=__GNUC_MINOR__;
#endif

#ifdef _MSC_VER
char buildcompiler[]="MS VC++";
int buildcompilerversionhigh=_MSC_VER/100;
int buildcompilerversionlow=_MSC_VER%100;
#endif
