



















#ifdef UTRIE_DEBUG
#   include <stdio.h>
#endif

#include "unicode/utypes.h"
#include "cmemory.h"
#include "utrie.h"



#undef ABS
#define ABS(x) ((x)>=0 ? (x) : -(x))

static inline UBool
equal_uint32(const uint32_t *s, const uint32_t *t, int32_t length) {
    while(length>0 && *s==*t) {
        ++s;
        ++t;
        --length;
    }
    return (UBool)(length==0);
}



U_CAPI UNewTrie * U_EXPORT2
utrie_open(UNewTrie *fillIn,
           uint32_t *aliasData, int32_t maxDataLength,
           uint32_t initialValue, uint32_t leadUnitValue,
           UBool latin1Linear) {
    UNewTrie *trie;
    int32_t i, j;

    if( maxDataLength<UTRIE_DATA_BLOCK_LENGTH ||
        (latin1Linear && maxDataLength<1024)
    ) {
        return NULL;
    }

    if(fillIn!=NULL) {
        trie=fillIn;
    } else {
        trie=(UNewTrie *)uprv_malloc(sizeof(UNewTrie));
        if(trie==NULL) {
            return NULL;
        }
    }
    uprv_memset(trie, 0, sizeof(UNewTrie));
    trie->isAllocated= (UBool)(fillIn==NULL);

    if(aliasData!=NULL) {
        trie->data=aliasData;
        trie->isDataAllocated=FALSE;
    } else {
        trie->data=(uint32_t *)uprv_malloc(maxDataLength*4);
        if(trie->data==NULL) {
            uprv_free(trie);
            return NULL;
        }
        trie->isDataAllocated=TRUE;
    }

    
    j=UTRIE_DATA_BLOCK_LENGTH;

    if(latin1Linear) {
        
        

        
        i=0;
        do {
            
            trie->index[i++]=j;
            j+=UTRIE_DATA_BLOCK_LENGTH;
        } while(i<(256>>UTRIE_SHIFT));
    }

    
    trie->dataLength=j;
    while(j>0) {
        trie->data[--j]=initialValue;
    }

    trie->leadUnitValue=leadUnitValue;
    trie->indexLength=UTRIE_MAX_INDEX_LENGTH;
    trie->dataCapacity=maxDataLength;
    trie->isLatin1Linear=latin1Linear;
    trie->isCompacted=FALSE;
    return trie;
}

U_CAPI UNewTrie * U_EXPORT2
utrie_clone(UNewTrie *fillIn, const UNewTrie *other, uint32_t *aliasData, int32_t aliasDataCapacity) {
    UNewTrie *trie;
    UBool isDataAllocated;

    
    if(other==NULL || other->data==NULL || other->isCompacted) {
        return NULL;
    }

    
    if(aliasData!=NULL && aliasDataCapacity>=other->dataCapacity) {
        isDataAllocated=FALSE;
    } else {
        aliasDataCapacity=other->dataCapacity;
        aliasData=(uint32_t *)uprv_malloc(other->dataCapacity*4);
        if(aliasData==NULL) {
            return NULL;
        }
        isDataAllocated=TRUE;
    }

    trie=utrie_open(fillIn, aliasData, aliasDataCapacity,
                    other->data[0], other->leadUnitValue,
                    other->isLatin1Linear);
    if(trie==NULL) {
        uprv_free(aliasData);
    } else {
        uprv_memcpy(trie->index, other->index, sizeof(trie->index));
        uprv_memcpy(trie->data, other->data, other->dataLength*4);
        trie->dataLength=other->dataLength;
        trie->isDataAllocated=isDataAllocated;
    }

    return trie;
}

U_CAPI void U_EXPORT2
utrie_close(UNewTrie *trie) {
    if(trie!=NULL) {
        if(trie->isDataAllocated) {
            uprv_free(trie->data);
            trie->data=NULL;
        }
        if(trie->isAllocated) {
            uprv_free(trie);
        }
    }
}

U_CAPI uint32_t * U_EXPORT2
utrie_getData(UNewTrie *trie, int32_t *pLength) {
    if(trie==NULL || pLength==NULL) {
        return NULL;
    }

    *pLength=trie->dataLength;
    return trie->data;
}

static int32_t
utrie_allocDataBlock(UNewTrie *trie) {
    int32_t newBlock, newTop;

    newBlock=trie->dataLength;
    newTop=newBlock+UTRIE_DATA_BLOCK_LENGTH;
    if(newTop>trie->dataCapacity) {
        
        return -1;
    }
    trie->dataLength=newTop;
    return newBlock;
}







static int32_t
utrie_getDataBlock(UNewTrie *trie, UChar32 c) {
    int32_t indexValue, newBlock;

    c>>=UTRIE_SHIFT;
    indexValue=trie->index[c];
    if(indexValue>0) {
        return indexValue;
    }

    
    newBlock=utrie_allocDataBlock(trie);
    if(newBlock<0) {
        
        return -1;
    }
    trie->index[c]=newBlock;

    
    uprv_memcpy(trie->data+newBlock, trie->data-indexValue, 4*UTRIE_DATA_BLOCK_LENGTH);
    return newBlock;
}




U_CAPI UBool U_EXPORT2
utrie_set32(UNewTrie *trie, UChar32 c, uint32_t value) {
    int32_t block;

    
    if(trie==NULL || trie->isCompacted || (uint32_t)c>0x10ffff) {
        return FALSE;
    }

    block=utrie_getDataBlock(trie, c);
    if(block<0) {
        return FALSE;
    }

    trie->data[block+(c&UTRIE_MASK)]=value;
    return TRUE;
}

U_CAPI uint32_t U_EXPORT2
utrie_get32(UNewTrie *trie, UChar32 c, UBool *pInBlockZero) {
    int32_t block;

    
    if(trie==NULL || trie->isCompacted || (uint32_t)c>0x10ffff) {
        if(pInBlockZero!=NULL) {
            *pInBlockZero=TRUE;
        }
        return 0;
    }

    block=trie->index[c>>UTRIE_SHIFT];
    if(pInBlockZero!=NULL) {
        *pInBlockZero= (UBool)(block==0);
    }

    return trie->data[ABS(block)+(c&UTRIE_MASK)];
}




static void
utrie_fillBlock(uint32_t *block, UChar32 start, UChar32 limit,
                uint32_t value, uint32_t initialValue, UBool overwrite) {
    uint32_t *pLimit;

    pLimit=block+limit;
    block+=start;
    if(overwrite) {
        while(block<pLimit) {
            *block++=value;
        }
    } else {
        while(block<pLimit) {
            if(*block==initialValue) {
                *block=value;
            }
            ++block;
        }
    }
}

U_CAPI UBool U_EXPORT2
utrie_setRange32(UNewTrie *trie, UChar32 start, UChar32 limit, uint32_t value, UBool overwrite) {
    




    uint32_t initialValue;
    int32_t block, rest, repeatBlock;

    
    if( trie==NULL || trie->isCompacted ||
        (uint32_t)start>0x10ffff || (uint32_t)limit>0x110000 || start>limit
    ) {
        return FALSE;
    }
    if(start==limit) {
        return TRUE; 
    }

    initialValue=trie->data[0];
    if(start&UTRIE_MASK) {
        UChar32 nextStart;

        
        block=utrie_getDataBlock(trie, start);
        if(block<0) {
            return FALSE;
        }

        nextStart=(start+UTRIE_DATA_BLOCK_LENGTH)&~UTRIE_MASK;
        if(nextStart<=limit) {
            utrie_fillBlock(trie->data+block, start&UTRIE_MASK, UTRIE_DATA_BLOCK_LENGTH,
                            value, initialValue, overwrite);
            start=nextStart;
        } else {
            utrie_fillBlock(trie->data+block, start&UTRIE_MASK, limit&UTRIE_MASK,
                            value, initialValue, overwrite);
            return TRUE;
        }
    }

    
    rest=limit&UTRIE_MASK;

    
    limit&=~UTRIE_MASK;

    
    if(value==initialValue) {
        repeatBlock=0;
    } else {
        repeatBlock=-1;
    }
    while(start<limit) {
        
        block=trie->index[start>>UTRIE_SHIFT];
        if(block>0) {
            
            utrie_fillBlock(trie->data+block, 0, UTRIE_DATA_BLOCK_LENGTH, value, initialValue, overwrite);
        } else if(trie->data[-block]!=value && (block==0 || overwrite)) {
            
            if(repeatBlock>=0) {
                trie->index[start>>UTRIE_SHIFT]=-repeatBlock;
            } else {
                
                repeatBlock=utrie_getDataBlock(trie, start);
                if(repeatBlock<0) {
                    return FALSE;
                }

                
                trie->index[start>>UTRIE_SHIFT]=-repeatBlock;
                utrie_fillBlock(trie->data+repeatBlock, 0, UTRIE_DATA_BLOCK_LENGTH, value, initialValue, TRUE);
            }
        }

        start+=UTRIE_DATA_BLOCK_LENGTH;
    }

    if(rest>0) {
        
        block=utrie_getDataBlock(trie, start);
        if(block<0) {
            return FALSE;
        }

        utrie_fillBlock(trie->data+block, 0, rest, value, initialValue, overwrite);
    }

    return TRUE;
}

static int32_t
_findSameIndexBlock(const int32_t *idx, int32_t indexLength,
                    int32_t otherBlock) {
    int32_t block, i;

    for(block=UTRIE_BMP_INDEX_LENGTH; block<indexLength; block+=UTRIE_SURROGATE_BLOCK_COUNT) {
        for(i=0; i<UTRIE_SURROGATE_BLOCK_COUNT; ++i) {
            if(idx[block+i]!=idx[otherBlock+i]) {
                break;
            }
        }
        if(i==UTRIE_SURROGATE_BLOCK_COUNT) {
            return block;
        }
    }
    return indexLength;
}











static void
utrie_fold(UNewTrie *trie, UNewTrieGetFoldedValue *getFoldedValue, UErrorCode *pErrorCode) {
    int32_t leadIndexes[UTRIE_SURROGATE_BLOCK_COUNT];
    int32_t *idx;
    uint32_t value;
    UChar32 c;
    int32_t indexLength, block;
#ifdef UTRIE_DEBUG
    int countLeadCUWithData=0;
#endif

    idx=trie->index;

    
    uprv_memcpy(leadIndexes, idx+(0xd800>>UTRIE_SHIFT), 4*UTRIE_SURROGATE_BLOCK_COUNT);

    









    if(trie->leadUnitValue==trie->data[0]) {
        block=0; 
    } else {
        
        block=utrie_allocDataBlock(trie);
        if(block<0) {
            
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        utrie_fillBlock(trie->data+block, 0, UTRIE_DATA_BLOCK_LENGTH, trie->leadUnitValue, trie->data[0], TRUE);
        block=-block; 
    }
    for(c=(0xd800>>UTRIE_SHIFT); c<(0xdc00>>UTRIE_SHIFT); ++c) {
        trie->index[c]=block;
    }

    






    indexLength=UTRIE_BMP_INDEX_LENGTH;

    
    for(c=0x10000; c<0x110000;) {
        if(idx[c>>UTRIE_SHIFT]!=0) {
            
            c&=~0x3ff;

#ifdef UTRIE_DEBUG
            ++countLeadCUWithData;
            
#endif

            
            block=_findSameIndexBlock(idx, indexLength, c>>UTRIE_SHIFT);

            




            value=getFoldedValue(trie, c, block+UTRIE_SURROGATE_BLOCK_COUNT);
            if(value!=utrie_get32(trie, U16_LEAD(c), NULL)) {
                if(!utrie_set32(trie, U16_LEAD(c), value)) {
                    
                    *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                    return;
                }

                
                if(block==indexLength) {
                    
                    uprv_memmove(idx+indexLength,
                                 idx+(c>>UTRIE_SHIFT),
                                 4*UTRIE_SURROGATE_BLOCK_COUNT);
                    indexLength+=UTRIE_SURROGATE_BLOCK_COUNT;
                }
            }
            c+=0x400;
        } else {
            c+=UTRIE_DATA_BLOCK_LENGTH;
        }
    }
#ifdef UTRIE_DEBUG
    if(countLeadCUWithData>0) {
        printf("supplementary data for %d lead surrogates\n", countLeadCUWithData);
    }
#endif

    








    if(indexLength>=UTRIE_MAX_INDEX_LENGTH) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    }

    



    uprv_memmove(idx+UTRIE_BMP_INDEX_LENGTH+UTRIE_SURROGATE_BLOCK_COUNT,
                 idx+UTRIE_BMP_INDEX_LENGTH,
                 4*(indexLength-UTRIE_BMP_INDEX_LENGTH));
    uprv_memcpy(idx+UTRIE_BMP_INDEX_LENGTH,
                leadIndexes,
                4*UTRIE_SURROGATE_BLOCK_COUNT);
    indexLength+=UTRIE_SURROGATE_BLOCK_COUNT;

#ifdef UTRIE_DEBUG
    printf("trie index count: BMP %ld  all Unicode %ld  folded %ld\n",
           UTRIE_BMP_INDEX_LENGTH, (long)UTRIE_MAX_INDEX_LENGTH, indexLength);
#endif

    trie->indexLength=indexLength;
}









static void
_findUnusedBlocks(UNewTrie *trie) {
    int32_t i;

    
    uprv_memset(trie->map, 0xff, (UTRIE_MAX_BUILD_TIME_DATA_LENGTH>>UTRIE_SHIFT)*4);

    
    for(i=0; i<trie->indexLength; ++i) {
        trie->map[ABS(trie->index[i])>>UTRIE_SHIFT]=0;
    }

    
    trie->map[0]=0;
}

static int32_t
_findSameDataBlock(const uint32_t *data, int32_t dataLength,
                   int32_t otherBlock, int32_t step) {
    int32_t block;

    
    dataLength-=UTRIE_DATA_BLOCK_LENGTH;

    for(block=0; block<=dataLength; block+=step) {
        if(equal_uint32(data+block, data+otherBlock, UTRIE_DATA_BLOCK_LENGTH)) {
            return block;
        }
    }
    return -1;
}













static void
utrie_compact(UNewTrie *trie, UBool overlap, UErrorCode *pErrorCode) {
    int32_t i, start, newStart, overlapStart;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    
    if(trie==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if(trie->isCompacted) {
        return; 
    }

    

    
    _findUnusedBlocks(trie);

    
    if(trie->isLatin1Linear && UTRIE_SHIFT<=8) {
        overlapStart=UTRIE_DATA_BLOCK_LENGTH+256;
    } else {
        overlapStart=UTRIE_DATA_BLOCK_LENGTH;
    }

    newStart=UTRIE_DATA_BLOCK_LENGTH;
    for(start=newStart; start<trie->dataLength;) {
        





        
        if(trie->map[start>>UTRIE_SHIFT]<0) {
            
            start+=UTRIE_DATA_BLOCK_LENGTH;

            
            continue;
        }

        
        if( start>=overlapStart &&
            (i=_findSameDataBlock(trie->data, newStart, start,
                            overlap ? UTRIE_DATA_GRANULARITY : UTRIE_DATA_BLOCK_LENGTH))
             >=0
        ) {
            
            trie->map[start>>UTRIE_SHIFT]=i;

            
            start+=UTRIE_DATA_BLOCK_LENGTH;

            
            continue;
        }

        
        if(overlap && start>=overlapStart) {
            
            for(i=UTRIE_DATA_BLOCK_LENGTH-UTRIE_DATA_GRANULARITY;
                i>0 && !equal_uint32(trie->data+(newStart-i), trie->data+start, i);
                i-=UTRIE_DATA_GRANULARITY) {}
        } else {
            i=0;
        }

        if(i>0) {
            
            trie->map[start>>UTRIE_SHIFT]=newStart-i;

            
            start+=i;
            for(i=UTRIE_DATA_BLOCK_LENGTH-i; i>0; --i) {
                trie->data[newStart++]=trie->data[start++];
            }
        } else if(newStart<start) {
            
            trie->map[start>>UTRIE_SHIFT]=newStart;
            for(i=UTRIE_DATA_BLOCK_LENGTH; i>0; --i) {
                trie->data[newStart++]=trie->data[start++];
            }
        } else  {
            trie->map[start>>UTRIE_SHIFT]=start;
            newStart+=UTRIE_DATA_BLOCK_LENGTH;
            start=newStart;
        }
    }

    
    for(i=0; i<trie->indexLength; ++i) {
        trie->index[i]=trie->map[ABS(trie->index[i])>>UTRIE_SHIFT];
    }

#ifdef UTRIE_DEBUG
    
    printf("compacting trie: count of 32-bit words %lu->%lu\n",
            (long)trie->dataLength, (long)newStart);
#endif

    trie->dataLength=newStart;
}
























static uint32_t U_CALLCONV
defaultGetFoldedValue(UNewTrie *trie, UChar32 start, int32_t offset) {
    uint32_t value, initialValue;
    UChar32 limit;
    UBool inBlockZero;

    initialValue=trie->data[0];
    limit=start+0x400;
    while(start<limit) {
        value=utrie_get32(trie, start, &inBlockZero);
        if(inBlockZero) {
            start+=UTRIE_DATA_BLOCK_LENGTH;
        } else if(value!=initialValue) {
            return (uint32_t)offset;
        } else {
            ++start;
        }
    }
    return 0;
}

U_CAPI int32_t U_EXPORT2
utrie_serialize(UNewTrie *trie, void *dt, int32_t capacity,
                UNewTrieGetFoldedValue *getFoldedValue,
                UBool reduceTo16Bits,
                UErrorCode *pErrorCode) {
    UTrieHeader *header;
    uint32_t *p;
    uint16_t *dest16;
    int32_t i, length;
    uint8_t* data = NULL;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(trie==NULL || capacity<0 || (capacity>0 && dt==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(getFoldedValue==NULL) {
        getFoldedValue=defaultGetFoldedValue;
    }

    data = (uint8_t*)dt;
    
    if(!trie->isCompacted) {
        
        utrie_compact(trie, FALSE, pErrorCode);

        
        utrie_fold(trie, getFoldedValue, pErrorCode);

        
        utrie_compact(trie, TRUE, pErrorCode);

        trie->isCompacted=TRUE;
        if(U_FAILURE(*pErrorCode)) {
            return 0;
        }
    }

    
    if( (reduceTo16Bits ? (trie->dataLength+trie->indexLength) : trie->dataLength) >= UTRIE_MAX_DATA_LENGTH) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
    }

    length=sizeof(UTrieHeader)+2*trie->indexLength;
    if(reduceTo16Bits) {
        length+=2*trie->dataLength;
    } else {
        length+=4*trie->dataLength;
    }

    if(length>capacity) {
        return length; 
    }

#ifdef UTRIE_DEBUG
    printf("**UTrieLengths(serialize)** index:%6ld  data:%6ld  serialized:%6ld\n",
           (long)trie->indexLength, (long)trie->dataLength, (long)length);
#endif

    
    header=(UTrieHeader *)data;
    data+=sizeof(UTrieHeader);

    header->signature=0x54726965; 
    header->options=UTRIE_SHIFT | (UTRIE_INDEX_SHIFT<<UTRIE_OPTIONS_INDEX_SHIFT);

    if(!reduceTo16Bits) {
        header->options|=UTRIE_OPTIONS_DATA_IS_32_BIT;
    }
    if(trie->isLatin1Linear) {
        header->options|=UTRIE_OPTIONS_LATIN1_IS_LINEAR;
    }

    header->indexLength=trie->indexLength;
    header->dataLength=trie->dataLength;

    
    if(reduceTo16Bits) {
        
        p=(uint32_t *)trie->index;
        dest16=(uint16_t *)data;
        for(i=trie->indexLength; i>0; --i) {
            *dest16++=(uint16_t)((*p++ + trie->indexLength)>>UTRIE_INDEX_SHIFT);
        }

        
        p=trie->data;
        for(i=trie->dataLength; i>0; --i) {
            *dest16++=(uint16_t)*p++;
        }
    } else {
        
        p=(uint32_t *)trie->index;
        dest16=(uint16_t *)data;
        for(i=trie->indexLength; i>0; --i) {
            *dest16++=(uint16_t)(*p++ >> UTRIE_INDEX_SHIFT);
        }

        
        uprv_memcpy(dest16, trie->data, 4*trie->dataLength);
    }

    return length;
}


U_CAPI int32_t U_EXPORT2
utrie_defaultGetFoldingOffset(uint32_t data) {
    return (int32_t)data;
}

U_CAPI int32_t U_EXPORT2
utrie_unserialize(UTrie *trie, const void *data, int32_t length, UErrorCode *pErrorCode) {
    const UTrieHeader *header;
    const uint16_t *p16;
    uint32_t options;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return -1;
    }

    
    if(length<(int32_t)sizeof(UTrieHeader)) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return -1;
    }

    
    header=(const UTrieHeader *)data;
    if(header->signature!=0x54726965) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return -1;
    }

    
    options=header->options;
    if( (options&UTRIE_OPTIONS_SHIFT_MASK)!=UTRIE_SHIFT ||
        ((options>>UTRIE_OPTIONS_INDEX_SHIFT)&UTRIE_OPTIONS_SHIFT_MASK)!=UTRIE_INDEX_SHIFT
    ) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return -1;
    }
    trie->isLatin1Linear= (UBool)((options&UTRIE_OPTIONS_LATIN1_IS_LINEAR)!=0);

    
    trie->indexLength=header->indexLength;
    trie->dataLength=header->dataLength;

    length-=(int32_t)sizeof(UTrieHeader);

    
    if(length<2*trie->indexLength) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return -1;
    }
    p16=(const uint16_t *)(header+1);
    trie->index=p16;
    p16+=trie->indexLength;
    length-=2*trie->indexLength;

    
    if(options&UTRIE_OPTIONS_DATA_IS_32_BIT) {
        if(length<4*trie->dataLength) {
            *pErrorCode=U_INVALID_FORMAT_ERROR;
            return -1;
        }
        trie->data32=(const uint32_t *)p16;
        trie->initialValue=trie->data32[0];
        length=(int32_t)sizeof(UTrieHeader)+2*trie->indexLength+4*trie->dataLength;
    } else {
        if(length<2*trie->dataLength) {
            *pErrorCode=U_INVALID_FORMAT_ERROR;
            return -1;
        }

        
        trie->data32=NULL;
        trie->initialValue=trie->index[trie->indexLength];
        length=(int32_t)sizeof(UTrieHeader)+2*trie->indexLength+2*trie->dataLength;
    }

    trie->getFoldingOffset=utrie_defaultGetFoldingOffset;

    return length;
}

U_CAPI int32_t U_EXPORT2
utrie_unserializeDummy(UTrie *trie,
                       void *data, int32_t length,
                       uint32_t initialValue, uint32_t leadUnitValue,
                       UBool make16BitTrie,
                       UErrorCode *pErrorCode) {
    uint16_t *p16;
    int32_t actualLength, latin1Length, i, limit;
    uint16_t block;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return -1;
    }

    

    
    latin1Length= 256; 

    trie->indexLength=UTRIE_BMP_INDEX_LENGTH+UTRIE_SURROGATE_BLOCK_COUNT;
    trie->dataLength=latin1Length;
    if(leadUnitValue!=initialValue) {
        trie->dataLength+=UTRIE_DATA_BLOCK_LENGTH;
    }

    actualLength=trie->indexLength*2;
    if(make16BitTrie) {
        actualLength+=trie->dataLength*2;
    } else {
        actualLength+=trie->dataLength*4;
    }

    
    if(length<actualLength) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        return actualLength;
    }

    trie->isLatin1Linear=TRUE;
    trie->initialValue=initialValue;

    
    p16=(uint16_t *)data;
    trie->index=p16;

    if(make16BitTrie) {
        
        block=(uint16_t)(trie->indexLength>>UTRIE_INDEX_SHIFT);
        limit=trie->indexLength;
        for(i=0; i<limit; ++i) {
            p16[i]=block;
        }

        if(leadUnitValue!=initialValue) {
            
            block+=(uint16_t)(latin1Length>>UTRIE_INDEX_SHIFT);
            i=0xd800>>UTRIE_SHIFT;
            limit=0xdc00>>UTRIE_SHIFT;
            for(; i<limit; ++i) {
                p16[i]=block;
            }
        }

        trie->data32=NULL;

        
        p16+=trie->indexLength;
        for(i=0; i<latin1Length; ++i) {
            p16[i]=(uint16_t)initialValue;
        }

        
        if(leadUnitValue!=initialValue) {
            limit=latin1Length+UTRIE_DATA_BLOCK_LENGTH;
            for(; i<limit; ++i) {
                p16[i]=(uint16_t)leadUnitValue;
            }
        }
    } else {
        uint32_t *p32;

        
        uprv_memset(p16, 0, trie->indexLength*2);

        if(leadUnitValue!=initialValue) {
            
            block=(uint16_t)(latin1Length>>UTRIE_INDEX_SHIFT);
            i=0xd800>>UTRIE_SHIFT;
            limit=0xdc00>>UTRIE_SHIFT;
            for(; i<limit; ++i) {
                p16[i]=block;
            }
        }

        trie->data32=p32=(uint32_t *)(p16+trie->indexLength);

        
        for(i=0; i<latin1Length; ++i) {
            p32[i]=initialValue;
        }

        
        if(leadUnitValue!=initialValue) {
            limit=latin1Length+UTRIE_DATA_BLOCK_LENGTH;
            for(; i<limit; ++i) {
                p32[i]=leadUnitValue;
            }
        }
    }

    trie->getFoldingOffset=utrie_defaultGetFoldingOffset;

    return actualLength;
}




static uint32_t U_CALLCONV
enumSameValue(const void * , uint32_t value) {
    return value;
}





U_CAPI void U_EXPORT2
utrie_enum(const UTrie *trie,
           UTrieEnumValue *enumValue, UTrieEnumRange *enumRange, const void *context) {
    const uint32_t *data32;
    const uint16_t *idx;

    uint32_t value, prevValue, initialValue;
    UChar32 c, prev;
    int32_t l, i, j, block, prevBlock, nullBlock, offset;

    
    if(trie==NULL || trie->index==NULL || enumRange==NULL) {
        return;
    }
    if(enumValue==NULL) {
        enumValue=enumSameValue;
    }

    idx=trie->index;
    data32=trie->data32;

    
    initialValue=enumValue(context, trie->initialValue);

    if(data32==NULL) {
        nullBlock=trie->indexLength;
    } else {
        nullBlock=0;
    }

    
    prevBlock=nullBlock;
    prev=0;
    prevValue=initialValue;

    
    for(i=0, c=0; c<=0xffff; ++i) {
        if(c==0xd800) {
            
            i=UTRIE_BMP_INDEX_LENGTH;
        } else if(c==0xdc00) {
            
            i=c>>UTRIE_SHIFT;
        }

        block=idx[i]<<UTRIE_INDEX_SHIFT;
        if(block==prevBlock) {
            
            c+=UTRIE_DATA_BLOCK_LENGTH;
        } else if(block==nullBlock) {
            
            if(prevValue!=initialValue) {
                if(prev<c) {
                    if(!enumRange(context, prev, c, prevValue)) {
                        return;
                    }
                }
                prevBlock=nullBlock;
                prev=c;
                prevValue=initialValue;
            }
            c+=UTRIE_DATA_BLOCK_LENGTH;
        } else {
            prevBlock=block;
            for(j=0; j<UTRIE_DATA_BLOCK_LENGTH; ++j) {
                value=enumValue(context, data32!=NULL ? data32[block+j] : idx[block+j]);
                if(value!=prevValue) {
                    if(prev<c) {
                        if(!enumRange(context, prev, c, prevValue)) {
                            return;
                        }
                    }
                    if(j>0) {
                        
                        prevBlock=-1;
                    }
                    prev=c;
                    prevValue=value;
                }
                ++c;
            }
        }
    }

    
    for(l=0xd800; l<0xdc00;) {
        
        offset=idx[l>>UTRIE_SHIFT]<<UTRIE_INDEX_SHIFT;
        if(offset==nullBlock) {
            
            if(prevValue!=initialValue) {
                if(prev<c) {
                    if(!enumRange(context, prev, c, prevValue)) {
                        return;
                    }
                }
                prevBlock=nullBlock;
                prev=c;
                prevValue=initialValue;
            }

            l+=UTRIE_DATA_BLOCK_LENGTH;
            c+=UTRIE_DATA_BLOCK_LENGTH<<10;
            continue;
        }

        value= data32!=NULL ? data32[offset+(l&UTRIE_MASK)] : idx[offset+(l&UTRIE_MASK)];

        
        offset=trie->getFoldingOffset(value);
        if(offset<=0) {
            
            if(prevValue!=initialValue) {
                if(prev<c) {
                    if(!enumRange(context, prev, c, prevValue)) {
                        return;
                    }
                }
                prevBlock=nullBlock;
                prev=c;
                prevValue=initialValue;
            }

            
            c+=0x400;
        } else {
            
            i=offset;
            offset+=UTRIE_SURROGATE_BLOCK_COUNT;
            do {
                
                block=idx[i]<<UTRIE_INDEX_SHIFT;
                if(block==prevBlock) {
                    
                    c+=UTRIE_DATA_BLOCK_LENGTH;
                } else if(block==nullBlock) {
                    
                    if(prevValue!=initialValue) {
                        if(prev<c) {
                            if(!enumRange(context, prev, c, prevValue)) {
                                return;
                            }
                        }
                        prevBlock=nullBlock;
                        prev=c;
                        prevValue=initialValue;
                    }
                    c+=UTRIE_DATA_BLOCK_LENGTH;
                } else {
                    prevBlock=block;
                    for(j=0; j<UTRIE_DATA_BLOCK_LENGTH; ++j) {
                        value=enumValue(context, data32!=NULL ? data32[block+j] : idx[block+j]);
                        if(value!=prevValue) {
                            if(prev<c) {
                                if(!enumRange(context, prev, c, prevValue)) {
                                    return;
                                }
                            }
                            if(j>0) {
                                
                                prevBlock=-1;
                            }
                            prev=c;
                            prevValue=value;
                        }
                        ++c;
                    }
                }
            } while(++i<offset);
        }

        ++l;
    }

    
    enumRange(context, prev, c, prevValue);
}
