















#ifndef __UTRIE2_H__
#define __UTRIE2_H__

#include "unicode/utypes.h"
#include "putilimp.h"
#include "udataswp.h"

U_CDECL_BEGIN

struct UTrie;  
#ifndef __UTRIE_H__
typedef struct UTrie UTrie;
#endif































struct UTrie2;
typedef struct UTrie2 UTrie2;






enum UTrie2ValueBits {
    
    UTRIE2_16_VALUE_BITS,
    
    UTRIE2_32_VALUE_BITS,
    
    UTRIE2_COUNT_VALUE_BITS
};
typedef enum UTrie2ValueBits UTrie2ValueBits;




















U_CAPI UTrie2 * U_EXPORT2
utrie2_openFromSerialized(UTrie2ValueBits valueBits,
                          const void *data, int32_t length, int32_t *pActualLength,
                          UErrorCode *pErrorCode);






















U_CAPI UTrie2 * U_EXPORT2
utrie2_openDummy(UTrie2ValueBits valueBits,
                 uint32_t initialValue, uint32_t errorValue,
                 UErrorCode *pErrorCode);











U_CAPI uint32_t U_EXPORT2
utrie2_get32(const UTrie2 *trie, UChar32 c);











typedef uint32_t U_CALLCONV
UTrie2EnumValue(const void *context, uint32_t value);














typedef UBool U_CALLCONV
UTrie2EnumRange(const void *context, UChar32 start, UChar32 end, uint32_t value);



















U_CAPI void U_EXPORT2
utrie2_enum(const UTrie2 *trie,
            UTrie2EnumValue *enumValue, UTrie2EnumRange *enumRange, const void *context);














U_CAPI UTrie2 * U_EXPORT2
utrie2_open(uint32_t initialValue, uint32_t errorValue, UErrorCode *pErrorCode);









U_CAPI UTrie2 * U_EXPORT2
utrie2_clone(const UTrie2 *other, UErrorCode *pErrorCode);










U_CAPI UTrie2 * U_EXPORT2
utrie2_cloneAsThawed(const UTrie2 *other, UErrorCode *pErrorCode);






U_CAPI void U_EXPORT2
utrie2_close(UTrie2 *trie);










U_CAPI void U_EXPORT2
utrie2_set32(UTrie2 *trie, UChar32 c, uint32_t value, UErrorCode *pErrorCode);














U_CAPI void U_EXPORT2
utrie2_setRange32(UTrie2 *trie,
                  UChar32 start, UChar32 end,
                  uint32_t value, UBool overwrite,
                  UErrorCode *pErrorCode);




















U_CAPI void U_EXPORT2
utrie2_freeze(UTrie2 *trie, UTrie2ValueBits valueBits, UErrorCode *pErrorCode);








U_CAPI UBool U_EXPORT2
utrie2_isFrozen(const UTrie2 *trie);



















U_CAPI int32_t U_EXPORT2
utrie2_serialize(UTrie2 *trie,
                 void *data, int32_t capacity,
                 UErrorCode *pErrorCode);
















U_CAPI int32_t U_EXPORT2
utrie2_getVersion(const void *data, int32_t length, UBool anyEndianOk);





U_CAPI int32_t U_EXPORT2
utrie2_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode);





U_CAPI int32_t U_EXPORT2
utrie2_swapAnyVersion(const UDataSwapper *ds,
                      const void *inData, int32_t length, void *outData,
                      UErrorCode *pErrorCode);











U_CAPI UTrie2 * U_EXPORT2
utrie2_fromUTrie(const UTrie *trie1, uint32_t errorValue, UErrorCode *pErrorCode);
















#define UTRIE2_GET16(trie, c) _UTRIE2_GET((trie), index, (trie)->indexLength, (c))









#define UTRIE2_GET32(trie, c) _UTRIE2_GET((trie), data32, 0, (c))











#define UTRIE2_U16_NEXT16(trie, src, limit, c, result) _UTRIE2_U16_NEXT(trie, index, src, limit, c, result)











#define UTRIE2_U16_NEXT32(trie, src, limit, c, result) _UTRIE2_U16_NEXT(trie, data32, src, limit, c, result)











#define UTRIE2_U16_PREV16(trie, start, src, c, result) _UTRIE2_U16_PREV(trie, index, start, src, c, result)











#define UTRIE2_U16_PREV32(trie, start, src, c, result) _UTRIE2_U16_PREV(trie, data32, start, src, c, result)









#define UTRIE2_U8_NEXT16(trie, src, limit, result)\
    _UTRIE2_U8_NEXT(trie, data16, index, src, limit, result)









#define UTRIE2_U8_NEXT32(trie, src, limit, result) \
    _UTRIE2_U8_NEXT(trie, data32, data32, src, limit, result)









#define UTRIE2_U8_PREV16(trie, start, src, result) \
    _UTRIE2_U8_PREV(trie, data16, index, start, src, result)









#define UTRIE2_U8_PREV32(trie, start, src, result) \
    _UTRIE2_U8_PREV(trie, data32, data32, start, src, result)






































U_CAPI uint32_t U_EXPORT2
utrie2_get32FromLeadSurrogateCodeUnit(const UTrie2 *trie, UChar32 c);


























U_CAPI void U_EXPORT2
utrie2_enumForLeadSurrogate(const UTrie2 *trie, UChar32 lead,
                            UTrie2EnumValue *enumValue, UTrie2EnumRange *enumRange,
                            const void *context);










U_CAPI void U_EXPORT2
utrie2_set32ForLeadSurrogateCodeUnit(UTrie2 *trie,
                                     UChar32 lead, uint32_t value,
                                     UErrorCode *pErrorCode);










#define UTRIE2_GET16_FROM_U16_SINGLE_LEAD(trie, c) _UTRIE2_GET_FROM_U16_SINGLE_LEAD((trie), index, c)










#define UTRIE2_GET32_FROM_U16_SINGLE_LEAD(trie, c) _UTRIE2_GET_FROM_U16_SINGLE_LEAD((trie), data32, c)








#define UTRIE2_GET16_FROM_SUPP(trie, c) _UTRIE2_GET_FROM_SUPP((trie), index, c)








#define UTRIE2_GET32_FROM_SUPP(trie, c) _UTRIE2_GET_FROM_SUPP((trie), data32, c)

U_CDECL_END



#ifdef __cplusplus

#include "unicode/utf.h"
#include "mutex.h"

U_NAMESPACE_BEGIN


class UTrie2StringIterator : public UMemory {
public:
    UTrie2StringIterator(const UTrie2 *t, const UChar *p) :
        trie(t), codePointStart(p), codePointLimit(p), codePoint(U_SENTINEL) {}

    const UTrie2 *trie;
    const UChar *codePointStart, *codePointLimit;
    UChar32 codePoint;
};

class BackwardUTrie2StringIterator : public UTrie2StringIterator {
public:
    BackwardUTrie2StringIterator(const UTrie2 *t, const UChar *s, const UChar *p) :
        UTrie2StringIterator(t, p), start(s) {}

    uint16_t previous16();

    const UChar *start;
};

class ForwardUTrie2StringIterator : public UTrie2StringIterator {
public:
    
    
    ForwardUTrie2StringIterator(const UTrie2 *t, const UChar *p, const UChar *l) :
        UTrie2StringIterator(t, p), limit(l) {}

    uint16_t next16();

    const UChar *limit;
};

class UTrie2Singleton {
public:
    UTrie2Singleton(SimpleSingleton &s) : singleton(s) {}
    void deleteInstance() {
        utrie2_close((UTrie2 *)singleton.fInstance);
        singleton.reset();
    }
    UTrie2 *getInstance(InstantiatorFn *instantiator, const void *context,
                        UErrorCode &errorCode);
private:
    SimpleSingleton &singleton;
};

U_NAMESPACE_END

#endif



U_CDECL_BEGIN


struct UNewTrie2;
typedef struct UNewTrie2 UNewTrie2;










struct UTrie2 {
    
    const uint16_t *index;
    const uint16_t *data16;     
    const uint32_t *data32;     

    int32_t indexLength, dataLength;
    uint16_t index2NullOffset;  
    uint16_t dataNullOffset;
    uint32_t initialValue;
    
    uint32_t errorValue;

    
    UChar32 highStart;
    int32_t highValueIndex;

    
    void *memory;           
    int32_t length;         
    UBool isMemoryOwned;    
    UBool padding1;
    int16_t padding2;
    UNewTrie2 *newTrie;     
};







enum {
    
    UTRIE2_SHIFT_1=6+5,

    
    UTRIE2_SHIFT_2=5,

    



    UTRIE2_SHIFT_1_2=UTRIE2_SHIFT_1-UTRIE2_SHIFT_2,

    



    UTRIE2_OMITTED_BMP_INDEX_1_LENGTH=0x10000>>UTRIE2_SHIFT_1,

    
    UTRIE2_CP_PER_INDEX_1_ENTRY=1<<UTRIE2_SHIFT_1,

    
    UTRIE2_INDEX_2_BLOCK_LENGTH=1<<UTRIE2_SHIFT_1_2,

    
    UTRIE2_INDEX_2_MASK=UTRIE2_INDEX_2_BLOCK_LENGTH-1,

    
    UTRIE2_DATA_BLOCK_LENGTH=1<<UTRIE2_SHIFT_2,

    
    UTRIE2_DATA_MASK=UTRIE2_DATA_BLOCK_LENGTH-1,

    





    UTRIE2_INDEX_SHIFT=2,

    
    UTRIE2_DATA_GRANULARITY=1<<UTRIE2_INDEX_SHIFT,

    

    



    UTRIE2_INDEX_2_OFFSET=0,

    





    UTRIE2_LSCP_INDEX_2_OFFSET=0x10000>>UTRIE2_SHIFT_2,
    UTRIE2_LSCP_INDEX_2_LENGTH=0x400>>UTRIE2_SHIFT_2,

    
    UTRIE2_INDEX_2_BMP_LENGTH=UTRIE2_LSCP_INDEX_2_OFFSET+UTRIE2_LSCP_INDEX_2_LENGTH,

    



    UTRIE2_UTF8_2B_INDEX_2_OFFSET=UTRIE2_INDEX_2_BMP_LENGTH,
    UTRIE2_UTF8_2B_INDEX_2_LENGTH=0x800>>6,  

    











    UTRIE2_INDEX_1_OFFSET=UTRIE2_UTF8_2B_INDEX_2_OFFSET+UTRIE2_UTF8_2B_INDEX_2_LENGTH,
    UTRIE2_MAX_INDEX_1_LENGTH=0x100000>>UTRIE2_SHIFT_1,

    




    




    UTRIE2_BAD_UTF8_DATA_OFFSET=0x80,

    
    UTRIE2_DATA_START_OFFSET=0xc0
};








U_INTERNAL int32_t U_EXPORT2
utrie2_internalU8NextIndex(const UTrie2 *trie, UChar32 c,
                           const uint8_t *src, const uint8_t *limit);






U_INTERNAL int32_t U_EXPORT2
utrie2_internalU8PrevIndex(const UTrie2 *trie, UChar32 c,
                           const uint8_t *start, const uint8_t *src);



#define _UTRIE2_INDEX_RAW(offset, trieIndex, c) \
    (((int32_t)((trieIndex)[(offset)+((c)>>UTRIE2_SHIFT_2)]) \
    <<UTRIE2_INDEX_SHIFT)+ \
    ((c)&UTRIE2_DATA_MASK))


#define _UTRIE2_INDEX_FROM_U16_SINGLE_LEAD(trieIndex, c) _UTRIE2_INDEX_RAW(0, trieIndex, c)


#define _UTRIE2_INDEX_FROM_LSCP(trieIndex, c) \
    _UTRIE2_INDEX_RAW(UTRIE2_LSCP_INDEX_2_OFFSET-(0xd800>>UTRIE2_SHIFT_2), trieIndex, c)


#define _UTRIE2_INDEX_FROM_BMP(trieIndex, c) \
    _UTRIE2_INDEX_RAW(U_IS_LEAD(c) ? UTRIE2_LSCP_INDEX_2_OFFSET-(0xd800>>UTRIE2_SHIFT_2) : 0, \
                      trieIndex, c)


#define _UTRIE2_INDEX_FROM_SUPP(trieIndex, c) \
    (((int32_t)((trieIndex)[ \
        (trieIndex)[(UTRIE2_INDEX_1_OFFSET-UTRIE2_OMITTED_BMP_INDEX_1_LENGTH)+ \
                      ((c)>>UTRIE2_SHIFT_1)]+ \
        (((c)>>UTRIE2_SHIFT_2)&UTRIE2_INDEX_2_MASK)]) \
    <<UTRIE2_INDEX_SHIFT)+ \
    ((c)&UTRIE2_DATA_MASK))





#define _UTRIE2_INDEX_FROM_CP(trie, asciiOffset, c) \
    ((uint32_t)(c)<0xd800 ? \
        _UTRIE2_INDEX_RAW(0, (trie)->index, c) : \
        (uint32_t)(c)<=0xffff ? \
            _UTRIE2_INDEX_RAW( \
                (c)<=0xdbff ? UTRIE2_LSCP_INDEX_2_OFFSET-(0xd800>>UTRIE2_SHIFT_2) : 0, \
                (trie)->index, c) : \
            (uint32_t)(c)>0x10ffff ? \
                (asciiOffset)+UTRIE2_BAD_UTF8_DATA_OFFSET : \
                (c)>=(trie)->highStart ? \
                    (trie)->highValueIndex : \
                    _UTRIE2_INDEX_FROM_SUPP((trie)->index, c))


#define _UTRIE2_GET_FROM_U16_SINGLE_LEAD(trie, data, c) \
    (trie)->data[_UTRIE2_INDEX_FROM_U16_SINGLE_LEAD((trie)->index, c)]


#define _UTRIE2_GET_FROM_SUPP(trie, data, c) \
    (trie)->data[(c)>=(trie)->highStart ? (trie)->highValueIndex : \
                 _UTRIE2_INDEX_FROM_SUPP((trie)->index, c)]





#define _UTRIE2_GET(trie, data, asciiOffset, c) \
    (trie)->data[_UTRIE2_INDEX_FROM_CP(trie, asciiOffset, c)]


#define _UTRIE2_U16_NEXT(trie, data, src, limit, c, result) { \
    { \
        uint16_t __c2; \
        (c)=*(src)++; \
        if(!U16_IS_LEAD(c)) { \
            (result)=_UTRIE2_GET_FROM_U16_SINGLE_LEAD(trie, data, c); \
        } else if((src)==(limit) || !U16_IS_TRAIL(__c2=*(src))) { \
            (result)=(trie)->data[_UTRIE2_INDEX_FROM_LSCP((trie)->index, c)]; \
        } else { \
            ++(src); \
            (c)=U16_GET_SUPPLEMENTARY((c), __c2); \
            (result)=_UTRIE2_GET_FROM_SUPP((trie), data, (c)); \
        } \
    } \
}


#define _UTRIE2_U16_PREV(trie, data, start, src, c, result) { \
    { \
        uint16_t __c2; \
        (c)=*--(src); \
        if(!U16_IS_TRAIL(c) || (src)==(start) || !U16_IS_LEAD(__c2=*((src)-1))) { \
            (result)=(trie)->data[_UTRIE2_INDEX_FROM_BMP((trie)->index, c)]; \
        } else { \
            --(src); \
            (c)=U16_GET_SUPPLEMENTARY(__c2, (c)); \
            (result)=_UTRIE2_GET_FROM_SUPP((trie), data, (c)); \
        } \
    } \
}


#define _UTRIE2_U8_NEXT(trie, ascii, data, src, limit, result) { \
    uint8_t __lead=(uint8_t)*(src)++; \
    if(__lead<0xc0) { \
        (result)=(trie)->ascii[__lead]; \
    } else { \
        uint8_t __t1, __t2; \
        if( /* handle U+0000..U+07FF inline */ \
            __lead<0xe0 && (src)<(limit) && \
            (__t1=(uint8_t)(*(src)-0x80))<=0x3f \
        ) { \
            ++(src); \
            (result)=(trie)->data[ \
                (trie)->index[(UTRIE2_UTF8_2B_INDEX_2_OFFSET-0xc0)+__lead]+ \
                __t1]; \
        } else if( /* handle U+0000..U+CFFF inline */ \
            __lead<0xed && ((src)+1)<(limit) && \
            (__t1=(uint8_t)(*(src)-0x80))<=0x3f && (__lead>0xe0 || __t1>=0x20) && \
            (__t2=(uint8_t)(*((src)+1)-0x80))<= 0x3f \
        ) { \
            (src)+=2; \
            (result)=(trie)->data[ \
                ((int32_t)((trie)->index[((__lead-0xe0)<<(12-UTRIE2_SHIFT_2))+ \
                                         (__t1<<(6-UTRIE2_SHIFT_2))+(__t2>>UTRIE2_SHIFT_2)]) \
                <<UTRIE2_INDEX_SHIFT)+ \
                (__t2&UTRIE2_DATA_MASK)]; \
        } else { \
            int32_t __index=utrie2_internalU8NextIndex((trie), __lead, (const uint8_t *)(src), \
                                                                       (const uint8_t *)(limit)); \
            (src)+=__index&7; \
            (result)=(trie)->data[__index>>3]; \
        } \
    } \
}


#define _UTRIE2_U8_PREV(trie, ascii, data, start, src, result) { \
    uint8_t __b=(uint8_t)*--(src); \
    if(__b<0x80) { \
        (result)=(trie)->ascii[__b]; \
    } else { \
        int32_t __index=utrie2_internalU8PrevIndex((trie), __b, (const uint8_t *)(start), \
                                                                (const uint8_t *)(src)); \
        (src)-=__index&7; \
        (result)=(trie)->data[__index>>3]; \
    } \
}

U_CDECL_END




#if defined (U_HAVE_MSVC_2003_OR_EARLIER)
#pragma optimize("", off)
#endif

#endif
