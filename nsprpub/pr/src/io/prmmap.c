












































#include "primpl.h"

PR_IMPLEMENT(PRFileMap *) PR_CreateFileMap(
    PRFileDesc *fd,
    PRInt64 size,
    PRFileMapProtect prot)
{
    PRFileMap *fmap;

    PR_ASSERT(prot == PR_PROT_READONLY || prot == PR_PROT_READWRITE
            || prot == PR_PROT_WRITECOPY);
    fmap = PR_NEWZAP(PRFileMap);
    if (NULL == fmap) {
	PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
	return NULL;
    }
    fmap->fd = fd;
    fmap->prot = prot;
    if (_PR_MD_CREATE_FILE_MAP(fmap, size) == PR_SUCCESS) {
	return fmap;
    } else {
	PR_DELETE(fmap);
	return NULL;
    }
}

PR_IMPLEMENT(PRInt32) PR_GetMemMapAlignment(void)
{
    return _PR_MD_GET_MEM_MAP_ALIGNMENT();
}

PR_IMPLEMENT(void *) PR_MemMap(
    PRFileMap *fmap,
    PROffset64 offset,
    PRUint32 len)
{
    return _PR_MD_MEM_MAP(fmap, offset, len);
}

PR_IMPLEMENT(PRStatus) PR_MemUnmap(void *addr, PRUint32 len)
{
    return _PR_MD_MEM_UNMAP(addr, len);
}

PR_IMPLEMENT(PRStatus) PR_CloseFileMap(PRFileMap *fmap)
{
    return _PR_MD_CLOSE_FILE_MAP(fmap);
}
