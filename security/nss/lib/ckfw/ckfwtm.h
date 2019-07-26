



#ifndef CKFWTM_H
#define CKFWTM_H

#ifdef DEBUG
static const char CKFWTM_CVS_ID[] = "@(#) $RCSfile: ckfwtm.h,v $ $Revision: 1.4 $ $Date: 2012/04/25 14:49:28 $";
#endif 







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

struct nssCKFWHashStr;
typedef struct nssCKFWHashStr nssCKFWHash;

typedef void (PR_CALLBACK *nssCKFWHashIterator)(const void *key, void *value, void *closure);

#endif 
