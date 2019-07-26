















#ifndef __UTRIE_H__
#define __UTRIE_H__

#include "unicode/utypes.h"
#include "unicode/utf16.h"
#include "udataswp.h"

U_CDECL_BEGIN



























enum {
    
    UTRIE_SHIFT=5,

    
    UTRIE_DATA_BLOCK_LENGTH=1<<UTRIE_SHIFT,

    
    UTRIE_MASK=UTRIE_DATA_BLOCK_LENGTH-1,

    



    UTRIE_LEAD_INDEX_DISP=0x2800>>UTRIE_SHIFT,

    






    UTRIE_INDEX_SHIFT=2,

    
    UTRIE_DATA_GRANULARITY=1<<UTRIE_INDEX_SHIFT,

    
    UTRIE_SURROGATE_BLOCK_BITS=10-UTRIE_SHIFT,

    




    UTRIE_SURROGATE_BLOCK_COUNT=(1<<UTRIE_SURROGATE_BLOCK_BITS),

    
    UTRIE_BMP_INDEX_LENGTH=0x10000>>UTRIE_SHIFT
};





#define UTRIE_MAX_INDEX_LENGTH (0x110000>>UTRIE_SHIFT)





#define UTRIE_MAX_DATA_LENGTH (0x10000<<UTRIE_INDEX_SHIFT)







#define UTRIE_MAX_BUILD_TIME_DATA_LENGTH (0x110000+UTRIE_DATA_BLOCK_LENGTH+0x400)
















#define UTRIE_DUMMY_SIZE ((UTRIE_BMP_INDEX_LENGTH+UTRIE_SURROGATE_BLOCK_COUNT)*2+(UTRIE_SHIFT<=8?256:UTRIE_DATA_BLOCK_LENGTH)*4+UTRIE_DATA_BLOCK_LENGTH*4)









typedef int32_t U_CALLCONV
UTrieGetFoldingOffset(uint32_t data);










struct UTrie {
    const uint16_t *index;
    const uint32_t *data32; 

    









    UTrieGetFoldingOffset *getFoldingOffset;

    int32_t indexLength, dataLength;
    uint32_t initialValue;
    UBool isLatin1Linear;
};

#ifndef __UTRIE2_H__
typedef struct UTrie UTrie;
#endif


#define _UTRIE_GET_RAW(trie, data, offset, c16) \
    (trie)->data[ \
        ((int32_t)((trie)->index[(offset)+((c16)>>UTRIE_SHIFT)])<<UTRIE_INDEX_SHIFT)+ \
        ((c16)&UTRIE_MASK) \
    ]


#define _UTRIE_GET_FROM_PAIR(trie, data, c, c2, result, resultType) { \
    int32_t __offset; \
\
    /* get data for lead surrogate */ \
    (result)=_UTRIE_GET_RAW((trie), data, 0, (c)); \
    __offset=(trie)->getFoldingOffset(result); \
\
    /* get the real data from the folded lead/trail units */ \
    if(__offset>0) { \
        (result)=_UTRIE_GET_RAW((trie), data, __offset, (c2)&0x3ff); \
    } else { \
        (result)=(resultType)((trie)->initialValue); \
    } \
}


#define _UTRIE_GET_FROM_BMP(trie, data, c16) \
    _UTRIE_GET_RAW(trie, data, 0xd800<=(c16) && (c16)<=0xdbff ? UTRIE_LEAD_INDEX_DISP : 0, c16);






#define _UTRIE_GET(trie, data, c32, result, resultType) \
    if((uint32_t)(c32)<=0xffff) { \
        /* BMP code points */ \
        (result)=_UTRIE_GET_FROM_BMP(trie, data, c32); \
    } else if((uint32_t)(c32)<=0x10ffff) { \
        /* supplementary code point */ \
        UChar __lead16=U16_LEAD(c32); \
        _UTRIE_GET_FROM_PAIR(trie, data, __lead16, c32, result, resultType); \
    } else { \
        /* out of range */ \
        (result)=(resultType)((trie)->initialValue); \
    }


#define _UTRIE_NEXT(trie, data, src, limit, c, c2, result, resultType) { \
    (c)=*(src)++; \
    if(!U16_IS_LEAD(c)) { \
        (c2)=0; \
        (result)=_UTRIE_GET_RAW((trie), data, 0, (c)); \
    } else if((src)!=(limit) && U16_IS_TRAIL((c2)=*(src))) { \
        ++(src); \
        _UTRIE_GET_FROM_PAIR((trie), data, (c), (c2), (result), resultType); \
    } else { \
        /* unpaired lead surrogate code point */ \
        (c2)=0; \
        (result)=_UTRIE_GET_RAW((trie), data, UTRIE_LEAD_INDEX_DISP, (c)); \
    } \
}


#define _UTRIE_PREVIOUS(trie, data, start, src, c, c2, result, resultType) { \
    (c)=*--(src); \
    if(!U16_IS_SURROGATE(c)) { \
        (c2)=0; \
        (result)=_UTRIE_GET_RAW((trie), data, 0, (c)); \
    } else if(!U16_IS_SURROGATE_LEAD(c)) { \
        /* trail surrogate */ \
        if((start)!=(src) && U16_IS_LEAD((c2)=*((src)-1))) { \
            --(src); \
            (result)=(c); (c)=(c2); (c2)=(UChar)(result); /* swap c, c2 */ \
            _UTRIE_GET_FROM_PAIR((trie), data, (c), (c2), (result), resultType); \
        } else { \
            /* unpaired trail surrogate code point */ \
            (c2)=0; \
            (result)=_UTRIE_GET_RAW((trie), data, 0, (c)); \
        } \
    } else { \
        /* unpaired lead surrogate code point */ \
        (c2)=0; \
        (result)=_UTRIE_GET_RAW((trie), data, UTRIE_LEAD_INDEX_DISP, (c)); \
    } \
}












#define UTRIE_GET16_LATIN1(trie) ((trie)->index+(trie)->indexLength+UTRIE_DATA_BLOCK_LENGTH)










#define UTRIE_GET32_LATIN1(trie) ((trie)->data32+UTRIE_DATA_BLOCK_LENGTH)









#define UTRIE_GET16_FROM_LEAD(trie, c16) _UTRIE_GET_RAW(trie, index, 0, c16)









#define UTRIE_GET32_FROM_LEAD(trie, c16) _UTRIE_GET_RAW(trie, data32, 0, c16)










#define UTRIE_GET16_FROM_BMP(trie, c16) _UTRIE_GET_FROM_BMP(trie, index, c16)










#define UTRIE_GET32_FROM_BMP(trie, c16) _UTRIE_GET_FROM_BMP(trie, data32, c16)










#define UTRIE_GET16(trie, c32, result) _UTRIE_GET(trie, index, c32, result, uint16_t)










#define UTRIE_GET32(trie, c32, result) _UTRIE_GET(trie, data32, c32, result, uint32_t)












#define UTRIE_NEXT16(trie, src, limit, c, c2, result) _UTRIE_NEXT(trie, index, src, limit, c, c2, result, uint16_t)












#define UTRIE_NEXT32(trie, src, limit, c, c2, result) _UTRIE_NEXT(trie, data32, src, limit, c, c2, result, uint32_t)












#define UTRIE_PREVIOUS16(trie, start, src, c, c2, result) _UTRIE_PREVIOUS(trie, index, start, src, c, c2, result, uint16_t)












#define UTRIE_PREVIOUS32(trie, start, src, c, c2, result) _UTRIE_PREVIOUS(trie, data32, start, src, c, c2, result, uint32_t)









#define UTRIE_GET16_FROM_PAIR(trie, c, c2, result) _UTRIE_GET_FROM_PAIR(trie, index, c, c2, result, uint16_t)









#define UTRIE_GET32_FROM_PAIR(trie, c, c2, result) _UTRIE_GET_FROM_PAIR(trie, data32, c, c2, result, uint32_t)










#define UTRIE_GET16_FROM_OFFSET_TRAIL(trie, offset, c2) _UTRIE_GET_RAW(trie, index, offset, (c2)&0x3ff)










#define UTRIE_GET32_FROM_OFFSET_TRAIL(trie, offset, c2) _UTRIE_GET_RAW(trie, data32, offset, (c2)&0x3ff)











typedef uint32_t U_CALLCONV
UTrieEnumValue(const void *context, uint32_t value);














typedef UBool U_CALLCONV
UTrieEnumRange(const void *context, UChar32 start, UChar32 limit, uint32_t value);

















U_CAPI void U_EXPORT2
utrie_enum(const UTrie *trie,
           UTrieEnumValue *enumValue, UTrieEnumRange *enumRange, const void *context);












U_CAPI int32_t U_EXPORT2
utrie_unserialize(UTrie *trie, const void *data, int32_t length, UErrorCode *pErrorCode);





















U_CAPI int32_t U_EXPORT2
utrie_unserializeDummy(UTrie *trie,
                       void *data, int32_t length,
                       uint32_t initialValue, uint32_t leadUnitValue,
                       UBool make16BitTrie,
                       UErrorCode *pErrorCode);










U_CAPI int32_t U_EXPORT2
utrie_defaultGetFoldingOffset(uint32_t data);








struct UNewTrie {
    



    int32_t index[UTRIE_MAX_INDEX_LENGTH];
    uint32_t *data;

    uint32_t leadUnitValue;
    int32_t indexLength, dataCapacity, dataLength;
    UBool isAllocated, isDataAllocated;
    UBool isLatin1Linear, isCompacted;

    



    int32_t map[UTRIE_MAX_BUILD_TIME_DATA_LENGTH>>UTRIE_SHIFT];
};

typedef struct UNewTrie UNewTrie;



















typedef uint32_t U_CALLCONV
UNewTrieGetFoldedValue(UNewTrie *trie, UChar32 start, int32_t offset);



























U_CAPI UNewTrie * U_EXPORT2
utrie_open(UNewTrie *fillIn,
           uint32_t *aliasData, int32_t maxDataLength,
           uint32_t initialValue, uint32_t leadUnitValue,
           UBool latin1Linear);











U_CAPI UNewTrie * U_EXPORT2
utrie_clone(UNewTrie *fillIn, const UNewTrie *other, uint32_t *aliasData, int32_t aliasDataLength);







U_CAPI void U_EXPORT2
utrie_close(UNewTrie *trie);











U_CAPI uint32_t * U_EXPORT2
utrie_getData(UNewTrie *trie, int32_t *pLength);









U_CAPI UBool U_EXPORT2
utrie_set32(UNewTrie *trie, UChar32 c, uint32_t value);











U_CAPI uint32_t U_EXPORT2
utrie_get32(UNewTrie *trie, UChar32 c, UBool *pInBlockZero);













U_CAPI UBool U_EXPORT2
utrie_setRange32(UNewTrie *trie, UChar32 start, UChar32 limit, uint32_t value, UBool overwrite);



























U_CAPI int32_t U_EXPORT2
utrie_serialize(UNewTrie *trie, void *data, int32_t capacity,
                UNewTrieGetFoldedValue *getFoldedValue,
                UBool reduceTo16Bits,
                UErrorCode *pErrorCode);





U_CAPI int32_t U_EXPORT2
utrie_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode);











typedef struct UTrieHeader {
    
    uint32_t signature;

    






    uint32_t options;

    
    int32_t indexLength;

    
    int32_t dataLength;
} UTrieHeader;





enum {
    
    UTRIE_OPTIONS_SHIFT_MASK=0xf,

    
    UTRIE_OPTIONS_INDEX_SHIFT=4,

    
    UTRIE_OPTIONS_DATA_IS_32_BIT=0x100,

    



    UTRIE_OPTIONS_LATIN1_IS_LINEAR=0x200
};

U_CDECL_END

#endif
