


















#include "unicode/utypes.h"
#include "cmemory.h"
#include "unicode/udata.h"

#include "udatamem.h"

U_CFUNC void UDataMemory_init(UDataMemory *This) {
    uprv_memset(This, 0, sizeof(UDataMemory));
    This->length=-1;
}


U_CFUNC void UDatamemory_assign(UDataMemory *dest, UDataMemory *source) {
    
    UBool mallocedFlag = dest->heapAllocated;
    uprv_memcpy(dest, source, sizeof(UDataMemory));
    dest->heapAllocated = mallocedFlag;
}

U_CFUNC UDataMemory *UDataMemory_createNewInstance(UErrorCode *pErr) {
    UDataMemory *This;

    if (U_FAILURE(*pErr)) {
        return NULL;
    }
    This = uprv_malloc(sizeof(UDataMemory));
    if (This == NULL) {
        *pErr = U_MEMORY_ALLOCATION_ERROR; }
    else {
        UDataMemory_init(This);
        This->heapAllocated = TRUE;
    }
    return This;
}


U_CFUNC const DataHeader *
UDataMemory_normalizeDataPointer(const void *p) {
    
    const DataHeader *pdh = (const DataHeader *)p;
    if(pdh==NULL || (pdh->dataHeader.magic1==0xda && pdh->dataHeader.magic2==0x27)) {
        return pdh;
    } else {
#if U_PLATFORM == U_PF_OS400
        











        return (const DataHeader *)*((const void **)p+1);
#else
        return (const DataHeader *)((const double *)p+1);
#endif
    }
}


U_CFUNC void UDataMemory_setData (UDataMemory *This, const void *dataAddr) {
    This->pHeader = UDataMemory_normalizeDataPointer(dataAddr);
}


U_CAPI void U_EXPORT2
udata_close(UDataMemory *pData) {
    if(pData!=NULL) {
        uprv_unmapFile(pData);
        if(pData->heapAllocated ) {
            uprv_free(pData);
        } else {
            UDataMemory_init(pData);
        }
    }
}

U_CAPI const void * U_EXPORT2
udata_getMemory(UDataMemory *pData) {
    if(pData!=NULL && pData->pHeader!=NULL) {
        return (char *)(pData->pHeader)+udata_getHeaderSize(pData->pHeader);
    } else {
        return NULL;
    }
}





















U_CAPI int32_t U_EXPORT2
udata_getLength(const UDataMemory *pData) {
    if(pData!=NULL && pData->pHeader!=NULL && pData->length>=0) {
        



        return pData->length-udata_getHeaderSize(pData->pHeader);
    } else {
        return -1;
    }
}






U_CAPI const void * U_EXPORT2
udata_getRawMemory(const UDataMemory *pData) {
    if(pData!=NULL && pData->pHeader!=NULL) {
        return pData->pHeader;
    } else {
        return NULL;
    }
}

U_CFUNC UBool UDataMemory_isLoaded(const UDataMemory *This) {
    return This->pHeader != NULL;
}
