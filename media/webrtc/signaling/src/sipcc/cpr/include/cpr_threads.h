



#ifndef _CPR_THREADS_H_
#define _CPR_THREADS_H_

#include "cpr_types.h"

__BEGIN_DECLS





typedef void *cprThread_t;











typedef struct {
    const char *name;
    uint32_t threadId;
    union {
        void *handlePtr;
        uint64_t handleInt;
    } u;
} cpr_thread_t;




typedef void *(*cprThreadStartRoutine)(void *data);

























cprThread_t cprCreateThread(const char *name,
                            cprThreadStartRoutine startRoutine,
                            uint16_t stackSize,
                            uint16_t priority,
                            void *data);







void cprJoinThread(cprThread_t thread);
















cprRC_t cprDestroyThread(cprThread_t thread);
































cprRC_t cprAdjustRelativeThreadPriority(int relPri);

__END_DECLS

#endif
