






































#include "cpr.h"
#include "cpr_stdlib.h"
#include "cpr_win_defines.h"


#ifdef TRACK_ALLOCS
uint32_t total = 0;

#define MAX_TRACKED 2048
typedef struct _mem_info
{
    void *ptr;
    int   size;
} mem_info_t;

mem_info_t memdata[MAX_TRACKED] = { {0, 0} };

void
record_ptr (void *ptr, int size)
{
    int i;

    for (i = 0; i < MAX_TRACKED; i++) {
        if (memdata[i].ptr == 0) {
            memdata[i].ptr = ptr;
            memdata[i].size = size;
            return;
        }
    }
    buginf("record_ptr():did not find a slot..\n");
    return;
}

int
get_ptr_size_and_free_slot (void *ptr)
{
    int i;
    int size = 0;

    for (i = 0; i < MAX_TRACKED; i++) {
        if (memdata[i].ptr == ptr) {
            memdata[i].ptr = 0;
            size = memdata[i].size;
            memdata[i].size = 0;
            break;
        }
    }
    if (size == 0) {
        buginf("get_ptr():did not find the ptr..\n");
    }
    return size;
}
#endif




#define MISC_LN 8
typedef struct
{
    void    *Next;
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
    void *TempPtr;
} phn_syshdr_t;

char *strdup(const char *strSource);

#ifndef CPR_USE_DIRECT_OS_CALL
void *
cpr_malloc (size_t size)
{
#ifdef TRACK_ALLOCS
    char *ptr;

    total = total + size;
    buginf("cpr_malloc(): total=%d size=%d\n", total, size);
    ptr = calloc(1, size);
    record_ptr(ptr, size);
    return (ptr);
#else
    return calloc(1, size);
#endif
}

void *
cpr_calloc (size_t nelem, size_t size)
{
#ifdef TRACK_ALLOCS
    buginf("cpr_calloc(): called nelem=%d size=%d\n", nelem, size);
    return (cpr_malloc(nelem * size));
#else
    return calloc(nelem, size);
#endif
}

void *
cpr_realloc (void *object, size_t size)
{
    return realloc(object, size);
}

char *
cpr_strdup (const char *string)
{
    return strdup(string);
}

void
cpr_free (void *mem)
{
#ifdef TRACK_ALLOCS
    int sz;

    sz = get_ptr_size_and_free_slot(mem);
    if (total && sz) {
        total = total - sz;
    }

    buginf("cpr_free(): total=%d size=%d\n", total, sz);
#endif

    free(mem);
}
#endif













cprRegion_t
cprCreateRegion (const char *regionName)
{
    static uint32_t regionId = 0;
    uint32_t *region;

    





    region = cpr_malloc(sizeof(uint32_t));
    if (!region) {
        return NULL;
    }
    *region = ++regionId;

    return region;
}














cprPool_t
cprCreatePool (cprRegion_t region,
               const char *name,
               uint32_t initialBuffers,
               uint32_t bufferSize)
{
    static uint32_t poolId = 0;
    uint32_t *pool;

    





    if (!region) {
        return NULL;
    }

    pool = cpr_malloc(sizeof(cprPool_t));
    if (!pool) {
        return NULL;
    }
    *pool = ++poolId;

    return pool;
}











cprRC_t
cprDestroyRegion (cprRegion_t region)
{
    if (region) {
        cpr_free(region);
    }

    return (CPR_SUCCESS);
}











cprRC_t
cprDestroyPool (cprPool_t pool)
{
    if (pool) {
        cpr_free(pool);
    }

    return (CPR_SUCCESS);
}














































void
cprReleaseBuffer (cprBuffer_t buffer)
{

    


    if (buffer) {
        free(buffer);
    }
}















void *
fillInSysHeader (void *buffer, uint16_t cmd, uint16_t len, void *timerMsg)
{
    phn_syshdr_t *syshdr;

    syshdr = (phn_syshdr_t *) buffer;
    syshdr->Cmd = cmd;
    syshdr->Len = len;
    syshdr->Usr.UsrPtr = timerMsg;
    return buffer;
}












void *
cprGetSysHeader (void *buffer)
{
    phn_syshdr_t *syshdr;

    



    syshdr = calloc(1, sizeof(phn_syshdr_t));
    if (!syshdr) {
        syshdr->Data = buffer;
    }
    return (void *) syshdr;
}











void
cprReleaseSysHeader (void *sysHdr)
{
    if (sysHdr) {
        free(sysHdr);
    }
}

