



































#include <string.h>
#include "mmapio.h"
#include "prmem.h"
#include "prlog.h"

struct MmioFileStruct
{
    PRFileDesc *fd;
    PRFileMap  *fileMap;
    PRUint32   fsize; 
    PRUint32   msize; 
    PRInt32    pos;   
    char       *addr; 
    PRBool     needSeek; 
};

PRStatus mmio_FileSeek(MmioFile *mmio, PRInt32 offset, PRSeekWhence whence)
{
    mmio->needSeek = PR_TRUE;

    switch(whence) {
        case PR_SEEK_SET:
            mmio->pos = offset;
            break;
        case PR_SEEK_END:
            mmio->pos = mmio->fsize + offset;
            break;
        case PR_SEEK_CUR:
            mmio->pos = mmio->pos + offset;
            break;
        default:
            return PR_FAILURE;
    }

    if(mmio->pos<0) {
        mmio->pos = 0;
    }

    return PR_SUCCESS;
}

PRInt32  mmio_FileRead(MmioFile *mmio, char *dest, PRInt32 count)
{
    static PRFileMapProtect prot = PR_PROT_READONLY;
    static PRInt64 fsize_l;

    


    if(mmio->pos+count > mmio->fsize) {
        count = mmio->fsize - mmio->pos;
    }

    if(count<1) {
        return 0;
    }

    
    if(mmio->pos+count > mmio->msize) {
        if(mmio->addr && mmio->msize) {
            PR_ASSERT(mmio->fileMap);
            PR_MemUnmap(mmio->addr, mmio->msize);
            PR_CloseFileMap(mmio->fileMap);
            mmio->addr  = NULL;
            mmio->msize = 0;
        }

        LL_UI2L(fsize_l, mmio->fsize);
        mmio->fileMap = PR_CreateFileMap(mmio->fd, fsize_l, prot);

        if(!mmio->fileMap) {
            return -1;
        }

        mmio->addr = PR_MemMap(mmio->fileMap, 0, fsize_l);

        if(!mmio->addr) {
            return -1;
        }

        mmio->msize = mmio->fsize;
    }

    memcpy(dest, mmio->addr+mmio->pos, count);

    mmio->pos += count;
    mmio->needSeek = PR_TRUE;

    return count;
}

PRInt32  mmio_FileWrite(MmioFile *mmio, const char *src, PRInt32 count)
{
    PRInt32 wcode;

    if(mmio->needSeek) {
        PR_Seek(mmio->fd, mmio->pos, PR_SEEK_SET);
        mmio->needSeek = PR_FALSE;
    }

    





#if MMAP_MISSES_WRITES
    if(mmio->addr && mmio->msize) {
	PR_ASSERT(mmio->fileMap);
	PR_MemUnmap(mmio->addr, mmio->msize);
	PR_CloseFileMap(mmio->fileMap);
	mmio->addr  = NULL;
	mmio->msize = 0;
    }
#endif

    wcode = PR_Write(mmio->fd, src, count);

    if(wcode>0) {
        mmio->pos += wcode;
        if(mmio->pos>mmio->fsize) {
            mmio->fsize=mmio->pos;
        }
    }

    return wcode;
}

PRInt32  mmio_FileTell(MmioFile *mmio)
{
    return mmio->pos;
}

PRStatus mmio_FileClose(MmioFile *mmio)
{
    if(mmio->addr && mmio->msize) {
        PR_ASSERT(mmio->fileMap);
        PR_MemUnmap(mmio->addr, mmio->msize);
        PR_CloseFileMap(mmio->fileMap);
    }

    PR_Close(mmio->fd);

    memset(mmio, 0, sizeof(*mmio)); 

    PR_Free(mmio);

    return PR_SUCCESS;
}

MmioFile *mmio_FileOpen(char *path, PRIntn flags, PRIntn mode)
{
    PRFileDesc *fd = PR_Open(path, flags, mode);
    PRFileInfo info;
    MmioFile   *mmio;

    if(!fd) {
        return NULL;
    }

    mmio = PR_MALLOC(sizeof(MmioFile));

    if(!mmio || PR_FAILURE==PR_GetOpenFileInfo(fd, &info)) {
        PR_Close(fd);
        return NULL;
    }

    mmio->fd = fd;
    mmio->fileMap = NULL;
    mmio->fsize = info.size;
    mmio->msize = 0;
    mmio->pos   = 0;
    mmio->addr  = NULL;
    mmio->needSeek = PR_FALSE;

    return mmio;
}
