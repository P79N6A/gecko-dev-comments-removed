

















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION && !UCONFIG_NO_LEGACY_CONVERSION

#include "unicode/uset.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "ucnv_ext.h"
#include "cmemory.h"
#include "uassert.h"






static inline uint32_t
ucnv_extFindToU(const uint32_t *toUSection, int32_t length, uint8_t byte) {
    uint32_t word0, word;
    int32_t i, start, limit;

    
    start=(int32_t)UCNV_EXT_TO_U_GET_BYTE(toUSection[0]);
    limit=(int32_t)UCNV_EXT_TO_U_GET_BYTE(toUSection[length-1]);
    if(byte<start || limit<byte) {
        return 0; 
    }

    if(length==((limit-start)+1)) {
        
        return UCNV_EXT_TO_U_GET_VALUE(toUSection[byte-start]); 
    }

    
    word0=UCNV_EXT_TO_U_MAKE_WORD(byte, 0);

    







    word=word0|UCNV_EXT_TO_U_VALUE_MASK;

    
    start=0;
    limit=length;
    for(;;) {
        i=limit-start;
        if(i<=1) {
            break; 
        }
        

        if(i<=4) {
            
            if(word0<=toUSection[start]) {
                break;
            }
            if(++start<limit && word0<=toUSection[start]) {
                break;
            }
            if(++start<limit && word0<=toUSection[start]) {
                break;
            }
            
            ++start;
            break;
        }

        i=(start+limit)/2;
        if(word<toUSection[i]) {
            limit=i;
        } else {
            start=i;
        }
    }

    
    if(start<limit && byte==UCNV_EXT_TO_U_GET_BYTE(word=toUSection[start])) {
        return UCNV_EXT_TO_U_GET_VALUE(word); 
    } else {
        return 0; 
    }
}





#define UCNV_EXT_TO_U_VERIFY_SISO_MATCH(sisoState, match) \
    ((sisoState)<0 || ((sisoState)==0) == (match==1))







static int32_t
ucnv_extMatchToU(const int32_t *cx, int8_t sisoState,
                 const char *pre, int32_t preLength,
                 const char *src, int32_t srcLength,
                 uint32_t *pMatchValue,
                 UBool , UBool flush) {
    const uint32_t *toUTable, *toUSection;

    uint32_t value, matchValue;
    int32_t i, j, idx, length, matchLength;
    uint8_t b;

    if(cx==NULL || cx[UCNV_EXT_TO_U_LENGTH]<=0) {
        return 0; 
    }

    
    toUTable=UCNV_EXT_ARRAY(cx, UCNV_EXT_TO_U_INDEX, uint32_t);
    idx=0;

    matchValue=0;
    i=j=matchLength=0;

    if(sisoState==0) {
        
        if(preLength>1) {
            return 0; 
        } else if(preLength==1) {
            srcLength=0;
        } else  {
            if(srcLength>1) {
                srcLength=1;
            }
        }
        flush=TRUE;
    }

    

    
    for(;;) {
        
        toUSection=toUTable+idx;

        
        value=*toUSection++;
        length=UCNV_EXT_TO_U_GET_BYTE(value);
        value=UCNV_EXT_TO_U_GET_VALUE(value);
        if( value!=0 &&
            (UCNV_EXT_TO_U_IS_ROUNDTRIP(value) ||
             TO_U_USE_FALLBACK(useFallback)) &&
            UCNV_EXT_TO_U_VERIFY_SISO_MATCH(sisoState, i+j)
        ) {
            
            matchValue=value;
            matchLength=i+j;
        }

        
        if(i<preLength) {
            b=(uint8_t)pre[i++];
        } else if(j<srcLength) {
            b=(uint8_t)src[j++];
        } else {
            
            if(flush || (length=(i+j))>UCNV_EXT_MAX_BYTES) {
                




                break;
            } else {
                
                return -length;
            }
        }

        
        value=ucnv_extFindToU(toUSection, length, b);
        if(value==0) {
            
            break;
        } else {
            if(UCNV_EXT_TO_U_IS_PARTIAL(value)) {
                
                idx=(int32_t)UCNV_EXT_TO_U_GET_PARTIAL_INDEX(value);
            } else {
                if( (UCNV_EXT_TO_U_IS_ROUNDTRIP(value) ||
                     TO_U_USE_FALLBACK(useFallback)) &&
                    UCNV_EXT_TO_U_VERIFY_SISO_MATCH(sisoState, i+j)
                ) {
                    
                    matchValue=value;
                    matchLength=i+j;
                } else {
                    
                }
                break;
            }
        }
    }

    if(matchLength==0) {
        
        return 0;
    }

    
    *pMatchValue=UCNV_EXT_TO_U_MASK_ROUNDTRIP(matchValue);
    return matchLength;
}

static inline void
ucnv_extWriteToU(UConverter *cnv, const int32_t *cx,
                 uint32_t value,
                 UChar **target, const UChar *targetLimit,
                 int32_t **offsets, int32_t srcIndex,
                 UErrorCode *pErrorCode) {
    
    if(UCNV_EXT_TO_U_IS_CODE_POINT(value)) {
        
        ucnv_toUWriteCodePoint(
            cnv, UCNV_EXT_TO_U_GET_CODE_POINT(value),
            target, targetLimit,
            offsets, srcIndex,
            pErrorCode);
    } else {
        
        ucnv_toUWriteUChars(
            cnv,
            UCNV_EXT_ARRAY(cx, UCNV_EXT_TO_U_UCHARS_INDEX, UChar)+
                UCNV_EXT_TO_U_GET_INDEX(value),
            UCNV_EXT_TO_U_GET_LENGTH(value),
            target, targetLimit,
            offsets, srcIndex,
            pErrorCode);
    }
}









#define UCNV_SISO_STATE(cnv) \
    ((cnv)->sharedData->mbcs.outputType==MBCS_OUTPUT_2_SISO ? (int8_t)(cnv)->mode : \
     (cnv)->sharedData->mbcs.outputType==MBCS_OUTPUT_DBCS_ONLY ? 1 : -1)




U_CFUNC UBool
ucnv_extInitialMatchToU(UConverter *cnv, const int32_t *cx,
                        int32_t firstLength,
                        const char **src, const char *srcLimit,
                        UChar **target, const UChar *targetLimit,
                        int32_t **offsets, int32_t srcIndex,
                        UBool flush,
                        UErrorCode *pErrorCode) {
    uint32_t value = 0;  
    int32_t match;

    
    match=ucnv_extMatchToU(cx, (int8_t)UCNV_SISO_STATE(cnv),
                           (const char *)cnv->toUBytes, firstLength,
                           *src, (int32_t)(srcLimit-*src),
                           &value,
                           cnv->useFallback, flush);
    if(match>0) {
        
        *src+=match-firstLength;

        
        ucnv_extWriteToU(cnv, cx,
                         value,
                         target, targetLimit,
                         offsets, srcIndex,
                         pErrorCode);
        return TRUE;
    } else if(match<0) {
        
        const char *s;
        int32_t j;

        
        s=(const char *)cnv->toUBytes;
        cnv->preToUFirstLength=(int8_t)firstLength;
        for(j=0; j<firstLength; ++j) {
            cnv->preToU[j]=*s++;
        }

        
        s=*src;
        match=-match;
        for(; j<match; ++j) {
            cnv->preToU[j]=*s++;
        }
        *src=s; 
        cnv->preToULength=(int8_t)match;
        return TRUE;
    } else  {
        return FALSE;
    }
}

U_CFUNC UChar32
ucnv_extSimpleMatchToU(const int32_t *cx,
                       const char *source, int32_t length,
                       UBool useFallback) {
    uint32_t value = 0;  
    int32_t match;

    if(length<=0) {
        return 0xffff;
    }

    
    match=ucnv_extMatchToU(cx, -1,
                           source, length,
                           NULL, 0,
                           &value,
                           useFallback, TRUE);
    if(match==length) {
        
        if(UCNV_EXT_TO_U_IS_CODE_POINT(value)) {
            return UCNV_EXT_TO_U_GET_CODE_POINT(value);
        }
    }

    






    return 0xfffe;
}





U_CFUNC void
ucnv_extContinueMatchToU(UConverter *cnv,
                         UConverterToUnicodeArgs *pArgs, int32_t srcIndex,
                         UErrorCode *pErrorCode) {
    uint32_t value = 0;  
    int32_t match, length;

    match=ucnv_extMatchToU(cnv->sharedData->mbcs.extIndexes, (int8_t)UCNV_SISO_STATE(cnv),
                           cnv->preToU, cnv->preToULength,
                           pArgs->source, (int32_t)(pArgs->sourceLimit-pArgs->source),
                           &value,
                           cnv->useFallback, pArgs->flush);
    if(match>0) {
        if(match>=cnv->preToULength) {
            
            pArgs->source+=match-cnv->preToULength;
            cnv->preToULength=0;
        } else {
            
            length=cnv->preToULength-match;
            uprv_memmove(cnv->preToU, cnv->preToU+match, length);
            cnv->preToULength=(int8_t)-length;
        }

        
        ucnv_extWriteToU(cnv, cnv->sharedData->mbcs.extIndexes,
                         value,
                         &pArgs->target, pArgs->targetLimit,
                         &pArgs->offsets, srcIndex,
                         pErrorCode);
    } else if(match<0) {
        
        const char *s;
        int32_t j;

        
        s=pArgs->source;
        match=-match;
        for(j=cnv->preToULength; j<match; ++j) {
            cnv->preToU[j]=*s++;
        }
        pArgs->source=s; 
        cnv->preToULength=(int8_t)match;
    } else  {
        

















        
        uprv_memcpy(cnv->toUBytes, cnv->preToU, cnv->preToUFirstLength);
        cnv->toULength=cnv->preToUFirstLength;

        
        length=cnv->preToULength-cnv->preToUFirstLength;
        if(length>0) {
            uprv_memmove(cnv->preToU, cnv->preToU+cnv->preToUFirstLength, length);
        }

        
        cnv->preToULength=(int8_t)-length;

        
        *pErrorCode=U_INVALID_CHAR_FOUND;
    }
}






static inline int32_t
ucnv_extFindFromU(const UChar *fromUSection, int32_t length, UChar u) {
    int32_t i, start, limit;

    
    start=0;
    limit=length;
    for(;;) {
        i=limit-start;
        if(i<=1) {
            break; 
        }
        

        if(i<=4) {
            
            if(u<=fromUSection[start]) {
                break;
            }
            if(++start<limit && u<=fromUSection[start]) {
                break;
            }
            if(++start<limit && u<=fromUSection[start]) {
                break;
            }
            
            ++start;
            break;
        }

        i=(start+limit)/2;
        if(u<fromUSection[i]) {
            limit=i;
        } else {
            start=i;
        }
    }

    
    if(start<limit && u==fromUSection[start]) {
        return start;
    } else {
        return -1; 
    }
}





















static int32_t
ucnv_extMatchFromU(const int32_t *cx,
                   UChar32 firstCP,
                   const UChar *pre, int32_t preLength,
                   const UChar *src, int32_t srcLength,
                   uint32_t *pMatchValue,
                   UBool useFallback, UBool flush) {
    const uint16_t *stage12, *stage3;
    const uint32_t *stage3b;

    const UChar *fromUTableUChars, *fromUSectionUChars;
    const uint32_t *fromUTableValues, *fromUSectionValues;

    uint32_t value, matchValue;
    int32_t i, j, idx, length, matchLength;
    UChar c;

    if(cx==NULL) {
        return 0; 
    }

    
    idx=firstCP>>10; 
    if(idx>=cx[UCNV_EXT_FROM_U_STAGE_1_LENGTH]) {
        return 0; 
    }

    stage12=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_STAGE_12_INDEX, uint16_t);
    stage3=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_STAGE_3_INDEX, uint16_t);
    idx=UCNV_EXT_FROM_U(stage12, stage3, idx, firstCP);

    stage3b=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_STAGE_3B_INDEX, uint32_t);
    value=stage3b[idx];
    if(value==0) {
        return 0;
    }

    





    if(UCNV_EXT_TO_U_IS_PARTIAL(value)) {
        
        idx=(int32_t)UCNV_EXT_FROM_U_GET_PARTIAL_INDEX(value);

        
        fromUTableUChars=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_UCHARS_INDEX, UChar);
        fromUTableValues=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_VALUES_INDEX, uint32_t);

        matchValue=0;
        i=j=matchLength=0;

        

        
        for(;;) {
            
            fromUSectionUChars=fromUTableUChars+idx;
            fromUSectionValues=fromUTableValues+idx;

            
            length=*fromUSectionUChars++;
            value=*fromUSectionValues++;
            if( value!=0 &&
                (UCNV_EXT_FROM_U_IS_ROUNDTRIP(value) ||
                 FROM_U_USE_FALLBACK(useFallback, firstCP)) &&
                (value&UCNV_EXT_FROM_U_RESERVED_MASK)==0
            ) {
                
                matchValue=value;
                matchLength=2+i+j;
            }

            
            if(i<preLength) {
                c=pre[i++];
            } else if(j<srcLength) {
                c=src[j++];
            } else {
                
                if(flush || (length=(i+j))>UCNV_EXT_MAX_UCHARS) {
                    




                    break;
                } else {
                    
                    return -(2+length);
                }
            }

            
            idx=ucnv_extFindFromU(fromUSectionUChars, length, c);
            if(idx<0) {
                
                break;
            } else {
                value=fromUSectionValues[idx];
                if(UCNV_EXT_FROM_U_IS_PARTIAL(value)) {
                    
                    idx=(int32_t)UCNV_EXT_FROM_U_GET_PARTIAL_INDEX(value);
                } else {
                    if( (UCNV_EXT_FROM_U_IS_ROUNDTRIP(value) ||
                         FROM_U_USE_FALLBACK(useFallback, firstCP)) &&
                        (value&UCNV_EXT_FROM_U_RESERVED_MASK)==0
                    ) {
                        
                        matchValue=value;
                        matchLength=2+i+j;
                    } else {
                        
                    }
                    break;
                }
            }
        }

        if(matchLength==0) {
            
            return 0;
        }
    } else  {
        if( (UCNV_EXT_FROM_U_IS_ROUNDTRIP(value) ||
             FROM_U_USE_FALLBACK(useFallback, firstCP)) &&
            (value&UCNV_EXT_FROM_U_RESERVED_MASK)==0
        ) {
            
            matchValue=value;
            matchLength=2;
        } else {
            
            return 0;
        }
    }

    
    if(matchValue==UCNV_EXT_FROM_U_SUBCHAR1) {
        return 1; 
    }

    *pMatchValue=matchValue;
    return matchLength;
}




static inline void
ucnv_extWriteFromU(UConverter *cnv, const int32_t *cx,
                   uint32_t value,
                   char **target, const char *targetLimit,
                   int32_t **offsets, int32_t srcIndex,
                   UErrorCode *pErrorCode) {
    uint8_t buffer[1+UCNV_EXT_MAX_BYTES];
    const uint8_t *result;
    int32_t length, prevLength;

    length=UCNV_EXT_FROM_U_GET_LENGTH(value);
    value=(uint32_t)UCNV_EXT_FROM_U_GET_DATA(value);

    
    if(length<=UCNV_EXT_FROM_U_MAX_DIRECT_LENGTH) {
        





        uint8_t *p=buffer+1; 
        switch(length) {
        case 3:
            *p++=(uint8_t)(value>>16);
        case 2: 
            *p++=(uint8_t)(value>>8);
        case 1: 
            *p++=(uint8_t)value;
        default:
            break; 
        }
        result=buffer+1;
    } else {
        result=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_BYTES_INDEX, uint8_t)+value;
    }

    

    if((prevLength=cnv->fromUnicodeStatus)!=0) {
        
        uint8_t shiftByte;

        if(prevLength>1 && length==1) {
            
            shiftByte=(uint8_t)UCNV_SI;
            cnv->fromUnicodeStatus=1;
        } else if(prevLength==1 && length>1) {
            
            shiftByte=(uint8_t)UCNV_SO;
            cnv->fromUnicodeStatus=2;
        } else {
            shiftByte=0;
        }

        if(shiftByte!=0) {
            
            buffer[0]=shiftByte;
            if(result!=buffer+1) {
                uprv_memcpy(buffer+1, result, length);
            }
            result=buffer;
            ++length;
        }
    }

    ucnv_fromUWriteBytes(cnv, (const char *)result, length,
                         target, targetLimit,
                         offsets, srcIndex,
                         pErrorCode);
}




U_CFUNC UBool
ucnv_extInitialMatchFromU(UConverter *cnv, const int32_t *cx,
                          UChar32 cp,
                          const UChar **src, const UChar *srcLimit,
                          char **target, const char *targetLimit,
                          int32_t **offsets, int32_t srcIndex,
                          UBool flush,
                          UErrorCode *pErrorCode) {
    uint32_t value = 0;  
    int32_t match;

    
    match=ucnv_extMatchFromU(cx, cp,
                             NULL, 0,
                             *src, (int32_t)(srcLimit-*src),
                             &value,
                             cnv->useFallback, flush);

    
    if( match>=2 &&
        !(UCNV_EXT_FROM_U_GET_LENGTH(value)==1 &&
          cnv->sharedData->mbcs.outputType==MBCS_OUTPUT_DBCS_ONLY)
    ) {
        
        *src+=match-2; 

        
        ucnv_extWriteFromU(cnv, cx,
                           value,
                           target, targetLimit,
                           offsets, srcIndex,
                           pErrorCode);
        return TRUE;
    } else if(match<0) {
        
        const UChar *s;
        int32_t j;

        
        cnv->preFromUFirstCP=cp;

        
        s=*src;
        match=-match-2; 
        for(j=0; j<match; ++j) {
            cnv->preFromU[j]=*s++;
        }
        *src=s; 
        cnv->preFromULength=(int8_t)match;
        return TRUE;
    } else if(match==1) {
        
        cnv->useSubChar1=TRUE;
        return FALSE;
    } else  {
        return FALSE;
    }
}





U_CFUNC int32_t
ucnv_extSimpleMatchFromU(const int32_t *cx,
                         UChar32 cp, uint32_t *pValue,
                         UBool useFallback) {
    uint32_t value;
    int32_t match;

    
    match=ucnv_extMatchFromU(cx,
                             cp,
                             NULL, 0,
                             NULL, 0,
                             &value,
                             useFallback, TRUE);
    if(match>=2) {
        
        int32_t length;
        int isRoundtrip;

        isRoundtrip=UCNV_EXT_FROM_U_IS_ROUNDTRIP(value);
        length=UCNV_EXT_FROM_U_GET_LENGTH(value);
        value=(uint32_t)UCNV_EXT_FROM_U_GET_DATA(value);

        if(length<=UCNV_EXT_FROM_U_MAX_DIRECT_LENGTH) {
            *pValue=value;
            return isRoundtrip ? length : -length;
#if 0 
        } else if(length==4) {
            
            const uint8_t *result=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_BYTES_INDEX, uint8_t)+value;
            *pValue=
                ((uint32_t)result[0]<<24)|
                ((uint32_t)result[1]<<16)|
                ((uint32_t)result[2]<<8)|
                result[3];
            return isRoundtrip ? 4 : -4;
#endif
        }
    }

    






    return 0;
}





U_CFUNC void
ucnv_extContinueMatchFromU(UConverter *cnv,
                           UConverterFromUnicodeArgs *pArgs, int32_t srcIndex,
                           UErrorCode *pErrorCode) {
    uint32_t value = 0;  
    int32_t match;

    match=ucnv_extMatchFromU(cnv->sharedData->mbcs.extIndexes,
                             cnv->preFromUFirstCP,
                             cnv->preFromU, cnv->preFromULength,
                             pArgs->source, (int32_t)(pArgs->sourceLimit-pArgs->source),
                             &value,
                             cnv->useFallback, pArgs->flush);
    if(match>=2) {
        match-=2; 

        if(match>=cnv->preFromULength) {
            
            pArgs->source+=match-cnv->preFromULength;
            cnv->preFromULength=0;
        } else {
            
            int32_t length=cnv->preFromULength-match;
            uprv_memmove(cnv->preFromU, cnv->preFromU+match, length*U_SIZEOF_UCHAR);
            cnv->preFromULength=(int8_t)-length;
        }

        
        cnv->preFromUFirstCP=U_SENTINEL;

        
        ucnv_extWriteFromU(cnv, cnv->sharedData->mbcs.extIndexes,
                           value,
                           &pArgs->target, pArgs->targetLimit,
                           &pArgs->offsets, srcIndex,
                           pErrorCode);
    } else if(match<0) {
        
        const UChar *s;
        int32_t j;

        
        s=pArgs->source;
        match=-match-2; 
        for(j=cnv->preFromULength; j<match; ++j) {
            U_ASSERT(j>=0);
            cnv->preFromU[j]=*s++;
        }
        pArgs->source=s; 
        cnv->preFromULength=(int8_t)match;
    } else  {
        

















        if(match==1) {
            
            cnv->useSubChar1=TRUE;
        }

        
        cnv->fromUChar32=cnv->preFromUFirstCP;
        cnv->preFromUFirstCP=U_SENTINEL;

        
        cnv->preFromULength=-cnv->preFromULength;

        
        *pErrorCode=U_INVALID_CHAR_FOUND;
    }
}

static void
ucnv_extGetUnicodeSetString(const UConverterSharedData *sharedData,
                            const int32_t *cx,
                            const USetAdder *sa,
                            UBool useFallback,
                            int32_t minLength,
                            UChar32 c,
                            UChar s[UCNV_EXT_MAX_UCHARS], int32_t length,
                            int32_t sectionIndex,
                            UErrorCode *pErrorCode) {
    const UChar *fromUSectionUChars;
    const uint32_t *fromUSectionValues;

    uint32_t value;
    int32_t i, count;

    fromUSectionUChars=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_UCHARS_INDEX, UChar)+sectionIndex;
    fromUSectionValues=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_VALUES_INDEX, uint32_t)+sectionIndex;

    
    count=*fromUSectionUChars++;
    value=*fromUSectionValues++;

    if( value!=0 &&
        (UCNV_EXT_FROM_U_IS_ROUNDTRIP(value) || useFallback) &&
        UCNV_EXT_FROM_U_GET_LENGTH(value)>=minLength
    ) {
        if(c>=0) {
            
            sa->add(sa->set, c);
        } else {
            
            sa->addString(sa->set, s, length);
        }
    }

    for(i=0; i<count; ++i) {
        
        s[length]=fromUSectionUChars[i];
        value=fromUSectionValues[i];

        if(value==0) {
            
        } else if(UCNV_EXT_FROM_U_IS_PARTIAL(value)) {
            ucnv_extGetUnicodeSetString(
                sharedData, cx, sa, useFallback, minLength,
                U_SENTINEL, s, length+1,
                (int32_t)UCNV_EXT_FROM_U_GET_PARTIAL_INDEX(value),
                pErrorCode);
        } else if((useFallback ?
                      (value&UCNV_EXT_FROM_U_RESERVED_MASK)==0 :
                      ((value&(UCNV_EXT_FROM_U_ROUNDTRIP_FLAG|UCNV_EXT_FROM_U_RESERVED_MASK))==
                          UCNV_EXT_FROM_U_ROUNDTRIP_FLAG)) &&
                  UCNV_EXT_FROM_U_GET_LENGTH(value)>=minLength
        ) {
            sa->addString(sa->set, s, length+1);
        }
    }
}

U_CFUNC void
ucnv_extGetUnicodeSet(const UConverterSharedData *sharedData,
                      const USetAdder *sa,
                      UConverterUnicodeSet which,
                      UConverterSetFilter filter,
                      UErrorCode *pErrorCode) {
    const int32_t *cx;
    const uint16_t *stage12, *stage3, *ps2, *ps3;
    const uint32_t *stage3b;

    uint32_t value;
    int32_t st1, stage1Length, st2, st3, minLength;
    UBool useFallback;

    UChar s[UCNV_EXT_MAX_UCHARS];
    UChar32 c;
    int32_t length;

    cx=sharedData->mbcs.extIndexes;
    if(cx==NULL) {
        return;
    }

    stage12=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_STAGE_12_INDEX, uint16_t);
    stage3=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_STAGE_3_INDEX, uint16_t);
    stage3b=UCNV_EXT_ARRAY(cx, UCNV_EXT_FROM_U_STAGE_3B_INDEX, uint32_t);

    stage1Length=cx[UCNV_EXT_FROM_U_STAGE_1_LENGTH];

    useFallback=(UBool)(which==UCNV_ROUNDTRIP_AND_FALLBACK_SET);

    
    c=0; 

    if(filter==UCNV_SET_FILTER_2022_CN) {
        minLength=3;
    } else if( sharedData->mbcs.outputType==MBCS_OUTPUT_DBCS_ONLY ||
               filter!=UCNV_SET_FILTER_NONE
    ) {
        
        minLength=2;
    } else {
        minLength=1;
    }

    



    for(st1=0; st1<stage1Length; ++st1) {
        st2=stage12[st1];
        if(st2>stage1Length) {
            ps2=stage12+st2;
            for(st2=0; st2<64; ++st2) {
                if((st3=(int32_t)ps2[st2]<<UCNV_EXT_STAGE_2_LEFT_SHIFT)!=0) {
                    
                    ps3=stage3+st3;

                    





                    do {
                        value=stage3b[*ps3++];
                        if(value==0) {
                            
                        } else if(UCNV_EXT_FROM_U_IS_PARTIAL(value)) {
                            length=0;
                            U16_APPEND_UNSAFE(s, length, c);
                            ucnv_extGetUnicodeSetString(
                                sharedData, cx, sa, useFallback, minLength,
                                c, s, length,
                                (int32_t)UCNV_EXT_FROM_U_GET_PARTIAL_INDEX(value),
                                pErrorCode);
                        } else if((useFallback ?
                                      (value&UCNV_EXT_FROM_U_RESERVED_MASK)==0 :
                                      ((value&(UCNV_EXT_FROM_U_ROUNDTRIP_FLAG|UCNV_EXT_FROM_U_RESERVED_MASK))==
                                          UCNV_EXT_FROM_U_ROUNDTRIP_FLAG)) &&
                                  UCNV_EXT_FROM_U_GET_LENGTH(value)>=minLength
                        ) {
                            switch(filter) {
                            case UCNV_SET_FILTER_2022_CN:
                                if(!(UCNV_EXT_FROM_U_GET_LENGTH(value)==3 && UCNV_EXT_FROM_U_GET_DATA(value)<=0x82ffff)) {
                                    continue;
                                }
                                break;
                            case UCNV_SET_FILTER_SJIS:
                                if(!(UCNV_EXT_FROM_U_GET_LENGTH(value)==2 && (value=UCNV_EXT_FROM_U_GET_DATA(value))>=0x8140 && value<=0xeffc)) {
                                    continue;
                                }
                                break;
                            case UCNV_SET_FILTER_GR94DBCS:
                                if(!(UCNV_EXT_FROM_U_GET_LENGTH(value)==2 &&
                                     (uint16_t)((value=UCNV_EXT_FROM_U_GET_DATA(value))-0xa1a1)<=(0xfefe - 0xa1a1) &&
                                     (uint8_t)(value-0xa1)<=(0xfe - 0xa1))) {
                                    continue;
                                }
                                break;
                            case UCNV_SET_FILTER_HZ:
                                if(!(UCNV_EXT_FROM_U_GET_LENGTH(value)==2 &&
                                     (uint16_t)((value=UCNV_EXT_FROM_U_GET_DATA(value))-0xa1a1)<=(0xfdfe - 0xa1a1) &&
                                     (uint8_t)(value-0xa1)<=(0xfe - 0xa1))) {
                                    continue;
                                }
                                break;
                            default:
                                



                                break;
                            }
                            sa->add(sa->set, c);
                        }
                    } while((++c&0xf)!=0);
                } else {
                    c+=16; 
                }
            }
        } else {
            c+=1024; 
        }
    }
}

#endif
