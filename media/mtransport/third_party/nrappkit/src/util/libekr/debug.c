


















































































static char *RCSSTRING __UNUSED__ ="$Id: debug.c,v 1.3 2007/06/26 22:37:57 adamcain Exp $";


#include <stdarg.h>
#include <stdio.h>
#include "r_common.h"
#include "debug.h"

int nr_debug(int class,char *format,...)
  {
    va_list ap;

    va_start(ap,format);
#ifdef WIN32
    vprintf(format,ap);
    printf("\n");
#else
    vfprintf(stderr,format,ap);
    fprintf(stderr,"\n");
#endif
    return(0);
  }

int nr_xdump(name,data,len)
  char *name;
  UCHAR *data;
  int len;
  {
    int i;

    if(name){
      printf("%s[%d]=\n",name,len);
    }
    for(i=0;i<len;i++){

      if((len>8) && i && !(i%12)){
        printf("\n");
      }
      printf("%.2x ",data[i]&255);
    }
    if(i%12)
      printf("\n");
    return(0);
  }




