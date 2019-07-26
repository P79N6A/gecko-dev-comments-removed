






































#include "cpr_types.h"
#include "cpr_debug.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "cpr_memory.h"
#include "cpr_locks.h"
#include "plat_api.h"
#include <errno.h>
#include <sys/syslog.h>

























#define MISC_LN 8
typedef struct
{
    void *Next;
    uint32_t Cmd;
    uint16_t RetID;
    uint16_t Port;
    uint16_t Len;
    uint8_t *Data;
    union {
        void *UsrPtr;
        uint32_t UsrInfo;
    } Usr;
    uint8_t Misc[MISC_LN];

} phn_syshdr_t;

 
































void *
cprGetBuffer (uint32_t size)
{
    static const char fname[] = "cprGetBuffer";
    cprLinuxBuffer_t *bufferPtr;
    char *charPtr;

    CPR_INFO("%s: Enter\n", fname);
    bufferPtr = (cprLinuxBuffer_t *) cpr_calloc(1, sizeof(cprLinuxBuffer_t) + size);
    if (bufferPtr != NULL) {
        bufferPtr->inUse = BUF_USED_FROM_HEAP;
        charPtr = (char *) (bufferPtr);
        return (charPtr + sizeof(cprLinuxBuffer_t));
    }

    CPR_ERROR("%s - Unable to malloc a buffer from the heap.\n", fname);
    errno = ENOMEM;
    return NULL;
}













void
cprReleaseBuffer (void *bufferPtr)
{
    static const char fname[] = "cprReleaseBuffer";
    cprLinuxBuffer_t *linuxBufferPtr;
    char *charPtr;

    CPR_INFO("%s: Enter\n", fname);

    


    if (bufferPtr == NULL) {
        CPR_ERROR("\n%s - Buffer pointer is NULL.\n", fname);
        return;
    }

    




    charPtr = (char *) (bufferPtr);
    linuxBufferPtr = (cprLinuxBuffer_t *) (charPtr - sizeof(cprLinuxBuffer_t));

    if (linuxBufferPtr->inUse == BUF_USED_FROM_HEAP) {
        cpr_free(linuxBufferPtr);
        return;
    }
    CPR_ERROR("%s: Buffer pointer 0x%08x: does not point to an in use buffer\n",
              fname, linuxBufferPtr);
    return;
}












void *
cprGetSysHeader (void *buffer)
{
    phn_syshdr_t *syshdr;

    



    syshdr = cpr_calloc(1, sizeof(phn_syshdr_t));
    if (syshdr) {
        syshdr->Data = buffer;
    }
    return (void *)syshdr;
}










void
cprReleaseSysHeader (void *syshdr)
{
    if (syshdr == NULL) {
        CPR_ERROR("cprReleaseSysHeader: Sys header pointer is NULL\n");
        return;
    }

    cpr_free(syshdr);
}

























void
fillInSysHeader (void *buffer, uint16_t cmd, uint16_t len, void *timerMsg)
{
    phn_syshdr_t *syshdr;

    syshdr = (phn_syshdr_t *) buffer;
    syshdr->Cmd = cmd;
    syshdr->Len = len;
    syshdr->Usr.UsrPtr = timerMsg;
    return;
}

