























#ifdef UTRIE2_DEBUG
#   include <stdio.h>
#endif

#include "unicode/utypes.h"
#include "unicode/utf.h"
#include "unicode/utf8.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "utrie2.h"
#include "utrie2_impl.h"
#include "uassert.h"



static uint32_t
get32(const UNewTrie2 *trie, UChar32 c, UBool fromLSCP) {
    int32_t i2, block;

    if(c>=trie->highStart && (!U_IS_LEAD(c) || fromLSCP)) {
        return trie->data[trie->dataLength-UTRIE2_DATA_GRANULARITY];
    }

    if(U_IS_LEAD(c) && fromLSCP) {
        i2=(UTRIE2_LSCP_INDEX_2_OFFSET-(0xd800>>UTRIE2_SHIFT_2))+
            (c>>UTRIE2_SHIFT_2);
    } else {
        i2=trie->index1[c>>UTRIE2_SHIFT_1]+
            ((c>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK);
    }
    block=trie->index2[i2];
    return trie->data[block+(c&UTRIE2_DATA_MASK)];
}

U_CAPI uint32_t U_EXPORT2
utrie2_get32(const UTrie2 *trie, UChar32 c) {
    if(trie->data16!=NULL) {
        return UTRIE2_GET16(trie, c);
    } else if(trie->data32!=NULL) {
        return UTRIE2_GET32(trie, c);
    } else if((uint32_t)c>0x10ffff) {
        return trie->errorValue;
    } else {
        return get32(trie->newTrie, c, TRUE);
    }
}

U_CAPI uint32_t U_EXPORT2
utrie2_get32FromLeadSurrogateCodeUnit(const UTrie2 *trie, UChar32 c) {
    if(!U_IS_LEAD(c)) {
        return trie->errorValue;
    }
    if(trie->data16!=NULL) {
        return UTRIE2_GET16_FROM_U16_SINGLE_LEAD(trie, c);
    } else if(trie->data32!=NULL) {
        return UTRIE2_GET32_FROM_U16_SINGLE_LEAD(trie, c);
    } else {
        return get32(trie->newTrie, c, FALSE);
    }
}

static inline int32_t
u8Index(const UTrie2 *trie, UChar32 c, int32_t i) {
    int32_t idx=
        _UTRIE2_INDEX_FROM_CP(
            trie,
            trie->data32==NULL ? trie->indexLength : 0,
            c);
    return (idx<<3)|i;
}

U_CAPI int32_t U_EXPORT2
utrie2_internalU8NextIndex(const UTrie2 *trie, UChar32 c,
                           const uint8_t *src, const uint8_t *limit) {
    int32_t i, length;
    i=0;
    
    if((limit-src)<=7) {
        length=(int32_t)(limit-src);
    } else {
        length=7;
    }
    c=utf8_nextCharSafeBody(src, &i, length, c, -1);
    return u8Index(trie, c, i);
}

U_CAPI int32_t U_EXPORT2
utrie2_internalU8PrevIndex(const UTrie2 *trie, UChar32 c,
                           const uint8_t *start, const uint8_t *src) {
    int32_t i, length;
    
    if((src-start)<=7) {
        i=length=(int32_t)(src-start);
    } else {
        i=length=7;
        start=src-7;
    }
    c=utf8_prevCharSafeBody(start, 0, &i, c, -1);
    i=length-i;  
    return u8Index(trie, c, i);
}

U_CAPI UTrie2 * U_EXPORT2
utrie2_openFromSerialized(UTrie2ValueBits valueBits,
                          const void *data, int32_t length, int32_t *pActualLength,
                          UErrorCode *pErrorCode) {
    const UTrie2Header *header;
    const uint16_t *p16;
    int32_t actualLength;

    UTrie2 tempTrie;
    UTrie2 *trie;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if( length<=0 || (U_POINTER_MASK_LSB(data, 3)!=0) ||
        valueBits<0 || UTRIE2_COUNT_VALUE_BITS<=valueBits
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if(length<(int32_t)sizeof(UTrie2Header)) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    
    header=(const UTrie2Header *)data;
    if(header->signature!=UTRIE2_SIG) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    
    if(valueBits!=(UTrie2ValueBits)(header->options&UTRIE2_OPTIONS_VALUE_BITS_MASK)) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    
    uprv_memset(&tempTrie, 0, sizeof(tempTrie));
    tempTrie.indexLength=header->indexLength;
    tempTrie.dataLength=header->shiftedDataLength<<UTRIE2_INDEX_SHIFT;
    tempTrie.index2NullOffset=header->index2NullOffset;
    tempTrie.dataNullOffset=header->dataNullOffset;

    tempTrie.highStart=header->shiftedHighStart<<UTRIE2_SHIFT_1;
    tempTrie.highValueIndex=tempTrie.dataLength-UTRIE2_DATA_GRANULARITY;
    if(valueBits==UTRIE2_16_VALUE_BITS) {
        tempTrie.highValueIndex+=tempTrie.indexLength;
    }

    
    actualLength=(int32_t)sizeof(UTrie2Header)+tempTrie.indexLength*2;
    if(valueBits==UTRIE2_16_VALUE_BITS) {
        actualLength+=tempTrie.dataLength*2;
    } else {
        actualLength+=tempTrie.dataLength*4;
    }
    if(length<actualLength) {
        *pErrorCode=U_INVALID_FORMAT_ERROR;  
        return 0;
    }

    
    trie=(UTrie2 *)uprv_malloc(sizeof(UTrie2));
    if(trie==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    uprv_memcpy(trie, &tempTrie, sizeof(tempTrie));
    trie->memory=(uint32_t *)data;
    trie->length=actualLength;
    trie->isMemoryOwned=FALSE;

    
    p16=(const uint16_t *)(header+1);
    trie->index=p16;
    p16+=trie->indexLength;

    
    switch(valueBits) {
    case UTRIE2_16_VALUE_BITS:
        trie->data16=p16;
        trie->data32=NULL;
        trie->initialValue=trie->index[trie->dataNullOffset];
        trie->errorValue=trie->data16[UTRIE2_BAD_UTF8_DATA_OFFSET];
        break;
    case UTRIE2_32_VALUE_BITS:
        trie->data16=NULL;
        trie->data32=(const uint32_t *)p16;
        trie->initialValue=trie->data32[trie->dataNullOffset];
        trie->errorValue=trie->data32[UTRIE2_BAD_UTF8_DATA_OFFSET];
        break;
    default:
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    if(pActualLength!=NULL) {
        *pActualLength=actualLength;
    }
    return trie;
}

U_CAPI UTrie2 * U_EXPORT2
utrie2_openDummy(UTrie2ValueBits valueBits,
                 uint32_t initialValue, uint32_t errorValue,
                 UErrorCode *pErrorCode) {
    UTrie2 *trie;
    UTrie2Header *header;
    uint32_t *p;
    uint16_t *dest16;
    int32_t indexLength, dataLength, length, i;
    int32_t dataMove;  

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(valueBits<0 || UTRIE2_COUNT_VALUE_BITS<=valueBits) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    indexLength=UTRIE2_INDEX_1_OFFSET;
    dataLength=UTRIE2_DATA_START_OFFSET+UTRIE2_DATA_GRANULARITY;
    length=(int32_t)sizeof(UTrie2Header)+indexLength*2;
    if(valueBits==UTRIE2_16_VALUE_BITS) {
        length+=dataLength*2;
    } else {
        length+=dataLength*4;
    }

    
    trie=(UTrie2 *)uprv_malloc(sizeof(UTrie2));
    if(trie==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    uprv_memset(trie, 0, sizeof(UTrie2));
    trie->memory=uprv_malloc(length);
    if(trie->memory==NULL) {
        uprv_free(trie);
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    trie->length=length;
    trie->isMemoryOwned=TRUE;

    
    if(valueBits==UTRIE2_16_VALUE_BITS) {
        dataMove=indexLength;
    } else {
        dataMove=0;
    }

    trie->indexLength=indexLength;
    trie->dataLength=dataLength;
    trie->index2NullOffset=UTRIE2_INDEX_2_OFFSET;
    trie->dataNullOffset=(uint16_t)dataMove;
    trie->initialValue=initialValue;
    trie->errorValue=errorValue;
    trie->highStart=0;
    trie->highValueIndex=dataMove+UTRIE2_DATA_START_OFFSET;

    
    header=(UTrie2Header *)trie->memory;

    header->signature=UTRIE2_SIG; 
    header->options=(uint16_t)valueBits;

    header->indexLength=(uint16_t)indexLength;
    header->shiftedDataLength=(uint16_t)(dataLength>>UTRIE2_INDEX_SHIFT);
    header->index2NullOffset=(uint16_t)UTRIE2_INDEX_2_OFFSET;
    header->dataNullOffset=(uint16_t)dataMove;
    header->shiftedHighStart=0;

    
    dest16=(uint16_t *)(header+1);
    trie->index=dest16;

    
    for(i=0; i<UTRIE2_INDEX_2_BMP_LENGTH; ++i) {
        *dest16++=(uint16_t)(dataMove>>UTRIE2_INDEX_SHIFT);  
    }

    
    for(i=0; i<(0xc2-0xc0); ++i) {                                  
        *dest16++=(uint16_t)(dataMove+UTRIE2_BAD_UTF8_DATA_OFFSET);
    }
    for(; i<(0xe0-0xc0); ++i) {                                     
        *dest16++=(uint16_t)dataMove;
    }

    
    switch(valueBits) {
    case UTRIE2_16_VALUE_BITS:
        
        trie->data16=dest16;
        trie->data32=NULL;
        for(i=0; i<0x80; ++i) {
            *dest16++=(uint16_t)initialValue;
        }
        for(; i<0xc0; ++i) {
            *dest16++=(uint16_t)errorValue;
        }
        
        for(i=0; i<UTRIE2_DATA_GRANULARITY; ++i) {
            *dest16++=(uint16_t)initialValue;
        }
        break;
    case UTRIE2_32_VALUE_BITS:
        
        p=(uint32_t *)dest16;
        trie->data16=NULL;
        trie->data32=p;
        for(i=0; i<0x80; ++i) {
            *p++=initialValue;
        }
        for(; i<0xc0; ++i) {
            *p++=errorValue;
        }
        
        for(i=0; i<UTRIE2_DATA_GRANULARITY; ++i) {
            *p++=initialValue;
        }
        break;
    default:
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    return trie;
}

U_CAPI void U_EXPORT2
utrie2_close(UTrie2 *trie) {
    if(trie!=NULL) {
        if(trie->isMemoryOwned) {
            uprv_free(trie->memory);
        }
        if(trie->newTrie!=NULL) {
            uprv_free(trie->newTrie->data);
            uprv_free(trie->newTrie);
        }
        uprv_free(trie);
    }
}

U_CAPI int32_t U_EXPORT2
utrie2_getVersion(const void *data, int32_t length, UBool anyEndianOk) {
    uint32_t signature;
    if(length<16 || data==NULL || (U_POINTER_MASK_LSB(data, 3)!=0)) {
        return 0;
    }
    signature=*(const uint32_t *)data;
    if(signature==UTRIE2_SIG) {
        return 2;
    }
    if(anyEndianOk && signature==UTRIE2_OE_SIG) {
        return 2;
    }
    if(signature==UTRIE_SIG) {
        return 1;
    }
    if(anyEndianOk && signature==UTRIE_OE_SIG) {
        return 1;
    }
    return 0;
}

U_CAPI UBool U_EXPORT2
utrie2_isFrozen(const UTrie2 *trie) {
    return (UBool)(trie->newTrie==NULL);
}

U_CAPI int32_t U_EXPORT2
utrie2_serialize(const UTrie2 *trie,
                 void *data, int32_t capacity,
                 UErrorCode *pErrorCode) {
    
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if( trie==NULL || trie->memory==NULL || trie->newTrie!=NULL ||
        capacity<0 || (capacity>0 && (data==NULL || (U_POINTER_MASK_LSB(data, 3)!=0)))
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(capacity>=trie->length) {
        uprv_memcpy(data, trie->memory, trie->length);
    } else {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }
    return trie->length;
}

U_CAPI int32_t U_EXPORT2
utrie2_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode) {
    const UTrie2Header *inTrie;
    UTrie2Header trie;
    int32_t dataLength, size;
    UTrie2ValueBits valueBits;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || (length>=0 && outData==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if(length>=0 && length<(int32_t)sizeof(UTrie2Header)) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    inTrie=(const UTrie2Header *)inData;
    trie.signature=ds->readUInt32(inTrie->signature);
    trie.options=ds->readUInt16(inTrie->options);
    trie.indexLength=ds->readUInt16(inTrie->indexLength);
    trie.shiftedDataLength=ds->readUInt16(inTrie->shiftedDataLength);

    valueBits=(UTrie2ValueBits)(trie.options&UTRIE2_OPTIONS_VALUE_BITS_MASK);
    dataLength=(int32_t)trie.shiftedDataLength<<UTRIE2_INDEX_SHIFT;

    if( trie.signature!=UTRIE2_SIG ||
        valueBits<0 || UTRIE2_COUNT_VALUE_BITS<=valueBits ||
        trie.indexLength<UTRIE2_INDEX_1_OFFSET ||
        dataLength<UTRIE2_DATA_START_OFFSET
    ) {
        *pErrorCode=U_INVALID_FORMAT_ERROR; 
        return 0;
    }

    size=sizeof(UTrie2Header)+trie.indexLength*2;
    switch(valueBits) {
    case UTRIE2_16_VALUE_BITS:
        size+=dataLength*2;
        break;
    case UTRIE2_32_VALUE_BITS:
        size+=dataLength*4;
        break;
    default:
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    if(length>=0) {
        UTrie2Header *outTrie;

        if(length<size) {
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        outTrie=(UTrie2Header *)outData;

        
        ds->swapArray32(ds, &inTrie->signature, 4, &outTrie->signature, pErrorCode);
        ds->swapArray16(ds, &inTrie->options, 12, &outTrie->options, pErrorCode);

        
        switch(valueBits) {
        case UTRIE2_16_VALUE_BITS:
            ds->swapArray16(ds, inTrie+1, (trie.indexLength+dataLength)*2, outTrie+1, pErrorCode);
            break;
        case UTRIE2_32_VALUE_BITS:
            ds->swapArray16(ds, inTrie+1, trie.indexLength*2, outTrie+1, pErrorCode);
            ds->swapArray32(ds, (const uint16_t *)(inTrie+1)+trie.indexLength, dataLength*4,
                                     (uint16_t *)(outTrie+1)+trie.indexLength, pErrorCode);
            break;
        default:
            *pErrorCode=U_INVALID_FORMAT_ERROR;
            return 0;
        }
    }

    return size;
}






#define MIN_VALUE(a, b) ((a)<(b) ? (a) : (b))


static uint32_t U_CALLCONV
enumSameValue(const void * , uint32_t value) {
    return value;
}














static void
enumEitherTrie(const UTrie2 *trie,
               UChar32 start, UChar32 limit,
               UTrie2EnumValue *enumValue, UTrie2EnumRange *enumRange, const void *context) {
    const uint32_t *data32;
    const uint16_t *idx;

    uint32_t value, prevValue, initialValue;
    UChar32 c, prev, highStart;
    int32_t j, i2Block, prevI2Block, index2NullOffset, block, prevBlock, nullBlock;

    if(enumRange==NULL) {
        return;
    }
    if(enumValue==NULL) {
        enumValue=enumSameValue;
    }

    if(trie->newTrie==NULL) {
        
        idx=trie->index;
        U_ASSERT(idx!=NULL); 
        data32=trie->data32;

        index2NullOffset=trie->index2NullOffset;
        nullBlock=trie->dataNullOffset;
    } else {
        
        idx=NULL;
        data32=trie->newTrie->data;
        U_ASSERT(data32!=NULL); 

        index2NullOffset=trie->newTrie->index2NullOffset;
        nullBlock=trie->newTrie->dataNullOffset;
    }

    highStart=trie->highStart;

    
    initialValue=enumValue(context, trie->initialValue);

    
    prevI2Block=-1;
    prevBlock=-1;
    prev=start;
    prevValue=0;

    
    for(c=start; c<limit && c<highStart;) {
        
        UChar32 tempLimit=c+UTRIE2_CP_PER_INDEX_1_ENTRY;
        if(limit<tempLimit) {
            tempLimit=limit;
        }
        if(c<=0xffff) {
            if(!U_IS_SURROGATE(c)) {
                i2Block=c>>UTRIE2_SHIFT_2;
            } else if(U_IS_SURROGATE_LEAD(c)) {
                



                i2Block=UTRIE2_LSCP_INDEX_2_OFFSET;
                tempLimit=MIN_VALUE(0xdc00, limit);
            } else {
                



                i2Block=0xd800>>UTRIE2_SHIFT_2;
                tempLimit=MIN_VALUE(0xe000, limit);
            }
        } else {
            
            if(idx!=NULL) {
                i2Block=idx[(UTRIE2_INDEX_1_OFFSET-UTRIE2_OMITTED_BMP_INDEX_1_LENGTH)+
                              (c>>UTRIE2_SHIFT_1)];
            } else {
                i2Block=trie->newTrie->index1[c>>UTRIE2_SHIFT_1];
            }
            if(i2Block==prevI2Block && (c-prev)>=UTRIE2_CP_PER_INDEX_1_ENTRY) {
                




                c+=UTRIE2_CP_PER_INDEX_1_ENTRY;
                continue;
            }
        }
        prevI2Block=i2Block;
        if(i2Block==index2NullOffset) {
            
            if(prevValue!=initialValue) {
                if(prev<c && !enumRange(context, prev, c-1, prevValue)) {
                    return;
                }
                prevBlock=nullBlock;
                prev=c;
                prevValue=initialValue;
            }
            c+=UTRIE2_CP_PER_INDEX_1_ENTRY;
        } else {
            
            int32_t i2, i2Limit;
            i2=(c>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK;
            if((c>>UTRIE2_SHIFT_1)==(tempLimit>>UTRIE2_SHIFT_1)) {
                i2Limit=(tempLimit>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK;
            } else {
                i2Limit=UTRIE2_INDEX_2_BLOCK_LENGTH;
            }
            for(; i2<i2Limit; ++i2) {
                if(idx!=NULL) {
                    block=(int32_t)idx[i2Block+i2]<<UTRIE2_INDEX_SHIFT;
                } else {
                    block=trie->newTrie->index2[i2Block+i2];
                }
                if(block==prevBlock && (c-prev)>=UTRIE2_DATA_BLOCK_LENGTH) {
                    
                    c+=UTRIE2_DATA_BLOCK_LENGTH;
                    continue;
                }
                prevBlock=block;
                if(block==nullBlock) {
                    
                    if(prevValue!=initialValue) {
                        if(prev<c && !enumRange(context, prev, c-1, prevValue)) {
                            return;
                        }
                        prev=c;
                        prevValue=initialValue;
                    }
                    c+=UTRIE2_DATA_BLOCK_LENGTH;
                } else {
                    for(j=0; j<UTRIE2_DATA_BLOCK_LENGTH; ++j) {
                        value=enumValue(context, data32!=NULL ? data32[block+j] : idx[block+j]);
                        if(value!=prevValue) {
                            if(prev<c && !enumRange(context, prev, c-1, prevValue)) {
                                return;
                            }
                            prev=c;
                            prevValue=value;
                        }
                        ++c;
                    }
                }
            }
        }
    }

    if(c>limit) {
        c=limit;  
    } else if(c<limit) {
        
        uint32_t highValue;
        if(idx!=NULL) {
            highValue=
                data32!=NULL ?
                    data32[trie->highValueIndex] :
                    idx[trie->highValueIndex];
        } else {
            highValue=trie->newTrie->data[trie->newTrie->dataLength-UTRIE2_DATA_GRANULARITY];
        }
        value=enumValue(context, highValue);
        if(value!=prevValue) {
            if(prev<c && !enumRange(context, prev, c-1, prevValue)) {
                return;
            }
            prev=c;
            prevValue=value;
        }
        c=limit;
    }

    
    enumRange(context, prev, c-1, prevValue);
}

U_CAPI void U_EXPORT2
utrie2_enum(const UTrie2 *trie,
            UTrie2EnumValue *enumValue, UTrie2EnumRange *enumRange, const void *context) {
    enumEitherTrie(trie, 0, 0x110000, enumValue, enumRange, context);
}

U_CAPI void U_EXPORT2
utrie2_enumForLeadSurrogate(const UTrie2 *trie, UChar32 lead,
                            UTrie2EnumValue *enumValue, UTrie2EnumRange *enumRange,
                            const void *context) {
    if(!U16_IS_LEAD(lead)) {
        return;
    }
    lead=(lead-0xd7c0)<<10;   
    enumEitherTrie(trie, lead, lead+0x400, enumValue, enumRange, context);
}



U_NAMESPACE_BEGIN

uint16_t BackwardUTrie2StringIterator::previous16() {
    codePointLimit=codePointStart;
    if(start>=codePointStart) {
        codePoint=U_SENTINEL;
        return 0;
    }
    uint16_t result;
    UTRIE2_U16_PREV16(trie, start, codePointStart, codePoint, result);
    return result;
}

uint16_t ForwardUTrie2StringIterator::next16() {
    codePointStart=codePointLimit;
    if(codePointLimit==limit) {
        codePoint=U_SENTINEL;
        return 0;
    }
    uint16_t result;
    UTRIE2_U16_NEXT16(trie, codePointLimit, limit, codePoint, result);
    return result;
}

U_NAMESPACE_END
