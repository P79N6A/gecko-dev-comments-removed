















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "cmemory.h"

enum {
    UCNV_NEED_TO_WRITE_BOM=1
};





static void
_UTF16ToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                           UErrorCode *pErrorCode);



#if U_IS_BIG_ENDIAN
#   define _UTF16PEFromUnicodeWithOffsets   _UTF16BEFromUnicodeWithOffsets
#else
#   define _UTF16PEFromUnicodeWithOffsets   _UTF16LEFromUnicodeWithOffsets
#endif


static void
_UTF16BEFromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                               UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source;
    char *target;
    int32_t *offsets;

    uint32_t targetCapacity, length, sourceIndex;
    UChar c, trail;
    char overflow[4];

    source=pArgs->source;
    length=(int32_t)(pArgs->sourceLimit-source);
    if(length<=0) {
        
        return;
    }

    cnv=pArgs->converter;

    
    if(cnv->fromUnicodeStatus==UCNV_NEED_TO_WRITE_BOM) {
        static const char bom[]={ (char)0xfe, (char)0xff };
        ucnv_fromUWriteBytes(cnv,
                             bom, 2,
                             &pArgs->target, pArgs->targetLimit,
                             &pArgs->offsets, -1,
                             pErrorCode);
        cnv->fromUnicodeStatus=0;
    }

    target=pArgs->target;
    if(target >= pArgs->targetLimit) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        return;
    }

    targetCapacity=(uint32_t)(pArgs->targetLimit-target);
    offsets=pArgs->offsets;
    sourceIndex=0;

    

    if((c=(UChar)cnv->fromUChar32)!=0 && U16_IS_TRAIL(trail=*source) && targetCapacity>=4) {
        
        ++source;
        --length;
        target[0]=(uint8_t)(c>>8);
        target[1]=(uint8_t)c;
        target[2]=(uint8_t)(trail>>8);
        target[3]=(uint8_t)trail;
        target+=4;
        targetCapacity-=4;
        if(offsets!=NULL) {
            *offsets++=-1;
            *offsets++=-1;
            *offsets++=-1;
            *offsets++=-1;
        }
        sourceIndex=1;
        cnv->fromUChar32=c=0;
    }

    if(c==0) {
        
        uint32_t count=2*length;
        if(count>targetCapacity) {
            count=targetCapacity&~1;
        }
        
        targetCapacity-=count;
        count>>=1;
        length-=count;

        if(offsets==NULL) {
            while(count>0) {
                c=*source++;
                if(U16_IS_SINGLE(c)) {
                    target[0]=(uint8_t)(c>>8);
                    target[1]=(uint8_t)c;
                    target+=2;
                } else if(U16_IS_SURROGATE_LEAD(c) && count>=2 && U16_IS_TRAIL(trail=*source)) {
                    ++source;
                    --count;
                    target[0]=(uint8_t)(c>>8);
                    target[1]=(uint8_t)c;
                    target[2]=(uint8_t)(trail>>8);
                    target[3]=(uint8_t)trail;
                    target+=4;
                } else {
                    break;
                }
                --count;
            }
        } else {
            while(count>0) {
                c=*source++;
                if(U16_IS_SINGLE(c)) {
                    target[0]=(uint8_t)(c>>8);
                    target[1]=(uint8_t)c;
                    target+=2;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex++;
                } else if(U16_IS_SURROGATE_LEAD(c) && count>=2 && U16_IS_TRAIL(trail=*source)) {
                    ++source;
                    --count;
                    target[0]=(uint8_t)(c>>8);
                    target[1]=(uint8_t)c;
                    target[2]=(uint8_t)(trail>>8);
                    target[3]=(uint8_t)trail;
                    target+=4;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex;
                    sourceIndex+=2;
                } else {
                    break;
                }
                --count;
            }
        }

        if(count==0) {
            
            if(length>0 && targetCapacity>0) {
                





                if(U16_IS_SINGLE(c=*source++)) {
                    overflow[0]=(char)(c>>8);
                    overflow[1]=(char)c;
                    length=2; 
                    c=0;
                
                }
            } else {
                length=0;
                c=0;
            }
        } else {
            
            targetCapacity+=2*count;
        }
    } else {
        length=0; 
    }
    
    if(c!=0) {
        




        length=0;
        if(U16_IS_SURROGATE_LEAD(c)) {
            if(source<pArgs->sourceLimit) {
                if(U16_IS_TRAIL(trail=*source)) {
                    
                    ++source;
                    overflow[0]=(char)(c>>8);
                    overflow[1]=(char)c;
                    overflow[2]=(char)(trail>>8);
                    overflow[3]=(char)trail;
                    length=4; 
                    c=0;
                } else {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                }
            } else {
                
            }
        } else {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
        }
        cnv->fromUChar32=c;
    }

    if(length>0) {
        
        ucnv_fromUWriteBytes(cnv,
                             overflow, length,
                             (char **)&target, pArgs->targetLimit,
                             &offsets, sourceIndex,
                             pErrorCode);
        targetCapacity=(uint32_t)(pArgs->targetLimit-(char *)target);
    }

    if(U_SUCCESS(*pErrorCode) && source<pArgs->sourceLimit && targetCapacity==0) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
}

static void
_UTF16BEToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                             UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source;
    UChar *target;
    int32_t *offsets;

    uint32_t targetCapacity, length, count, sourceIndex;
    UChar c, trail;

    if(pArgs->converter->mode<8) {
        _UTF16ToUnicodeWithOffsets(pArgs, pErrorCode);
        return;
    }

    cnv=pArgs->converter;
    source=(const uint8_t *)pArgs->source;
    length=(int32_t)((const uint8_t *)pArgs->sourceLimit-source);
    if(length<=0 && cnv->toUnicodeStatus==0) {
        
        return;
    }

    target=pArgs->target;
    if(target >= pArgs->targetLimit) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        return;
    }

    targetCapacity=(uint32_t)(pArgs->targetLimit-target);
    offsets=pArgs->offsets;
    sourceIndex=0;
    c=0;

    
    if(cnv->toUnicodeStatus!=0) {
        





        cnv->toUBytes[0]=(uint8_t)cnv->toUnicodeStatus;
        cnv->toULength=1;
        cnv->toUnicodeStatus=0;
    }
    if((count=cnv->toULength)!=0) {
        uint8_t *p=cnv->toUBytes;
        do {
            p[count++]=*source++;
            ++sourceIndex;
            --length;
            if(count==2) {
                c=((UChar)p[0]<<8)|p[1];
                if(U16_IS_SINGLE(c)) {
                    
                    *target++=c;
                    if(offsets!=NULL) {
                        *offsets++=-1;
                    }
                    --targetCapacity;
                    count=0;
                    c=0;
                    break;
                } else if(U16_IS_SURROGATE_LEAD(c)) {
                    
                    c=0; 
                } else {
                    
                    break;
                }
            } else if(count==4) {
                c=((UChar)p[0]<<8)|p[1];
                trail=((UChar)p[2]<<8)|p[3];
                if(U16_IS_TRAIL(trail)) {
                    
                    *target++=c;
                    if(targetCapacity>=2) {
                        *target++=trail;
                        if(offsets!=NULL) {
                            *offsets++=-1;
                            *offsets++=-1;
                        }
                        targetCapacity-=2;
                    } else  {
                        targetCapacity=0;
                        cnv->UCharErrorBuffer[0]=trail;
                        cnv->UCharErrorBufferLength=1;
                        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    }
                    count=0;
                    c=0;
                    break;
                } else {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;

                    
                    if(((const uint8_t *)pArgs->source-source)>=2) {
                        source-=2;
                    } else {
                        




                        cnv->toUnicodeStatus=0x100|p[2];
                        --source;
                    }
                    cnv->toULength=2;

                    
                    pArgs->source=(const char *)source;
                    pArgs->target=target;
                    pArgs->offsets=offsets;
                    return;
                }
            }
        } while(length>0);
        cnv->toULength=(int8_t)count;
    }

    
    count=2*targetCapacity;
    if(count>length) {
        count=length&~1;
    }
    if(c==0 && count>0) {
        length-=count;
        count>>=1;
        targetCapacity-=count;
        if(offsets==NULL) {
            do {
                c=((UChar)source[0]<<8)|source[1];
                source+=2;
                if(U16_IS_SINGLE(c)) {
                    *target++=c;
                } else if(U16_IS_SURROGATE_LEAD(c) && count>=2 &&
                          U16_IS_TRAIL(trail=((UChar)source[0]<<8)|source[1])
                ) {
                    source+=2;
                    --count;
                    *target++=c;
                    *target++=trail;
                } else {
                    break;
                }
            } while(--count>0);
        } else {
            do {
                c=((UChar)source[0]<<8)|source[1];
                source+=2;
                if(U16_IS_SINGLE(c)) {
                    *target++=c;
                    *offsets++=sourceIndex;
                    sourceIndex+=2;
                } else if(U16_IS_SURROGATE_LEAD(c) && count>=2 &&
                          U16_IS_TRAIL(trail=((UChar)source[0]<<8)|source[1])
                ) {
                    source+=2;
                    --count;
                    *target++=c;
                    *target++=trail;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex;
                    sourceIndex+=4;
                } else {
                    break;
                }
            } while(--count>0);
        }

        if(count==0) {
            
            c=0;
        } else {
            
            length+=2*(count-1); 
            targetCapacity+=count;
        }
    }

    if(c!=0) {
        




        cnv->toUBytes[0]=(uint8_t)(c>>8);
        cnv->toUBytes[1]=(uint8_t)c;
        cnv->toULength=2;

        if(U16_IS_SURROGATE_LEAD(c)) {
            if(length>=2) {
                if(U16_IS_TRAIL(trail=((UChar)source[0]<<8)|source[1])) {
                    
                    source+=2;
                    length-=2;
                    *target++=c;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                    cnv->UCharErrorBuffer[0]=trail;
                    cnv->UCharErrorBufferLength=1;
                    cnv->toULength=0;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                } else {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                }
            } else {
                
            }
        } else {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
        }
    }

    if(U_SUCCESS(*pErrorCode)) {
        
        if(length>0) {
            if(targetCapacity==0) {
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            } else {
                
                cnv->toUBytes[cnv->toULength++]=*source++;
            }
        }
    }

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
}

static UChar32
_UTF16BEGetNextUChar(UConverterToUnicodeArgs *pArgs, UErrorCode *err) {
    const uint8_t *s, *sourceLimit;
    UChar32 c;

    if(pArgs->converter->mode<8) {
        return UCNV_GET_NEXT_UCHAR_USE_TO_U;
    }

    s=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;

    if(s>=sourceLimit) {
        
        *err=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0xffff;
    }

    if(s+2>sourceLimit) {
        
        pArgs->converter->toUBytes[0]=*s++;
        pArgs->converter->toULength=1;
        pArgs->source=(const char *)s;
        *err = U_TRUNCATED_CHAR_FOUND;
        return 0xffff;
    }

    
    c=((UChar32)*s<<8)|s[1];
    s+=2;

    
    if(U_IS_SURROGATE(c)) {
        if(U16_IS_SURROGATE_LEAD(c)) {
            if(s+2<=sourceLimit) {
                UChar trail;

                
                trail=((UChar)*s<<8)|s[1];
                if(U16_IS_TRAIL(trail)) {
                    c=U16_GET_SUPPLEMENTARY(c, trail);
                    s+=2;
                } else {
                    
                    c=-2;
                }
            } else {
                
                uint8_t *bytes=pArgs->converter->toUBytes;
                s-=2;
                pArgs->converter->toULength=(int8_t)(sourceLimit-s);
                do {
                    *bytes++=*s++;
                } while(s<sourceLimit);

                c=0xffff;
                *err=U_TRUNCATED_CHAR_FOUND;
            }
        } else {
            
            c=-2;
        }

        if(c<0) {
            
            uint8_t *bytes=pArgs->converter->toUBytes;
            pArgs->converter->toULength=2;
            *bytes=*(s-2);
            bytes[1]=*(s-1);

            c=0xffff;
            *err=U_ILLEGAL_CHAR_FOUND;
        }
    }

    pArgs->source=(const char *)s;
    return c;
} 

static void
_UTF16BEReset(UConverter *cnv, UConverterResetChoice choice) {
    if(choice<=UCNV_RESET_TO_UNICODE) {
        
        if(UCNV_GET_VERSION(cnv)==0) {
            cnv->mode=8; 
        } else {
            cnv->mode=0; 
        }
    }
    if(choice!=UCNV_RESET_TO_UNICODE && UCNV_GET_VERSION(cnv)==1) {
        
        cnv->fromUnicodeStatus=UCNV_NEED_TO_WRITE_BOM;
    }
}

static void
_UTF16BEOpen(UConverter *cnv,
             UConverterLoadArgs *pArgs,
             UErrorCode *pErrorCode) {
    if(UCNV_GET_VERSION(cnv)<=1) {
        _UTF16BEReset(cnv, UCNV_RESET_BOTH);
    } else {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    }
}

static const char *
_UTF16BEGetName(const UConverter *cnv) {
    if(UCNV_GET_VERSION(cnv)==0) {
        return "UTF-16BE";
    } else {
        return "UTF-16BE,version=1";
    }
}

static const UConverterImpl _UTF16BEImpl={
    UCNV_UTF16_BigEndian,

    NULL,
    NULL,

    _UTF16BEOpen,
    NULL,
    _UTF16BEReset,

    _UTF16BEToUnicodeWithOffsets,
    _UTF16BEToUnicodeWithOffsets,
    _UTF16BEFromUnicodeWithOffsets,
    _UTF16BEFromUnicodeWithOffsets,
    _UTF16BEGetNextUChar,

    NULL,
    _UTF16BEGetName,
    NULL,
    NULL,
    ucnv_getNonSurrogateUnicodeSet
};

static const UConverterStaticData _UTF16BEStaticData={
    sizeof(UConverterStaticData),
    "UTF-16BE",
    1200, UCNV_IBM, UCNV_UTF16_BigEndian, 2, 2,
    { 0xff, 0xfd, 0, 0 },2,FALSE,FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};


const UConverterSharedData _UTF16BEData={
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_UTF16BEStaticData, FALSE, &_UTF16BEImpl, 
    0
};



static void
_UTF16LEFromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                               UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source;
    char *target;
    int32_t *offsets;

    uint32_t targetCapacity, length, sourceIndex;
    UChar c, trail;
    char overflow[4];

    source=pArgs->source;
    length=(int32_t)(pArgs->sourceLimit-source);
    if(length<=0) {
        
        return;
    }

    cnv=pArgs->converter;

    
    if(cnv->fromUnicodeStatus==UCNV_NEED_TO_WRITE_BOM) {
        static const char bom[]={ (char)0xff, (char)0xfe };
        ucnv_fromUWriteBytes(cnv,
                             bom, 2,
                             &pArgs->target, pArgs->targetLimit,
                             &pArgs->offsets, -1,
                             pErrorCode);
        cnv->fromUnicodeStatus=0;
    }

    target=pArgs->target;
    if(target >= pArgs->targetLimit) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        return;
    }

    targetCapacity=(uint32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;
    sourceIndex=0;

    

    if((c=(UChar)cnv->fromUChar32)!=0 && U16_IS_TRAIL(trail=*source) && targetCapacity>=4) {
        
        ++source;
        --length;
        target[0]=(uint8_t)c;
        target[1]=(uint8_t)(c>>8);
        target[2]=(uint8_t)trail;
        target[3]=(uint8_t)(trail>>8);
        target+=4;
        targetCapacity-=4;
        if(offsets!=NULL) {
            *offsets++=-1;
            *offsets++=-1;
            *offsets++=-1;
            *offsets++=-1;
        }
        sourceIndex=1;
        cnv->fromUChar32=c=0;
    }

    if(c==0) {
        
        uint32_t count=2*length;
        if(count>targetCapacity) {
            count=targetCapacity&~1;
        }
        
        targetCapacity-=count;
        count>>=1;
        length-=count;

        if(offsets==NULL) {
            while(count>0) {
                c=*source++;
                if(U16_IS_SINGLE(c)) {
                    target[0]=(uint8_t)c;
                    target[1]=(uint8_t)(c>>8);
                    target+=2;
                } else if(U16_IS_SURROGATE_LEAD(c) && count>=2 && U16_IS_TRAIL(trail=*source)) {
                    ++source;
                    --count;
                    target[0]=(uint8_t)c;
                    target[1]=(uint8_t)(c>>8);
                    target[2]=(uint8_t)trail;
                    target[3]=(uint8_t)(trail>>8);
                    target+=4;
                } else {
                    break;
                }
                --count;
            }
        } else {
            while(count>0) {
                c=*source++;
                if(U16_IS_SINGLE(c)) {
                    target[0]=(uint8_t)c;
                    target[1]=(uint8_t)(c>>8);
                    target+=2;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex++;
                } else if(U16_IS_SURROGATE_LEAD(c) && count>=2 && U16_IS_TRAIL(trail=*source)) {
                    ++source;
                    --count;
                    target[0]=(uint8_t)c;
                    target[1]=(uint8_t)(c>>8);
                    target[2]=(uint8_t)trail;
                    target[3]=(uint8_t)(trail>>8);
                    target+=4;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex;
                    sourceIndex+=2;
                } else {
                    break;
                }
                --count;
            }
        }

        if(count==0) {
            
            if(length>0 && targetCapacity>0) {
                





                if(U16_IS_SINGLE(c=*source++)) {
                    overflow[0]=(char)c;
                    overflow[1]=(char)(c>>8);
                    length=2; 
                    c=0;
                
                }
            } else {
                length=0;
                c=0;
            }
        } else {
            
            targetCapacity+=2*count;
        }
    } else {
        length=0; 
    }
    
    if(c!=0) {
        




        length=0;
        if(U16_IS_SURROGATE_LEAD(c)) {
            if(source<pArgs->sourceLimit) {
                if(U16_IS_TRAIL(trail=*source)) {
                    
                    ++source;
                    overflow[0]=(char)c;
                    overflow[1]=(char)(c>>8);
                    overflow[2]=(char)trail;
                    overflow[3]=(char)(trail>>8);
                    length=4; 
                    c=0;
                } else {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                }
            } else {
                
            }
        } else {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
        }
        cnv->fromUChar32=c;
    }

    if(length>0) {
        
        ucnv_fromUWriteBytes(cnv,
                             overflow, length,
                             &target, pArgs->targetLimit,
                             &offsets, sourceIndex,
                             pErrorCode);
        targetCapacity=(uint32_t)(pArgs->targetLimit-(char *)target);
    }

    if(U_SUCCESS(*pErrorCode) && source<pArgs->sourceLimit && targetCapacity==0) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
    }

    
    pArgs->source=source;
    pArgs->target=target;
    pArgs->offsets=offsets;
}

static void
_UTF16LEToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                             UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source;
    UChar *target;
    int32_t *offsets;

    uint32_t targetCapacity, length, count, sourceIndex;
    UChar c, trail;

    if(pArgs->converter->mode<8) {
        _UTF16ToUnicodeWithOffsets(pArgs, pErrorCode);
        return;
    }

    cnv=pArgs->converter;
    source=(const uint8_t *)pArgs->source;
    length=(int32_t)((const uint8_t *)pArgs->sourceLimit-source);
    if(length<=0 && cnv->toUnicodeStatus==0) {
        
        return;
    }

    target=pArgs->target;
    if(target >= pArgs->targetLimit) {
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        return;
    }

    targetCapacity=(uint32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;
    sourceIndex=0;
    c=0;

    
    if(cnv->toUnicodeStatus!=0) {
        





        cnv->toUBytes[0]=(uint8_t)cnv->toUnicodeStatus;
        cnv->toULength=1;
        cnv->toUnicodeStatus=0;
    }
    if((count=cnv->toULength)!=0) {
        uint8_t *p=cnv->toUBytes;
        do {
            p[count++]=*source++;
            ++sourceIndex;
            --length;
            if(count==2) {
                c=((UChar)p[1]<<8)|p[0];
                if(U16_IS_SINGLE(c)) {
                    
                    *target++=c;
                    if(offsets!=NULL) {
                        *offsets++=-1;
                    }
                    --targetCapacity;
                    count=0;
                    c=0;
                    break;
                } else if(U16_IS_SURROGATE_LEAD(c)) {
                    
                    c=0; 
                } else {
                    
                    break;
                }
            } else if(count==4) {
                c=((UChar)p[1]<<8)|p[0];
                trail=((UChar)p[3]<<8)|p[2];
                if(U16_IS_TRAIL(trail)) {
                    
                    *target++=c;
                    if(targetCapacity>=2) {
                        *target++=trail;
                        if(offsets!=NULL) {
                            *offsets++=-1;
                            *offsets++=-1;
                        }
                        targetCapacity-=2;
                    } else  {
                        targetCapacity=0;
                        cnv->UCharErrorBuffer[0]=trail;
                        cnv->UCharErrorBufferLength=1;
                        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    }
                    count=0;
                    c=0;
                    break;
                } else {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;

                    
                    if(((const uint8_t *)pArgs->source-source)>=2) {
                        source-=2;
                    } else {
                        




                        cnv->toUnicodeStatus=0x100|p[2];
                        --source;
                    }
                    cnv->toULength=2;

                    
                    pArgs->source=(const char *)source;
                    pArgs->target=target;
                    pArgs->offsets=offsets;
                    return;
                }
            }
        } while(length>0);
        cnv->toULength=(int8_t)count;
    }

    
    count=2*targetCapacity;
    if(count>length) {
        count=length&~1;
    }
    if(c==0 && count>0) {
        length-=count;
        count>>=1;
        targetCapacity-=count;
        if(offsets==NULL) {
            do {
                c=((UChar)source[1]<<8)|source[0];
                source+=2;
                if(U16_IS_SINGLE(c)) {
                    *target++=c;
                } else if(U16_IS_SURROGATE_LEAD(c) && count>=2 &&
                          U16_IS_TRAIL(trail=((UChar)source[1]<<8)|source[0])
                ) {
                    source+=2;
                    --count;
                    *target++=c;
                    *target++=trail;
                } else {
                    break;
                }
            } while(--count>0);
        } else {
            do {
                c=((UChar)source[1]<<8)|source[0];
                source+=2;
                if(U16_IS_SINGLE(c)) {
                    *target++=c;
                    *offsets++=sourceIndex;
                    sourceIndex+=2;
                } else if(U16_IS_SURROGATE_LEAD(c) && count>=2 &&
                          U16_IS_TRAIL(trail=((UChar)source[1]<<8)|source[0])
                ) {
                    source+=2;
                    --count;
                    *target++=c;
                    *target++=trail;
                    *offsets++=sourceIndex;
                    *offsets++=sourceIndex;
                    sourceIndex+=4;
                } else {
                    break;
                }
            } while(--count>0);
        }

        if(count==0) {
            
            c=0;
        } else {
            
            length+=2*(count-1); 
            targetCapacity+=count;
        }
    }

    if(c!=0) {
        




        cnv->toUBytes[0]=(uint8_t)c;
        cnv->toUBytes[1]=(uint8_t)(c>>8);
        cnv->toULength=2;

        if(U16_IS_SURROGATE_LEAD(c)) {
            if(length>=2) {
                if(U16_IS_TRAIL(trail=((UChar)source[1]<<8)|source[0])) {
                    
                    source+=2;
                    length-=2;
                    *target++=c;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                    cnv->UCharErrorBuffer[0]=trail;
                    cnv->UCharErrorBufferLength=1;
                    cnv->toULength=0;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                } else {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                }
            } else {
                
            }
        } else {
            
            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
        }
    }

    if(U_SUCCESS(*pErrorCode)) {
        
        if(length>0) {
            if(targetCapacity==0) {
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            } else {
                
                cnv->toUBytes[cnv->toULength++]=*source++;
            }
        }
    }

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
}

static UChar32
_UTF16LEGetNextUChar(UConverterToUnicodeArgs *pArgs, UErrorCode *err) {
    const uint8_t *s, *sourceLimit;
    UChar32 c;

    if(pArgs->converter->mode<8) {
        return UCNV_GET_NEXT_UCHAR_USE_TO_U;
    }

    s=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;

    if(s>=sourceLimit) {
        
        *err=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0xffff;
    }

    if(s+2>sourceLimit) {
        
        pArgs->converter->toUBytes[0]=*s++;
        pArgs->converter->toULength=1;
        pArgs->source=(const char *)s;
        *err = U_TRUNCATED_CHAR_FOUND;
        return 0xffff;
    }

    
    c=((UChar32)s[1]<<8)|*s;
    s+=2;

    
    if(U_IS_SURROGATE(c)) {
        if(U16_IS_SURROGATE_LEAD(c)) {
            if(s+2<=sourceLimit) {
                UChar trail;

                
                trail=((UChar)s[1]<<8)|*s;
                if(U16_IS_TRAIL(trail)) {
                    c=U16_GET_SUPPLEMENTARY(c, trail);
                    s+=2;
                } else {
                    
                    c=-2;
                }
            } else {
                
                uint8_t *bytes=pArgs->converter->toUBytes;
                s-=2;
                pArgs->converter->toULength=(int8_t)(sourceLimit-s);
                do {
                    *bytes++=*s++;
                } while(s<sourceLimit);

                c=0xffff;
                *err=U_TRUNCATED_CHAR_FOUND;
            }
        } else {
            
            c=-2;
        }

        if(c<0) {
            
            uint8_t *bytes=pArgs->converter->toUBytes;
            pArgs->converter->toULength=2;
            *bytes=*(s-2);
            bytes[1]=*(s-1);

            c=0xffff;
            *err=U_ILLEGAL_CHAR_FOUND;
        }
    }

    pArgs->source=(const char *)s;
    return c;
} 

static void
_UTF16LEReset(UConverter *cnv, UConverterResetChoice choice) {
    if(choice<=UCNV_RESET_TO_UNICODE) {
        
        if(UCNV_GET_VERSION(cnv)==0) {
            cnv->mode=8; 
        } else {
            cnv->mode=0; 
        }
    }
    if(choice!=UCNV_RESET_TO_UNICODE && UCNV_GET_VERSION(cnv)==1) {
        
        cnv->fromUnicodeStatus=UCNV_NEED_TO_WRITE_BOM;
    }
}

static void
_UTF16LEOpen(UConverter *cnv,
             UConverterLoadArgs *pArgs,
             UErrorCode *pErrorCode) {
    if(UCNV_GET_VERSION(cnv)<=1) {
        _UTF16LEReset(cnv, UCNV_RESET_BOTH);
    } else {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    }
}

static const char *
_UTF16LEGetName(const UConverter *cnv) {
    if(UCNV_GET_VERSION(cnv)==0) {
        return "UTF-16LE";
    } else {
        return "UTF-16LE,version=1";
    }
}

static const UConverterImpl _UTF16LEImpl={
    UCNV_UTF16_LittleEndian,

    NULL,
    NULL,

    _UTF16LEOpen,
    NULL,
    _UTF16LEReset,

    _UTF16LEToUnicodeWithOffsets,
    _UTF16LEToUnicodeWithOffsets,
    _UTF16LEFromUnicodeWithOffsets,
    _UTF16LEFromUnicodeWithOffsets,
    _UTF16LEGetNextUChar,

    NULL,
    _UTF16LEGetName,
    NULL,
    NULL,
    ucnv_getNonSurrogateUnicodeSet
};


static const UConverterStaticData _UTF16LEStaticData={
    sizeof(UConverterStaticData),
    "UTF-16LE",
    1202, UCNV_IBM, UCNV_UTF16_LittleEndian, 2, 2,
    { 0xfd, 0xff, 0, 0 },2,FALSE,FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};


const UConverterSharedData _UTF16LEData={
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_UTF16LEStaticData, FALSE, &_UTF16LEImpl, 
    0
};



























static void
_UTF16Reset(UConverter *cnv, UConverterResetChoice choice) {
    if(choice<=UCNV_RESET_TO_UNICODE) {
        
        cnv->mode=0;
    }
    if(choice!=UCNV_RESET_TO_UNICODE) {
        
        cnv->fromUnicodeStatus=UCNV_NEED_TO_WRITE_BOM;
    }
}

static const UConverterSharedData _UTF16v2Data;

static void
_UTF16Open(UConverter *cnv,
           UConverterLoadArgs *pArgs,
           UErrorCode *pErrorCode) {
    if(UCNV_GET_VERSION(cnv)<=2) {
        if(UCNV_GET_VERSION(cnv)==2 && !pArgs->onlyTestIsLoadable) {
            





            cnv->sharedData=(UConverterSharedData*)&_UTF16v2Data;
            uprv_memcpy(cnv->subChars, _UTF16v2Data.staticData->subChar, UCNV_MAX_SUBCHAR_LEN);
        }
        _UTF16Reset(cnv, UCNV_RESET_BOTH);
    } else {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    }
}

static const char *
_UTF16GetName(const UConverter *cnv) {
    if(UCNV_GET_VERSION(cnv)==0) {
        return "UTF-16";
    } else if(UCNV_GET_VERSION(cnv)==1) {
        return "UTF-16,version=1";
    } else {
        return "UTF-16,version=2";
    }
}

const UConverterSharedData _UTF16Data;

#define IS_UTF16BE(cnv) ((cnv)->sharedData==&_UTF16BEData)
#define IS_UTF16LE(cnv) ((cnv)->sharedData==&_UTF16LEData)
#define IS_UTF16(cnv) ((cnv)->sharedData==&_UTF16Data || (cnv)->sharedData==&_UTF16v2Data)

static void
_UTF16ToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                           UErrorCode *pErrorCode) {
    UConverter *cnv=pArgs->converter;
    const char *source=pArgs->source;
    const char *sourceLimit=pArgs->sourceLimit;
    int32_t *offsets=pArgs->offsets;

    int32_t state, offsetDelta;
    uint8_t b;

    state=cnv->mode;

    




    offsetDelta=0;

    while(source<sourceLimit && U_SUCCESS(*pErrorCode)) {
        switch(state) {
        case 0:
            cnv->toUBytes[0]=(uint8_t)*source++;
            cnv->toULength=1;
            state=1;
            break;
        case 1:
            





            b=*source;
            if(cnv->toUBytes[0]==0xfe && b==0xff) {
                if(IS_UTF16LE(cnv)) {
                    state=7; 
                } else {
                    state=8; 
                }
            } else if(cnv->toUBytes[0]==0xff && b==0xfe) {
                if(IS_UTF16BE(cnv)) {
                    state=6; 
                } else {
                    state=9; 
                }
            } else if((IS_UTF16(cnv) && UCNV_GET_VERSION(cnv)==1)) {
                state=6; 
            }
            if(state>=8) {
                
                ++source;
                cnv->toULength=0;
                offsetDelta=(int32_t)(source-pArgs->source);
            } else if(state<6) {
                
                if(source!=pArgs->source) {
                    
                    source=pArgs->source;
                    cnv->toULength=0;
                }
                if(IS_UTF16LE(cnv)) {
                    
                    state=9;
                } else {
                    
                    state=8;
                }
            } else {
                





                
                cnv->toUBytes[1]=b;
                cnv->toULength=2;
                pArgs->source=source+1;
                
                




                cnv->mode=state+2;
                *pErrorCode=U_ILLEGAL_ESCAPE_SEQUENCE;
                return;
            }
            
            cnv->mode=state;
            continue;
        case 8:
            
            pArgs->source=source;
            _UTF16BEToUnicodeWithOffsets(pArgs, pErrorCode);
            source=pArgs->source;
            break;
        case 9:
            
            pArgs->source=source;
            _UTF16LEToUnicodeWithOffsets(pArgs, pErrorCode);
            source=pArgs->source;
            break;
        default:
            break; 
        }
    }

    
    if(offsets!=NULL && offsetDelta!=0) {
        int32_t *offsetsLimit=pArgs->offsets;
        while(offsets<offsetsLimit) {
            *offsets++ += offsetDelta;
        }
    }

    pArgs->source=source;

    if(source==sourceLimit && pArgs->flush) {
        
        switch(state) {
        case 0:
            break; 
        case 8:
            _UTF16BEToUnicodeWithOffsets(pArgs, pErrorCode);
            break;
        case 9:
            _UTF16LEToUnicodeWithOffsets(pArgs, pErrorCode);
            break;
        default:
            
            break;
        }
    }

    cnv->mode=state;
}

static UChar32
_UTF16GetNextUChar(UConverterToUnicodeArgs *pArgs,
                   UErrorCode *pErrorCode) {
    switch(pArgs->converter->mode) {
    case 8:
        return _UTF16BEGetNextUChar(pArgs, pErrorCode);
    case 9:
        return _UTF16LEGetNextUChar(pArgs, pErrorCode);
    default:
        return UCNV_GET_NEXT_UCHAR_USE_TO_U;
    }
}

static const UConverterImpl _UTF16Impl = {
    UCNV_UTF16,

    NULL,
    NULL,

    _UTF16Open,
    NULL,
    _UTF16Reset,

    _UTF16ToUnicodeWithOffsets,
    _UTF16ToUnicodeWithOffsets,
    _UTF16PEFromUnicodeWithOffsets,
    _UTF16PEFromUnicodeWithOffsets,
    _UTF16GetNextUChar,

    NULL, 
    _UTF16GetName,
    NULL,
    NULL,
    ucnv_getNonSurrogateUnicodeSet
};

static const UConverterStaticData _UTF16StaticData = {
    sizeof(UConverterStaticData),
    "UTF-16",
    1204, 
    UCNV_IBM, UCNV_UTF16, 2, 2,
#if U_IS_BIG_ENDIAN
    { 0xff, 0xfd, 0, 0 }, 2,
#else
    { 0xfd, 0xff, 0, 0 }, 2,
#endif
    FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

const UConverterSharedData _UTF16Data = {
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_UTF16StaticData, FALSE, &_UTF16Impl, 
    0
};

static const UConverterImpl _UTF16v2Impl = {
    UCNV_UTF16,

    NULL,
    NULL,

    _UTF16Open,
    NULL,
    _UTF16Reset,

    _UTF16ToUnicodeWithOffsets,
    _UTF16ToUnicodeWithOffsets,
    _UTF16BEFromUnicodeWithOffsets,
    _UTF16BEFromUnicodeWithOffsets,
    _UTF16GetNextUChar,

    NULL, 
    _UTF16GetName,
    NULL,
    NULL,
    ucnv_getNonSurrogateUnicodeSet
};

static const UConverterStaticData _UTF16v2StaticData = {
    sizeof(UConverterStaticData),
    "UTF-16,version=2",
    1204, 
    UCNV_IBM, UCNV_UTF16, 2, 2,
    { 0xff, 0xfd, 0, 0 }, 2,
    FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

static const UConverterSharedData _UTF16v2Data = {
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_UTF16v2StaticData, FALSE, &_UTF16v2Impl, 
    0
};

#endif
