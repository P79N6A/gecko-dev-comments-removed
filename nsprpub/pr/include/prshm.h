

































































































































#ifndef prshm_h___
#define prshm_h___

#include "prtypes.h"
#include "prio.h"

PR_BEGIN_EXTERN_C




typedef struct PRSharedMemory PRSharedMemory;




































NSPR_API( PRSharedMemory * )
    PR_OpenSharedMemory(
        const char *name,
        PRSize      size,
        PRIntn      flags,
        PRIntn      mode
);

#define PR_SHM_CREATE 0x1  /* create if not exist */
#define PR_SHM_EXCL   0x2  /* fail if already exists */
























NSPR_API( void * )
    PR_AttachSharedMemory(
        PRSharedMemory *shm,
        PRIntn  flags
);
 
#define PR_SHM_READONLY 0x01



















NSPR_API( PRStatus )
    PR_DetachSharedMemory(
        PRSharedMemory *shm,
        void  *addr
);

















NSPR_API( PRStatus )
    PR_CloseSharedMemory(
        PRSharedMemory *shm
);

















NSPR_API( PRStatus )
    PR_DeleteSharedMemory( 
        const char *name
);

PR_END_EXTERN_C

#endif 
