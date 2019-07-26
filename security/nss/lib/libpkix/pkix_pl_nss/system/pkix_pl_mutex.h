









#ifndef _PKIX_PL_MUTEX_H
#define _PKIX_PL_MUTEX_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_MutexStruct {
        PRLock* lock;
};



PKIX_Error *
pkix_pl_Mutex_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
