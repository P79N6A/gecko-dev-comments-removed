



































#ifndef NSSBASE_H
#define NSSBASE_H

#ifdef DEBUG
static const char NSSBASE_CVS_ID[] = "@(#) $RCSfile: nssbase.h,v $ $Revision: 1.3 $ $Date: 2005/01/20 02:25:45 $";
#endif 








#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

PR_BEGIN_EXTERN_C
























NSS_EXTERN NSSArena *
NSSArena_Create
(
  void
);

extern const NSSError NSS_ERROR_NO_MEMORY;

















NSS_EXTERN PRStatus
NSSArena_Destroy
(
  NSSArena *arena
);

extern const NSSError NSS_ERROR_INVALID_ARENA;

























NSS_EXTERN NSSError
NSS_GetError
(
  void
);

extern const NSSError NSS_ERROR_NO_ERROR;



















NSS_EXTERN NSSError *
NSS_GetErrorStack
(
  void
);

PR_END_EXTERN_C

#endif 
