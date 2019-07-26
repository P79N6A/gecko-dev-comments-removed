



#ifndef CKFWTM_H
#define CKFWTM_H







#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

struct nssCKFWHashStr;
typedef struct nssCKFWHashStr nssCKFWHash;

typedef void (PR_CALLBACK *nssCKFWHashIterator)(const void *key, void *value, void *closure);

#endif 
