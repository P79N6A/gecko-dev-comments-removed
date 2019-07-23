



































#ifndef NSSBASET_H
#define NSSBASET_H

#ifdef DEBUG
static const char NSSBASET_CVS_ID[] = "@(#) $RCSfile: nssbaset.h,v $ $Revision: 1.8 $ $Date: 2009/04/07 23:52:05 $";
#endif 







#include "nspr.h"
#include "nssilock.h"










#define DUMMY
#define NSS_EXTERN         extern
#define NSS_EXTERN_DATA    extern
#define NSS_IMPLEMENT      
#define NSS_IMPLEMENT_DATA 

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
