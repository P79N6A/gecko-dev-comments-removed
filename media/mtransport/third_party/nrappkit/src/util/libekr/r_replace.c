


















































































static char *RCSSTRING __UNUSED__ ="$Id: r_replace.c,v 1.2 2006/08/16 19:39:17 adamcain Exp $";

#include  "r_common.h"

#ifndef HAVE_STRDUP

char *strdup(str)
  char *str;
  {
    int len=strlen(str);
    char *n;

    if(!(n=(char *)malloc(len+1)))
      return(0);

    memcpy(n,str,len+1);

    return(n);
  }
#endif


#ifdef SUPPLY_ATEXIT
int atexit(void (*func)(void)){
  ;
}
#endif
