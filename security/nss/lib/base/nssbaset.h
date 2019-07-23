



































#ifndef NSSBASET_H
#define NSSBASET_H

#ifdef DEBUG
static const char NSSBASET_CVS_ID[] = "@(#) $RCSfile: nssbaset.h,v $ $Revision: 1.6 $ $Date: 2005/01/20 02:25:45 $";
#endif 







#include "nspr.h"
#include "nssilock.h"











#define DUMMY
#define NSS_EXTERN         PR_EXTERN(DUMMY)
#define NSS_IMPLEMENT      PR_IMPLEMENT(DUMMY)
#define NSS_EXTERN_DATA    PR_EXTERN_DATA(DUMMY)
#define NSS_IMPLEMENT_DATA PR_IMPLEMENT_DATA(DUMMY)

PR_BEGIN_EXTERN_C










typedef PRInt32 NSSError;












struct NSSArenaStr;
typedef struct NSSArenaStr NSSArena;








struct NSSItemStr {
  void *data;
  PRUint32 size;
};
typedef struct NSSItemStr NSSItem;








typedef NSSItem NSSBER;








typedef NSSBER NSSDER;










typedef NSSItem NSSBitString;







typedef char NSSUTF8;







typedef char NSSASCII7;

PR_END_EXTERN_C

#endif 
