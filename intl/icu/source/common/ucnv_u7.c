















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "uassert.h"













































#define inSetD(c) \
    ((uint8_t)((c)-97)<26 || (uint8_t)((c)-65)<26 || /* letters */ \
     (uint8_t)((c)-48)<10 ||    /* digits */ \
     (uint8_t)((c)-39)<3 ||     /* '() */ \
     (uint8_t)((c)-44)<4 ||     /* ,-./ */ \
     (c)==58 || (c)==63         /* :? */ \
    )

#define inSetO(c) \
    ((uint8_t)((c)-33)<6 ||         /* !"#$%& */ \
     (uint8_t)((c)-59)<4 ||         /* ;<=> */ \
     (uint8_t)((c)-93)<4 ||         /* ]^_` */ \
     (uint8_t)((c)-123)<3 ||        /* {|} */ \
     (c)==42 || (c)==64 || (c)==91  /* *@[ */ \
    )

#define isCRLFTAB(c) ((c)==13 || (c)==10 || (c)==9)
#define isCRLFSPTAB(c) ((c)==32 || (c)==13 || (c)==10 || (c)==9)

#define PLUS  43
#define MINUS 45
#define BACKSLASH 92
#define TILDE 126


#define isLegalUTF7(c) (((uint8_t)((c)-32)<94 && (c)!=BACKSLASH) || isCRLFTAB(c))


static const UBool encodeDirectlyMaximum[128]={
 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0
};


static const UBool encodeDirectlyRestricted[128]={
 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,

    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,

    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

static const uint8_t
toBase64[64]={
    
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77,
    78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
    
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
    110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
    
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    
    43, 47
};

static const int8_t
fromBase64[128]={
    
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -1, -1, -3, -3, -1, -3, -3,
    -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,

    
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -2, -1, 63,
    
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,

    
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -3, -1, -1, -1,

    
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -3, -3
};

















static void
_UTF7Reset(UConverter *cnv, UConverterResetChoice choice) {
    if(choice<=UCNV_RESET_TO_UNICODE) {
        
        cnv->toUnicodeStatus=0x1000000; 
        cnv->toULength=0;
    }
    if(choice!=UCNV_RESET_TO_UNICODE) {
        
        cnv->fromUnicodeStatus=(cnv->fromUnicodeStatus&0xf0000000)|0x1000000; 
    }
}

static void
_UTF7Open(UConverter *cnv,
          UConverterLoadArgs *pArgs,
          UErrorCode *pErrorCode) {
    if(UCNV_GET_VERSION(cnv)<=1) {
        
        cnv->fromUnicodeStatus=UCNV_GET_VERSION(cnv)<<28;
        _UTF7Reset(cnv, UCNV_RESET_BOTH);
    } else {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
    }
}

static void
_UTF7ToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                          UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;
    int32_t *offsets;

    uint8_t *bytes;
    uint8_t byteIndex;

    int32_t length, targetCapacity;

    
    uint16_t bits;
    int8_t base64Counter;
    UBool inDirectMode;

    int8_t base64Value;

    int32_t sourceIndex, nextSourceIndex;

    uint8_t b;
    
    cnv=pArgs->converter;

    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;
    offsets=pArgs->offsets;
    
    {
        uint32_t status=cnv->toUnicodeStatus;
        inDirectMode=(UBool)((status>>24)&1);
        base64Counter=(int8_t)(status>>16);
        bits=(uint16_t)status;
    }
    bytes=cnv->toUBytes;
    byteIndex=cnv->toULength;

    
    sourceIndex=byteIndex==0 ? 0 : -1;
    nextSourceIndex=0;

    if(inDirectMode) {
directMode:
        







        byteIndex=0;
        length=(int32_t)(sourceLimit-source);
        targetCapacity=(int32_t)(targetLimit-target);
        if(length>targetCapacity) {
            length=targetCapacity;
        }
        while(length>0) {
            b=*source++;
            if(!isLegalUTF7(b)) {
                
                bytes[0]=b;
                byteIndex=1;
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                break;
            } else if(b!=PLUS) {
                
                *target++=b;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex++;
                }
            } else  {
                
                nextSourceIndex=++sourceIndex;
                inDirectMode=FALSE;
                byteIndex=0;
                bits=0;
                base64Counter=-1;
                goto unicodeMode;
            }
            --length;
        }
        if(source<sourceLimit && target>=targetLimit) {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
unicodeMode:
        









        while(source<sourceLimit) {
            if(target<targetLimit) {
                bytes[byteIndex++]=b=*source++;
                ++nextSourceIndex;
                base64Value = -3; 
                if(b>=126 || (base64Value=fromBase64[b])==-3 || base64Value==-1) {
                    












                    inDirectMode=TRUE;
                    if(base64Counter==-1) {
                        
                        
                        --source;
                        bytes[0]=PLUS;
                        byteIndex=1;
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        break;
                    } else if(bits!=0) {
                        
                        
                        --source;
                        --byteIndex;
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        break;
                    } else {
                        
                        if(base64Value==-3) {
                            
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            break;
                        } else {
                            
                            --source;
                            sourceIndex=nextSourceIndex-1;
                            goto directMode;
                        }
                    }
                } else if(base64Value>=0) {
                    
                    switch(base64Counter) {
                    case -1: 
                    case 0:
                        bits=base64Value;
                        base64Counter=1;
                        break;
                    case 1:
                    case 3:
                    case 4:
                    case 6:
                        bits=(uint16_t)((bits<<6)|base64Value);
                        ++base64Counter;
                        break;
                    case 2:
                        *target++=(UChar)((bits<<4)|(base64Value>>2));
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex-1;
                        }
                        bytes[0]=b; 
                        byteIndex=1;
                        bits=(uint16_t)(base64Value&3);
                        base64Counter=3;
                        break;
                    case 5:
                        *target++=(UChar)((bits<<2)|(base64Value>>4));
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex-1;
                        }
                        bytes[0]=b; 
                        byteIndex=1;
                        bits=(uint16_t)(base64Value&15);
                        base64Counter=6;
                        break;
                    case 7:
                        *target++=(UChar)((bits<<6)|base64Value);
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex;
                        }
                        byteIndex=0;
                        bits=0;
                        base64Counter=0;
                        break;
                    default:
                        
                        break;
                    }
                } else  {
                    
                    inDirectMode=TRUE;
                    if(base64Counter==-1) {
                        
                        *target++=PLUS;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    } else {
                        
                        if(bits!=0) {
                            
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            break;
                        }
                    }
                    sourceIndex=nextSourceIndex;
                    goto directMode;
                }
            } else {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
    }

    if(U_SUCCESS(*pErrorCode) && pArgs->flush && source==sourceLimit && bits==0) {
        





        byteIndex=0;
    }

    
    cnv->toUnicodeStatus=((uint32_t)inDirectMode<<24)|((uint32_t)((uint8_t)base64Counter)<<16)|(uint32_t)bits;
    cnv->toULength=byteIndex;

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
    return;
}

static void
_UTF7FromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                            UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target, *targetLimit;
    int32_t *offsets;

    int32_t length, targetCapacity, sourceIndex;
    UChar c;

    
    const UBool *encodeDirectly;
    uint8_t bits;
    int8_t base64Counter;
    UBool inDirectMode;

    
    cnv=pArgs->converter;

    
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetLimit=(uint8_t *)pArgs->targetLimit;
    offsets=pArgs->offsets;

    
    {
        uint32_t status=cnv->fromUnicodeStatus;
        encodeDirectly= status<0x10000000 ? encodeDirectlyMaximum : encodeDirectlyRestricted;
        inDirectMode=(UBool)((status>>24)&1);
        base64Counter=(int8_t)(status>>16);
        bits=(uint8_t)status;
        U_ASSERT(bits<=sizeof(toBase64)/sizeof(toBase64[0]));
    }

    
    sourceIndex=0;

    if(inDirectMode) {
directMode:
        length=(int32_t)(sourceLimit-source);
        targetCapacity=(int32_t)(targetLimit-target);
        if(length>targetCapacity) {
            length=targetCapacity;
        }
        while(length>0) {
            c=*source++;
            
            if(c<=127 && encodeDirectly[c]) {
                
                *target++=(uint8_t)c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex++;
                }
            } else if(c==PLUS) {
                
                *target++=PLUS;
                if(target<targetLimit) {
                    *target++=MINUS;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                        *offsets++=sourceIndex++;
                    }
                    
                    goto directMode;
                } else {
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex++;
                    }
                    cnv->charErrorBuffer[0]=MINUS;
                    cnv->charErrorBufferLength=1;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            } else {
                
                --source;
                *target++=PLUS;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                inDirectMode=FALSE;
                base64Counter=0;
                goto unicodeMode;
            }
            --length;
        }
        if(source<sourceLimit && target>=targetLimit) {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
unicodeMode:
        while(source<sourceLimit) {
            if(target<targetLimit) {
                c=*source++;
                if(c<=127 && encodeDirectly[c]) {
                    
                    inDirectMode=TRUE;

                    
                    --source;

                    
                    if(base64Counter!=0) {
                        
                        *target++=toBase64[bits];
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    }
                    if(fromBase64[c]!=-1) {
                        
                        if(target<targetLimit) {
                            *target++=MINUS;
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex-1;
                            }
                        } else {
                            cnv->charErrorBuffer[0]=MINUS;
                            cnv->charErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            break;
                        }
                    }
                    goto directMode;
                } else {
                    








                    switch(base64Counter) {
                    case 0:
                        *target++=toBase64[c>>10];
                        if(target<targetLimit) {
                            *target++=toBase64[(c>>4)&0x3f];
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                                *offsets++=sourceIndex++;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            cnv->charErrorBuffer[0]=toBase64[(c>>4)&0x3f];
                            cnv->charErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=(uint8_t)((c&15)<<2);
                        base64Counter=1;
                        break;
                    case 1:
                        *target++=toBase64[bits|(c>>14)];
                        if(target<targetLimit) {
                            *target++=toBase64[(c>>8)&0x3f];
                            if(target<targetLimit) {
                                *target++=toBase64[(c>>2)&0x3f];
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                            } else {
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                                cnv->charErrorBuffer[0]=toBase64[(c>>2)&0x3f];
                                cnv->charErrorBufferLength=1;
                                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            cnv->charErrorBuffer[0]=toBase64[(c>>8)&0x3f];
                            cnv->charErrorBuffer[1]=toBase64[(c>>2)&0x3f];
                            cnv->charErrorBufferLength=2;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=(uint8_t)((c&3)<<4);
                        base64Counter=2;
                        break;
                    case 2:
                        *target++=toBase64[bits|(c>>12)];
                        if(target<targetLimit) {
                            *target++=toBase64[(c>>6)&0x3f];
                            if(target<targetLimit) {
                                *target++=toBase64[c&0x3f];
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                            } else {
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                                cnv->charErrorBuffer[0]=toBase64[c&0x3f];
                                cnv->charErrorBufferLength=1;
                                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            cnv->charErrorBuffer[0]=toBase64[(c>>6)&0x3f];
                            cnv->charErrorBuffer[1]=toBase64[c&0x3f];
                            cnv->charErrorBufferLength=2;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=0;
                        base64Counter=0;
                        break;
                    default:
                        
                        break;
                    }
                }
            } else {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
    }

    if(pArgs->flush && source>=sourceLimit) {
        
        if(!inDirectMode) {
            if (base64Counter!=0) {
                if(target<targetLimit) {
                    *target++=toBase64[bits];
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex-1;
                    }
                } else {
                    cnv->charErrorBuffer[cnv->charErrorBufferLength++]=toBase64[bits];
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                }
            }
            
            if(target<targetLimit) {
                *target++=MINUS;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex-1;
                }
            } else {
                cnv->charErrorBuffer[cnv->charErrorBufferLength++]=MINUS;
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            }
        }
        
        cnv->fromUnicodeStatus=(cnv->fromUnicodeStatus&0xf0000000)|0x1000000; 
    } else {
        
        cnv->fromUnicodeStatus=
            (cnv->fromUnicodeStatus&0xf0000000)|    
            ((uint32_t)inDirectMode<<24)|((uint32_t)base64Counter<<16)|(uint32_t)bits;
    }

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
    return;
}

static const char *
_UTF7GetName(const UConverter *cnv) {
    switch(cnv->fromUnicodeStatus>>28) {
    case 1:
        return "UTF-7,version=1";
    default:
        return "UTF-7";
    }
}

static const UConverterImpl _UTF7Impl={
    UCNV_UTF7,

    NULL,
    NULL,

    _UTF7Open,
    NULL,
    _UTF7Reset,

    _UTF7ToUnicodeWithOffsets,
    _UTF7ToUnicodeWithOffsets,
    _UTF7FromUnicodeWithOffsets,
    _UTF7FromUnicodeWithOffsets,
    NULL,

    NULL,
    _UTF7GetName,
    NULL, 
    NULL,
    ucnv_getCompleteUnicodeSet
};

static const UConverterStaticData _UTF7StaticData={
    sizeof(UConverterStaticData),
    "UTF-7",
    0, 
    UCNV_IBM, UCNV_UTF7,
    1, 4,
    { 0x3f, 0, 0, 0 }, 1, 
    FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

const UConverterSharedData _UTF7Data={
    sizeof(UConverterSharedData), ~((uint32_t)0),
    NULL, NULL, &_UTF7StaticData, FALSE, &_UTF7Impl,
    0
};











































































#define AMPERSAND 0x26
#define COMMA 0x2c
#define SLASH 0x2f


#define isLegalIMAP(c) (0x20<=(c) && (c)<=0x7e)


#define inSetDIMAP(c) (isLegalIMAP(c) && c!=AMPERSAND)

#define TO_BASE64_IMAP(n) ((n)<63 ? toBase64[n] : COMMA)
#define FROM_BASE64_IMAP(c) ((c)==COMMA ? 63 : (c)==SLASH ? -1 : fromBase64[c])

















static void
_IMAPToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                          UErrorCode *pErrorCode) {
    UConverter *cnv;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;
    int32_t *offsets;

    uint8_t *bytes;
    uint8_t byteIndex;

    int32_t length, targetCapacity;

    
    uint16_t bits;
    int8_t base64Counter;
    UBool inDirectMode;

    int8_t base64Value;

    int32_t sourceIndex, nextSourceIndex;

    UChar c;
    uint8_t b;

    
    cnv=pArgs->converter;

    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;
    offsets=pArgs->offsets;
    
    {
        uint32_t status=cnv->toUnicodeStatus;
        inDirectMode=(UBool)((status>>24)&1);
        base64Counter=(int8_t)(status>>16);
        bits=(uint16_t)status;
    }
    bytes=cnv->toUBytes;
    byteIndex=cnv->toULength;

    
    sourceIndex=byteIndex==0 ? 0 : -1;
    nextSourceIndex=0;

    if(inDirectMode) {
directMode:
        






        byteIndex=0;
        length=(int32_t)(sourceLimit-source);
        targetCapacity=(int32_t)(targetLimit-target);
        if(length>targetCapacity) {
            length=targetCapacity;
        }
        while(length>0) {
            b=*source++;
            if(!isLegalIMAP(b)) {
                
                bytes[0]=b;
                byteIndex=1;
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                break;
            } else if(b!=AMPERSAND) {
                
                *target++=b;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex++;
                }
            } else  {
                
                nextSourceIndex=++sourceIndex;
                inDirectMode=FALSE;
                byteIndex=0;
                bits=0;
                base64Counter=-1;
                goto unicodeMode;
            }
            --length;
        }
        if(source<sourceLimit && target>=targetLimit) {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
unicodeMode:
        










        while(source<sourceLimit) {
            if(target<targetLimit) {
                bytes[byteIndex++]=b=*source++;
                ++nextSourceIndex;
                if(b>0x7e) {
                    
                    inDirectMode=TRUE;
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    break;
                } else if((base64Value=FROM_BASE64_IMAP(b))>=0) {
                    
                    switch(base64Counter) {
                    case -1: 
                    case 0:
                        bits=base64Value;
                        base64Counter=1;
                        break;
                    case 1:
                    case 3:
                    case 4:
                    case 6:
                        bits=(uint16_t)((bits<<6)|base64Value);
                        ++base64Counter;
                        break;
                    case 2:
                        c=(UChar)((bits<<4)|(base64Value>>2));
                        if(isLegalIMAP(c)) {
                            
                            inDirectMode=TRUE;
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                        *target++=c;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex-1;
                        }
                        bytes[0]=b; 
                        byteIndex=1;
                        bits=(uint16_t)(base64Value&3);
                        base64Counter=3;
                        break;
                    case 5:
                        c=(UChar)((bits<<2)|(base64Value>>4));
                        if(isLegalIMAP(c)) {
                            
                            inDirectMode=TRUE;
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                        *target++=c;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex-1;
                        }
                        bytes[0]=b; 
                        byteIndex=1;
                        bits=(uint16_t)(base64Value&15);
                        base64Counter=6;
                        break;
                    case 7:
                        c=(UChar)((bits<<6)|base64Value);
                        if(isLegalIMAP(c)) {
                            
                            inDirectMode=TRUE;
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                        *target++=c;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                            sourceIndex=nextSourceIndex;
                        }
                        byteIndex=0;
                        bits=0;
                        base64Counter=0;
                        break;
                    default:
                        
                        break;
                    }
                } else if(base64Value==-2) {
                    
                    inDirectMode=TRUE;
                    if(base64Counter==-1) {
                        
                        *target++=AMPERSAND;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    } else {
                        
                        if(bits!=0 || (base64Counter!=0 && base64Counter!=3 && base64Counter!=6)) {
                            
                            
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            break;
                        }
                    }
                    sourceIndex=nextSourceIndex;
                    goto directMode;
                } else {
                    if(base64Counter==-1) {
                        
                        
                        --sourceIndex;
                        bytes[0]=AMPERSAND;
                        bytes[1]=b;
                        byteIndex=2;
                    }
                    
                    
                    
                    inDirectMode=TRUE;
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    break;
                }
            } else {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
    }
endloop:

    









    if( U_SUCCESS(*pErrorCode) &&
        !inDirectMode && byteIndex==0 &&
        pArgs->flush && source>=sourceLimit
    ) {
        if(base64Counter==-1) {
            
            
            bytes[0]=AMPERSAND;
            byteIndex=1;
        }
        

        inDirectMode=TRUE; 
        *pErrorCode=U_TRUNCATED_CHAR_FOUND;
    }

    
    cnv->toUnicodeStatus=((uint32_t)inDirectMode<<24)|((uint32_t)((uint8_t)base64Counter)<<16)|(uint32_t)bits;
    cnv->toULength=byteIndex;

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
    return;
}

static void
_IMAPFromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                            UErrorCode *pErrorCode) {
    UConverter *cnv;
    const UChar *source, *sourceLimit;
    uint8_t *target, *targetLimit;
    int32_t *offsets;

    int32_t length, targetCapacity, sourceIndex;
    UChar c;
    uint8_t b;

    
    uint8_t bits;
    int8_t base64Counter;
    UBool inDirectMode;

    
    cnv=pArgs->converter;

    
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetLimit=(uint8_t *)pArgs->targetLimit;
    offsets=pArgs->offsets;

    
    {
        uint32_t status=cnv->fromUnicodeStatus;
        inDirectMode=(UBool)((status>>24)&1);
        base64Counter=(int8_t)(status>>16);
        bits=(uint8_t)status;
    }

    
    sourceIndex=0;

    if(inDirectMode) {
directMode:
        length=(int32_t)(sourceLimit-source);
        targetCapacity=(int32_t)(targetLimit-target);
        if(length>targetCapacity) {
            length=targetCapacity;
        }
        while(length>0) {
            c=*source++;
            
            if(inSetDIMAP(c)) {
                
                *target++=(uint8_t)c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex++;
                }
            } else if(c==AMPERSAND) {
                
                *target++=AMPERSAND;
                if(target<targetLimit) {
                    *target++=MINUS;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                        *offsets++=sourceIndex++;
                    }
                    
                    goto directMode;
                } else {
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex++;
                    }
                    cnv->charErrorBuffer[0]=MINUS;
                    cnv->charErrorBufferLength=1;
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            } else {
                
                --source;
                *target++=AMPERSAND;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                inDirectMode=FALSE;
                base64Counter=0;
                goto unicodeMode;
            }
            --length;
        }
        if(source<sourceLimit && target>=targetLimit) {
            
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
unicodeMode:
        while(source<sourceLimit) {
            if(target<targetLimit) {
                c=*source++;
                if(isLegalIMAP(c)) {
                    
                    inDirectMode=TRUE;

                    
                    --source;

                    
                    if(base64Counter!=0) {
                        
                        *target++=TO_BASE64_IMAP(bits);
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    }
                    
                    if(target<targetLimit) {
                        *target++=MINUS;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex-1;
                        }
                    } else {
                        cnv->charErrorBuffer[0]=MINUS;
                        cnv->charErrorBufferLength=1;
                        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        break;
                    }
                    goto directMode;
                } else {
                    








                    switch(base64Counter) {
                    case 0:
                        b=(uint8_t)(c>>10);
                        *target++=TO_BASE64_IMAP(b);
                        if(target<targetLimit) {
                            b=(uint8_t)((c>>4)&0x3f);
                            *target++=TO_BASE64_IMAP(b);
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                                *offsets++=sourceIndex++;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            b=(uint8_t)((c>>4)&0x3f);
                            cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                            cnv->charErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=(uint8_t)((c&15)<<2);
                        base64Counter=1;
                        break;
                    case 1:
                        b=(uint8_t)(bits|(c>>14));
                        *target++=TO_BASE64_IMAP(b);
                        if(target<targetLimit) {
                            b=(uint8_t)((c>>8)&0x3f);
                            *target++=TO_BASE64_IMAP(b);
                            if(target<targetLimit) {
                                b=(uint8_t)((c>>2)&0x3f);
                                *target++=TO_BASE64_IMAP(b);
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                            } else {
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                                b=(uint8_t)((c>>2)&0x3f);
                                cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                                cnv->charErrorBufferLength=1;
                                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            b=(uint8_t)((c>>8)&0x3f);
                            cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                            b=(uint8_t)((c>>2)&0x3f);
                            cnv->charErrorBuffer[1]=TO_BASE64_IMAP(b);
                            cnv->charErrorBufferLength=2;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=(uint8_t)((c&3)<<4);
                        base64Counter=2;
                        break;
                    case 2:
                        b=(uint8_t)(bits|(c>>12));
                        *target++=TO_BASE64_IMAP(b);
                        if(target<targetLimit) {
                            b=(uint8_t)((c>>6)&0x3f);
                            *target++=TO_BASE64_IMAP(b);
                            if(target<targetLimit) {
                                b=(uint8_t)(c&0x3f);
                                *target++=TO_BASE64_IMAP(b);
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                            } else {
                                if(offsets!=NULL) {
                                    *offsets++=sourceIndex;
                                    *offsets++=sourceIndex++;
                                }
                                b=(uint8_t)(c&0x3f);
                                cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                                cnv->charErrorBufferLength=1;
                                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            }
                        } else {
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex++;
                            }
                            b=(uint8_t)((c>>6)&0x3f);
                            cnv->charErrorBuffer[0]=TO_BASE64_IMAP(b);
                            b=(uint8_t)(c&0x3f);
                            cnv->charErrorBuffer[1]=TO_BASE64_IMAP(b);
                            cnv->charErrorBufferLength=2;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        }
                        bits=0;
                        base64Counter=0;
                        break;
                    default:
                        
                        break;
                    }
                }
            } else {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
        }
    }

    if(pArgs->flush && source>=sourceLimit) {
        
        if(!inDirectMode) {
            if(base64Counter!=0) {
                if(target<targetLimit) {
                    *target++=TO_BASE64_IMAP(bits);
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex-1;
                    }
                } else {
                    cnv->charErrorBuffer[cnv->charErrorBufferLength++]=TO_BASE64_IMAP(bits);
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                }
            }
            
            if(target<targetLimit) {
                *target++=MINUS;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex-1;
                }
            } else {
                cnv->charErrorBuffer[cnv->charErrorBufferLength++]=MINUS;
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            }
        }
        
        cnv->fromUnicodeStatus=(cnv->fromUnicodeStatus&0xf0000000)|0x1000000; 
    } else {
        
        cnv->fromUnicodeStatus=
            (cnv->fromUnicodeStatus&0xf0000000)|    
            ((uint32_t)inDirectMode<<24)|((uint32_t)base64Counter<<16)|(uint32_t)bits;
    }

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
    return;
}

static const UConverterImpl _IMAPImpl={
    UCNV_IMAP_MAILBOX,

    NULL,
    NULL,

    _UTF7Open,
    NULL,
    _UTF7Reset,

    _IMAPToUnicodeWithOffsets,
    _IMAPToUnicodeWithOffsets,
    _IMAPFromUnicodeWithOffsets,
    _IMAPFromUnicodeWithOffsets,
    NULL,

    NULL,
    NULL,
    NULL, 
    NULL,
    ucnv_getCompleteUnicodeSet
};

static const UConverterStaticData _IMAPStaticData={
    sizeof(UConverterStaticData),
    "IMAP-mailbox-name",
    0, 
    UCNV_IBM, UCNV_IMAP_MAILBOX,
    1, 4,
    { 0x3f, 0, 0, 0 }, 1, 
    FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

const UConverterSharedData _IMAPData={
    sizeof(UConverterSharedData), ~((uint32_t)0),
    NULL, NULL, &_IMAPStaticData, FALSE, &_IMAPImpl,
    0
};

#endif
