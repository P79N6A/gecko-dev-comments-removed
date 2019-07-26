



#ifndef _CPR_LOCKS_H_
#define _CPR_LOCKS_H_

#include "cpr_types.h"
#include "cpr_time.h"

__BEGIN_DECLS




typedef void* cprMutex_t;










typedef struct {
    const char*  name;
    uint16_t lockId;
    union {
      void* handlePtr;
      uint32_t handleInt;
    } u;
} cpr_mutex_t;















cprMutex_t
cprCreateMutex(const char * name);
















cprRC_t
cprDestroyMutex(cprMutex_t mutex);















cprRC_t
cprGetMutex(cprMutex_t mutex);












cprRC_t
cprReleaseMutex(cprMutex_t mutex);





typedef void* cprSignal_t;










typedef struct {
    const char *name;
    uint16_t lockId;
    union {
      void *handlePtr;
      uint32_t handleInt;
    } u;
} cpr_signal_t;


__END_DECLS

#endif
