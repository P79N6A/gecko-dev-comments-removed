



#ifndef NSSBASET_H
#define NSSBASET_H







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
