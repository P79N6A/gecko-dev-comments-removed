









#ifndef _PKIX_PL_RWLOCK_H
#define _PKIX_PL_RWLOCK_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_RWLockStruct {
        PRRWLock* lock;
        PKIX_UInt32 readCount;
        PKIX_Boolean writeLocked;
};



PKIX_Error *
pkix_pl_RWLock_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
