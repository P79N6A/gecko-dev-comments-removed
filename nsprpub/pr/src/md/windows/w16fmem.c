




































#include "primpl.h"









PRStatus _MD_CreateFileMap(PRFileMap *fmap, PRInt64 size)
{
    PR_ASSERT(!"Not implemented");
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

PRInt32 _MD_GetMemMapAlignment(void)
{
    PR_ASSERT(!"Not implemented");
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return -1;
}

void * _MD_MemMap(
    PRFileMap *fmap,
    PRInt64 offset,
    PRUint32 len)
{
    PR_ASSERT(!"Not implemented");
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}

PRStatus _MD_MemUnmap(void *addr, PRUint32 len)
{
    PR_ASSERT(!"Not implemented");
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

PRStatus _MD_CloseFileMap(PRFileMap *fmap)
{
    PR_ASSERT(!"Not implemented");
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

