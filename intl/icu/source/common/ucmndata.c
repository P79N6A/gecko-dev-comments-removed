





















#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "cstring.h"
#include "ucmndata.h"
#include "udatamem.h"

#if defined(UDATA_DEBUG) || defined(UDATA_DEBUG_DUMP)
#   include <stdio.h>
#endif

U_CFUNC uint16_t
udata_getHeaderSize(const DataHeader *udh) {
    if(udh==NULL) {
        return 0;
    } else if(udh->info.isBigEndian==U_IS_BIG_ENDIAN) {
        
        return udh->dataHeader.headerSize;
    } else {
        
        uint16_t x=udh->dataHeader.headerSize;
        return (uint16_t)((x<<8)|(x>>8));
    }
}

U_CFUNC uint16_t
udata_getInfoSize(const UDataInfo *info) {
    if(info==NULL) {
        return 0;
    } else if(info->isBigEndian==U_IS_BIG_ENDIAN) {
        
        return info->size;
    } else {
        
        uint16_t x=info->size;
        return (uint16_t)((x<<8)|(x>>8));
    }
}









typedef struct {
    const char       *entryName;
    const DataHeader *pHeader;
} PointerTOCEntry;


typedef struct  {
    uint32_t          count;
    uint32_t          reserved;
    PointerTOCEntry   entry[2];   
}  PointerTOC;










#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif





static int32_t
strcmpAfterPrefix(const char *s1, const char *s2, int32_t *pPrefixLength) {
    int32_t pl=*pPrefixLength;
    int32_t cmp=0;
    s1+=pl;
    s2+=pl;
    for(;;) {
        int32_t c1=(uint8_t)*s1++;
        int32_t c2=(uint8_t)*s2++;
        cmp=c1-c2;
        if(cmp!=0 || c1==0) {  
            break;
        }
        ++pl;  
    }
    *pPrefixLength=pl;
    return cmp;
}

static int32_t
offsetTOCPrefixBinarySearch(const char *s, const char *names,
                            const UDataOffsetTOCEntry *toc, int32_t count) {
    int32_t start=0;
    int32_t limit=count;
    




    int32_t startPrefixLength=0;
    int32_t limitPrefixLength=0;
    if(count==0) {
        return -1;
    }
    





    if(0==strcmpAfterPrefix(s, names+toc[0].nameOffset, &startPrefixLength)) {
        return 0;
    }
    ++start;
    --limit;
    if(0==strcmpAfterPrefix(s, names+toc[limit].nameOffset, &limitPrefixLength)) {
        return limit;
    }
    while(start<limit) {
        int32_t i=(start+limit)/2;
        int32_t prefixLength=MIN(startPrefixLength, limitPrefixLength);
        int32_t cmp=strcmpAfterPrefix(s, names+toc[i].nameOffset, &prefixLength);
        if(cmp<0) {
            limit=i;
            limitPrefixLength=prefixLength;
        } else if(cmp==0) {
            return i;
        } else {
            start=i+1;
            startPrefixLength=prefixLength;
        }
    }
    return -1;
}

static int32_t
pointerTOCPrefixBinarySearch(const char *s, const PointerTOCEntry *toc, int32_t count) {
    int32_t start=0;
    int32_t limit=count;
    




    int32_t startPrefixLength=0;
    int32_t limitPrefixLength=0;
    if(count==0) {
        return -1;
    }
    





    if(0==strcmpAfterPrefix(s, toc[0].entryName, &startPrefixLength)) {
        return 0;
    }
    ++start;
    --limit;
    if(0==strcmpAfterPrefix(s, toc[limit].entryName, &limitPrefixLength)) {
        return limit;
    }
    while(start<limit) {
        int32_t i=(start+limit)/2;
        int32_t prefixLength=MIN(startPrefixLength, limitPrefixLength);
        int32_t cmp=strcmpAfterPrefix(s, toc[i].entryName, &prefixLength);
        if(cmp<0) {
            limit=i;
            limitPrefixLength=prefixLength;
        } else if(cmp==0) {
            return i;
        } else {
            start=i+1;
            startPrefixLength=prefixLength;
        }
    }
    return -1;
}

static uint32_t offsetTOCEntryCount(const UDataMemory *pData) {
    int32_t          retVal=0;
    const UDataOffsetTOC *toc = (UDataOffsetTOC *)pData->toc;
    if (toc != NULL) {
        retVal = toc->count;
    }
    return retVal;
}

static const DataHeader *
offsetTOCLookupFn(const UDataMemory *pData,
                  const char *tocEntryName,
                  int32_t *pLength,
                  UErrorCode *pErrorCode) {
    const UDataOffsetTOC  *toc = (UDataOffsetTOC *)pData->toc;
    if(toc!=NULL) {
        const char *base=(const char *)toc;
        int32_t number, count=(int32_t)toc->count;

        
#if defined (UDATA_DEBUG_DUMP)
        
        for(number=0; number<count; ++number) {
            fprintf(stderr, "\tx%d: %s\n", number, &base[toc->entry[number].nameOffset]);
        }
#endif
        number=offsetTOCPrefixBinarySearch(tocEntryName, base, toc->entry, count);
        if(number>=0) {
            
            const UDataOffsetTOCEntry *entry=toc->entry+number;
#ifdef UDATA_DEBUG
            fprintf(stderr, "%s: Found.\n", tocEntryName);
#endif
            if((number+1) < count) {
                *pLength = (int32_t)(entry[1].dataOffset - entry->dataOffset);
            } else {
                *pLength = -1;
            }
            return (const DataHeader *)(base+entry->dataOffset);
        } else {
#ifdef UDATA_DEBUG
            fprintf(stderr, "%s: Not found.\n", tocEntryName);
#endif
            return NULL;
        }
    } else {
#ifdef UDATA_DEBUG
        fprintf(stderr, "returning header\n");
#endif

        return pData->pHeader;
    }
}


static uint32_t pointerTOCEntryCount(const UDataMemory *pData) {
    const PointerTOC *toc = (PointerTOC *)pData->toc;
    return (uint32_t)((toc != NULL) ? (toc->count) : 0);
}


static const DataHeader *pointerTOCLookupFn(const UDataMemory *pData,
                   const char *name,
                   int32_t *pLength,
                   UErrorCode *pErrorCode) {
    if(pData->toc!=NULL) {
        const PointerTOC *toc = (PointerTOC *)pData->toc;
        int32_t number, count=(int32_t)toc->count;

#if defined (UDATA_DEBUG_DUMP)
        
        for(number=0; number<count; ++number) {
            fprintf(stderr, "\tx%d: %s\n", number, toc->entry[number].entryName);
        }
#endif
        number=pointerTOCPrefixBinarySearch(name, toc->entry, count);
        if(number>=0) {
            
#ifdef UDATA_DEBUG
            fprintf(stderr, "%s: Found.\n", toc->entry[number].entryName);
#endif
            *pLength=-1;
            return UDataMemory_normalizeDataPointer(toc->entry[number].pHeader);
        } else {
#ifdef UDATA_DEBUG
            fprintf(stderr, "%s: Not found.\n", name);
#endif
            return NULL;
        }
    } else {
        return pData->pHeader;
    }
}

static const commonDataFuncs CmnDFuncs = {offsetTOCLookupFn,  offsetTOCEntryCount};
static const commonDataFuncs ToCPFuncs = {pointerTOCLookupFn, pointerTOCEntryCount};











U_CFUNC void udata_checkCommonData(UDataMemory *udm, UErrorCode *err) {
    if (U_FAILURE(*err)) {
        return;
    }

    if(udm==NULL || udm->pHeader==NULL) {
      *err=U_INVALID_FORMAT_ERROR;
    } else if(!(udm->pHeader->dataHeader.magic1==0xda &&
        udm->pHeader->dataHeader.magic2==0x27 &&
        udm->pHeader->info.isBigEndian==U_IS_BIG_ENDIAN &&
        udm->pHeader->info.charsetFamily==U_CHARSET_FAMILY)
        ) {
        
        *err=U_INVALID_FORMAT_ERROR;
    }
    else if (udm->pHeader->info.dataFormat[0]==0x43 &&
        udm->pHeader->info.dataFormat[1]==0x6d &&
        udm->pHeader->info.dataFormat[2]==0x6e &&
        udm->pHeader->info.dataFormat[3]==0x44 &&
        udm->pHeader->info.formatVersion[0]==1
        ) {
        
        udm->vFuncs = &CmnDFuncs;
        udm->toc=(const char *)udm->pHeader+udata_getHeaderSize(udm->pHeader);
    }
    else if(udm->pHeader->info.dataFormat[0]==0x54 &&
        udm->pHeader->info.dataFormat[1]==0x6f &&
        udm->pHeader->info.dataFormat[2]==0x43 &&
        udm->pHeader->info.dataFormat[3]==0x50 &&
        udm->pHeader->info.formatVersion[0]==1
        ) {
        
        udm->vFuncs = &ToCPFuncs;
        udm->toc=(const char *)udm->pHeader+udata_getHeaderSize(udm->pHeader);
    }
    else {
        
        *err=U_INVALID_FORMAT_ERROR;
    }

    if (U_FAILURE(*err)) {
        



        udata_close(udm);
    }
}




















