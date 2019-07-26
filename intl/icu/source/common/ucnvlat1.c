













#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "unicode/uset.h"
#include "unicode/utf8.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"


#define LATIN1_UNROLL_FROM_UNICODE 1




static void
_Latin1ToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                            UErrorCode *pErrorCode) {
    const uint8_t *source;
    UChar *target;
    int32_t targetCapacity, length;
    int32_t *offsets;

    int32_t sourceIndex;

    
    source=(const uint8_t *)pArgs->source;
    target=pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    sourceIndex=0;

    



    length=(int32_t)((const uint8_t *)pArgs->sourceLimit-source);
    if(length<=targetCapacity) {
        targetCapacity=length;
    } else {
        
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        length=targetCapacity;
    }

    if(targetCapacity>=8) {
        
        int32_t count, loops;

        loops=count=targetCapacity>>3;
        length=targetCapacity&=0x7;
        do {
            target[0]=source[0];
            target[1]=source[1];
            target[2]=source[2];
            target[3]=source[3];
            target[4]=source[4];
            target[5]=source[5];
            target[6]=source[6];
            target[7]=source[7];
            target+=8;
            source+=8;
        } while(--count>0);

        if(offsets!=NULL) {
            do {
                offsets[0]=sourceIndex++;
                offsets[1]=sourceIndex++;
                offsets[2]=sourceIndex++;
                offsets[3]=sourceIndex++;
                offsets[4]=sourceIndex++;
                offsets[5]=sourceIndex++;
                offsets[6]=sourceIndex++;
                offsets[7]=sourceIndex++;
                offsets+=8;
            } while(--loops>0);
        }
    }

    
    while(targetCapacity>0) {
        *target++=*source++;
        --targetCapacity;
    }

    
    pArgs->source=(const char *)source;
    pArgs->target=target;

    
    if(offsets!=NULL) {
        while(length>0) {
            *offsets++=sourceIndex++;
            --length;
        }
        pArgs->offsets=offsets;
    }
}


static UChar32
_Latin1GetNextUChar(UConverterToUnicodeArgs *pArgs,
                    UErrorCode *pErrorCode) {
    const uint8_t *source=(const uint8_t *)pArgs->source;
    if(source<(const uint8_t *)pArgs->sourceLimit) {
        pArgs->source=(const char *)(source+1);
        return *source;
    }

    
    *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
    return 0xffff;
}


static void
_Latin1FromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                              UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target, *oldTarget;
    int32_t targetCapacity, length;
    int32_t *offsets;

    UChar32 cp;
    UChar c, max;

    int32_t sourceIndex;

    
    cnv=pArgs->converter;
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=oldTarget=(uint8_t *)pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    if(cnv->sharedData==&_Latin1Data) {
        max=0xff; 
    } else {
        max=0x7f; 
    }

    
    cp=cnv->fromUChar32;

    
    sourceIndex= cp==0 ? 0 : -1;

    



    length=(int32_t)(sourceLimit-source);
    if(length<targetCapacity) {
        targetCapacity=length;
    }

    
    if(cp!=0 && targetCapacity>0) {
        goto getTrail;
    }

#if LATIN1_UNROLL_FROM_UNICODE
    
    if(targetCapacity>=16) {
        int32_t count, loops;
        UChar u, oredChars;

        loops=count=targetCapacity>>4;
        do {
            oredChars=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;
            oredChars|=u=*source++;
            *target++=(uint8_t)u;

            
            if(oredChars>max) {
                
                source-=16;
                target-=16;
                break;
            }
        } while(--count>0);
        count=loops-count;
        targetCapacity-=16*count;

        if(offsets!=NULL) {
            oldTarget+=16*count;
            while(count>0) {
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                *offsets++=sourceIndex++;
                --count;
            }
        }
    }
#endif

    
    c=0;
    while(targetCapacity>0 && (c=*source++)<=max) {
        
        *target++=(uint8_t)c;
        --targetCapacity;
    }

    if(c>max) {
        cp=c;
        if(!U_IS_SURROGATE(cp)) {
            
        } else if(U_IS_SURROGATE_LEAD(cp)) {
getTrail:
            if(source<sourceLimit) {
                
                UChar trail=*source;
                if(U16_IS_TRAIL(trail)) {
                    ++source;
                    cp=U16_GET_SUPPLEMENTARY(cp, trail);
                    
                    
                } else {
                    
                    
                }
            } else {
                
                cnv->fromUChar32=cp;
                goto noMoreInput;
            }
        } else {
            
            
        }

        *pErrorCode= U_IS_SURROGATE(cp) ? U_ILLEGAL_CHAR_FOUND : U_INVALID_CHAR_FOUND;
        cnv->fromUChar32=cp;
    }
noMoreInput:

    
    if(offsets!=NULL) {
        size_t count=target-oldTarget;
        while(count>0) {
            *offsets++=sourceIndex++;
            --count;
        }
    }

    if(U_SUCCESS(*pErrorCode) && source<sourceLimit && target>=(uint8_t *)pArgs->targetLimit) {
        
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
}


static void
ucnv_Latin1FromUTF8(UConverterFromUnicodeArgs *pFromUArgs,
                    UConverterToUnicodeArgs *pToUArgs,
                    UErrorCode *pErrorCode) {
    UConverter *utf8;
    const uint8_t *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;

    UChar32 c;
    uint8_t b, t1;

    
    utf8=pToUArgs->converter;
    source=(uint8_t *)pToUArgs->source;
    sourceLimit=(uint8_t *)pToUArgs->sourceLimit;
    target=(uint8_t *)pFromUArgs->target;
    targetCapacity=(int32_t)(pFromUArgs->targetLimit-pFromUArgs->target);

    
    c=(UChar32)utf8->toUnicodeStatus;
    if(c!=0 && source<sourceLimit) {
        if(targetCapacity==0) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            return;
        } else if(c>=0xc2 && c<=0xc3 && (t1=(uint8_t)(*source-0x80)) <= 0x3f) {
            ++source;
            *target++=(uint8_t)(((c&3)<<6)|t1);
            --targetCapacity;

            utf8->toUnicodeStatus=0;
            utf8->toULength=0;
        } else {
            
            *pErrorCode=U_USING_DEFAULT_WARNING;
            return;
        }
    }

    







    if(source<sourceLimit && U8_IS_LEAD(*(sourceLimit-1))) {
        --sourceLimit;
    }

    
    while(source<sourceLimit) {
        if(targetCapacity>0) {
            b=*source++;
            if((int8_t)b>=0) {
                
                *target++=(uint8_t)b;
                --targetCapacity;
            } else if( 
                       b>=0xc2 && b<=0xc3 &&
                       (t1=(uint8_t)(*source-0x80)) <= 0x3f
            ) {
                ++source;
                *target++=(uint8_t)(((b&3)<<6)|t1);
                --targetCapacity;
            } else {
                
                pToUArgs->source=(char *)(source-1);
                pFromUArgs->target=(char *)target;
                *pErrorCode=U_USING_DEFAULT_WARNING;
                return;
            }
        } else {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    






    if(U_SUCCESS(*pErrorCode) && source<(sourceLimit=(uint8_t *)pToUArgs->sourceLimit)) {
        utf8->toUnicodeStatus=utf8->toUBytes[0]=b=*source++;
        utf8->toULength=1;
        utf8->mode=U8_COUNT_TRAIL_BYTES(b)+1;
    }

    
    pToUArgs->source=(char *)source;
    pFromUArgs->target=(char *)target;
}

static void
_Latin1GetUnicodeSet(const UConverter *cnv,
                     const USetAdder *sa,
                     UConverterUnicodeSet which,
                     UErrorCode *pErrorCode) {
    sa->addRange(sa->set, 0, 0xff);
}

static const UConverterImpl _Latin1Impl={
    UCNV_LATIN_1,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,

    _Latin1ToUnicodeWithOffsets,
    _Latin1ToUnicodeWithOffsets,
    _Latin1FromUnicodeWithOffsets,
    _Latin1FromUnicodeWithOffsets,
    _Latin1GetNextUChar,

    NULL,
    NULL,
    NULL,
    NULL,
    _Latin1GetUnicodeSet,

    NULL,
    ucnv_Latin1FromUTF8
};

static const UConverterStaticData _Latin1StaticData={
    sizeof(UConverterStaticData),
    "ISO-8859-1",
    819, UCNV_IBM, UCNV_LATIN_1, 1, 1,
    { 0x1a, 0, 0, 0 }, 1, FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

const UConverterSharedData _Latin1Data={
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_Latin1StaticData, FALSE, &_Latin1Impl, 
    0
};




static void
_ASCIIToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                           UErrorCode *pErrorCode) {
    const uint8_t *source, *sourceLimit;
    UChar *target, *oldTarget;
    int32_t targetCapacity, length;
    int32_t *offsets;

    int32_t sourceIndex;

    uint8_t c;

    
    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=oldTarget=pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    
    sourceIndex=0;

    



    length=(int32_t)(sourceLimit-source);
    if(length<targetCapacity) {
        targetCapacity=length;
    }

    if(targetCapacity>=8) {
        
        int32_t count, loops;
        UChar oredChars;

        loops=count=targetCapacity>>3;
        do {
            oredChars=target[0]=source[0];
            oredChars|=target[1]=source[1];
            oredChars|=target[2]=source[2];
            oredChars|=target[3]=source[3];
            oredChars|=target[4]=source[4];
            oredChars|=target[5]=source[5];
            oredChars|=target[6]=source[6];
            oredChars|=target[7]=source[7];

            
            if(oredChars>0x7f) {
                
                break;
            }
            source+=8;
            target+=8;
        } while(--count>0);
        count=loops-count;
        targetCapacity-=count*8;

        if(offsets!=NULL) {
            oldTarget+=count*8;
            while(count>0) {
                offsets[0]=sourceIndex++;
                offsets[1]=sourceIndex++;
                offsets[2]=sourceIndex++;
                offsets[3]=sourceIndex++;
                offsets[4]=sourceIndex++;
                offsets[5]=sourceIndex++;
                offsets[6]=sourceIndex++;
                offsets[7]=sourceIndex++;
                offsets+=8;
                --count;
            }
        }
    }

    
    c=0;
    while(targetCapacity>0 && (c=*source++)<=0x7f) {
        *target++=c;
        --targetCapacity;
    }

    if(c>0x7f) {
        
        UConverter *cnv=pArgs->converter;
        cnv->toUBytes[0]=c;
        cnv->toULength=1;
        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
    } else if(source<sourceLimit && target>=pArgs->targetLimit) {
        
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }

    
    if(offsets!=NULL) {
        size_t count=target-oldTarget;
        while(count>0) {
            *offsets++=sourceIndex++;
            --count;
        }
    }

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
}


static UChar32
_ASCIIGetNextUChar(UConverterToUnicodeArgs *pArgs,
                   UErrorCode *pErrorCode) {
    const uint8_t *source;
    uint8_t b;

    source=(const uint8_t *)pArgs->source;
    if(source<(const uint8_t *)pArgs->sourceLimit) {
        b=*source++;
        pArgs->source=(const char *)source;
        if(b<=0x7f) {
            return b;
        } else {
            UConverter *cnv=pArgs->converter;
            cnv->toUBytes[0]=b;
            cnv->toULength=1;
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
            return 0xffff;
        }
    }

    
    *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
    return 0xffff;
}


static void
ucnv_ASCIIFromUTF8(UConverterFromUnicodeArgs *pFromUArgs,
                   UConverterToUnicodeArgs *pToUArgs,
                   UErrorCode *pErrorCode) {
    const uint8_t *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity, length;

    uint8_t c;

    if(pToUArgs->converter->toUnicodeStatus!=0) {
        
        *pErrorCode=U_USING_DEFAULT_WARNING;
        return;
    }

    
    source=(const uint8_t *)pToUArgs->source;
    sourceLimit=(const uint8_t *)pToUArgs->sourceLimit;
    target=(uint8_t *)pFromUArgs->target;
    targetCapacity=(int32_t)(pFromUArgs->targetLimit-pFromUArgs->target);

    



    length=(int32_t)(sourceLimit-source);
    if(length<targetCapacity) {
        targetCapacity=length;
    }

    
    if(targetCapacity>=16) {
        int32_t count, loops;
        uint8_t oredChars;

        loops=count=targetCapacity>>4;
        do {
            oredChars=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;
            oredChars|=*target++=*source++;

            
            if(oredChars>0x7f) {
                
                source-=16;
                target-=16;
                break;
            }
        } while(--count>0);
        count=loops-count;
        targetCapacity-=16*count;
    }

    
    c=0;
    while(targetCapacity>0 && (c=*source)<=0x7f) {
        ++source;
        *target++=c;
        --targetCapacity;
    }

    if(c>0x7f) {
        
        *pErrorCode=U_USING_DEFAULT_WARNING;
    } else if(source<sourceLimit && target>=(const uint8_t *)pFromUArgs->targetLimit) {
        
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }

    
    pToUArgs->source=(const char *)source;
    pFromUArgs->target=(char *)target;
}

static void
_ASCIIGetUnicodeSet(const UConverter *cnv,
                    const USetAdder *sa,
                    UConverterUnicodeSet which,
                    UErrorCode *pErrorCode) {
    sa->addRange(sa->set, 0, 0x7f);
}

static const UConverterImpl _ASCIIImpl={
    UCNV_US_ASCII,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,

    _ASCIIToUnicodeWithOffsets,
    _ASCIIToUnicodeWithOffsets,
    _Latin1FromUnicodeWithOffsets,
    _Latin1FromUnicodeWithOffsets,
    _ASCIIGetNextUChar,

    NULL,
    NULL,
    NULL,
    NULL,
    _ASCIIGetUnicodeSet,

    NULL,
    ucnv_ASCIIFromUTF8
};

static const UConverterStaticData _ASCIIStaticData={
    sizeof(UConverterStaticData),
    "US-ASCII",
    367, UCNV_IBM, UCNV_US_ASCII, 1, 1,
    { 0x1a, 0, 0, 0 }, 1, FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

const UConverterSharedData _ASCIIData={
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_ASCIIStaticData, FALSE, &_ASCIIImpl, 
    0
};

#endif
