









































#include <string.h>
#include "primpl.h"

extern PRLogModuleInfo *_pr_shm_lm;


#if defined PR_HAVE_SYSV_NAMED_SHARED_MEMORY

#elif defined PR_HAVE_POSIX_NAMED_SHARED_MEMORY

#elif defined PR_HAVE_WIN32_NAMED_SHARED_MEMORY

#else 



extern PRSharedMemory*  _MD_OpenSharedMemory( const char *name, PRSize size, PRIntn flags, PRIntn mode )
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}    

extern void * _MD_AttachSharedMemory( PRSharedMemory *shm, PRIntn flags )
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}    

extern PRStatus _MD_DetachSharedMemory( PRSharedMemory *shm, void *addr )
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}    

extern PRStatus _MD_CloseSharedMemory( PRSharedMemory *shm )
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}    

extern PRStatus _MD_DeleteSharedMemory( const char *name )
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}    
#endif 





PR_IMPLEMENT( PRSharedMemory * )
    PR_OpenSharedMemory(
        const char *name,
        PRSize      size,
        PRIntn      flags,
        PRIntn      mode
)
{
    if (!_pr_initialized) _PR_ImplicitInitialization();
    return( _PR_MD_OPEN_SHARED_MEMORY( name, size, flags, mode ));
} 





PR_IMPLEMENT( void * )
    PR_AttachSharedMemory(
        PRSharedMemory *shm,
        PRIntn          flags
)
{
    return( _PR_MD_ATTACH_SHARED_MEMORY( shm, flags ));
} 





PR_IMPLEMENT( PRStatus )
    PR_DetachSharedMemory(
        PRSharedMemory *shm,
        void *addr
)
{
    return( _PR_MD_DETACH_SHARED_MEMORY( shm, addr ));
} 





PR_IMPLEMENT( PRStatus )
    PR_CloseSharedMemory(
        PRSharedMemory *shm
)
{
    return( _PR_MD_CLOSE_SHARED_MEMORY( shm ));
} 





PR_EXTERN( PRStatus )
    PR_DeleteSharedMemory(
        const char *name
)
{
    if (!_pr_initialized) _PR_ImplicitInitialization();
    return(_PR_MD_DELETE_SHARED_MEMORY( name ));
} 

