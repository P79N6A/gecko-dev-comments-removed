









#ifndef _PKIX_PL_MONITORLOCK_H
#define _PKIX_PL_MONITORLOCK_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_PL_MonitorLockStruct {
        PRMonitor* lock;
};



PKIX_Error *
pkix_pl_MonitorLock_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
