

















#ifndef __UDATAMEM_H__
#define __UDATAMEM_H__

#include "unicode/udata.h"
#include "ucmndata.h"

struct UDataMemory {
    const commonDataFuncs  *vFuncs;      

    const DataHeader *pHeader;     
                                   
    const void       *toc;         
                                   
    UBool             heapAllocated;  
                                   

    void             *mapAddr;     
                                   
                                   
    void             *map;         
                                   
                                   
                                   
    int32_t           length;      
};

U_CFUNC UDataMemory *UDataMemory_createNewInstance(UErrorCode *pErr);
U_CFUNC void         UDatamemory_assign  (UDataMemory *dest, UDataMemory *source);
U_CFUNC void         UDataMemory_init    (UDataMemory *This);
U_CFUNC UBool        UDataMemory_isLoaded(const UDataMemory *This);
U_CFUNC void         UDataMemory_setData (UDataMemory *This, const void *dataAddr);

U_CFUNC const DataHeader *UDataMemory_normalizeDataPointer(const void *p);

U_CAPI int32_t U_EXPORT2
udata_getLength(const UDataMemory *pData);

U_CAPI const void * U_EXPORT2
udata_getRawMemory(const UDataMemory *pData);

#endif
