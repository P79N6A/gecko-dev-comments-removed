































#include <stdarg.h>


static char *RCSSTRING __UNUSED__="$Id: ice_util.c,v 1.2 2008/04/28 17:59:05 ekr Exp $";

#include <stdarg.h>
#include <string.h>
#include "nr_api.h"
#include "ice_util.h"

int nr_concat_strings(char **outp,...)
  {
    va_list ap;
    char *s,*out=0;
    int len=0;
    int _status;

    va_start(ap,outp);
    while(s=va_arg(ap,char *)){
      len+=strlen(s);
    }
    va_end(ap);


    if(!(out=RMALLOC(len+1)))
      ABORT(R_NO_MEMORY);

    *outp=out;

    va_start(ap,outp);
    while(s=va_arg(ap,char *)){
      len=strlen(s);
      memcpy(out,s,len);
      out+=len;
    }
    va_end(ap);

    *out=0;

    _status=0;
  abort:
    return(_status);
  }

