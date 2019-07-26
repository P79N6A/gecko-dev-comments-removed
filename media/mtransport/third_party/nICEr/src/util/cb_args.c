

































static char *RCSSTRING __UNUSED__="$Id: cb_args.c,v 1.2 2008/04/28 17:59:04 ekr Exp $";

#include <stdarg.h>
#include "nr_api.h"
#include "cb_args.h"

void **nr_pack_cb_args(int ct,...)
  {
    void **vlist;
    va_list ap;
    int i;

    va_start(ap,ct);
    if(!(vlist=RCALLOC(sizeof(void *)*ct+1)))
      abort();

    for(i=0;i<ct;i++){
      vlist[i]=va_arg(ap, void *);
    }

    va_end(ap);

    return(vlist);
  }



