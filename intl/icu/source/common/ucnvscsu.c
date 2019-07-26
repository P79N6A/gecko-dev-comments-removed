




















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "unicode/ucnv_cb.h"
#include "unicode/utf16.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "cmemory.h"




enum {
    SQ0=0x01, 
    SQ7=0x08, 
    SDX=0x0B, 
    Srs=0x0C, 
    SQU=0x0E, 
    SCU=0x0F, 
    SC0=0x10, 
    SC7=0x17, 
    SD0=0x18, 
    SD7=0x1F, 

    UC0=0xE0, 
    UC7=0xE7, 
    UD0=0xE8, 
    UD7=0xEF, 
    UQU=0xF0, 
    UDX=0xF1, 
    Urs=0xF2  
};

enum {
    




    gapThreshold=0x68,
    gapOffset=0xAC00,

    
    reservedStart=0xA8,

    
    fixedThreshold=0xF9
};


static const uint32_t staticOffsets[8]={
    0x0000, 
    0x0080, 
    0x0100, 
    0x0300, 
    0x2000, 
    0x2080, 
    0x2100, 
    0x3000  
};


static const uint32_t initialDynamicOffsets[8]={
    0x0080, 
    0x00C0, 
    0x0400, 
    0x0600, 
    0x0900, 
    0x3040, 
    0x30A0, 
    0xFF00  
};


static const uint32_t fixedOffsets[]={
     0x00C0, 
     0x0250, 
     0x0370, 
     0x0530, 
     0x3040, 
     0x30A0, 
     0xFF60  
};


enum {
    readCommand,
    quotePairOne,
    quotePairTwo,
    quoteOne,
    definePairOne,
    definePairTwo,
    defineOne
};

typedef struct SCSUData {
    
    uint32_t toUDynamicOffsets[8];
    uint32_t fromUDynamicOffsets[8];

    
    UBool toUIsSingleByteMode;
    uint8_t toUState;
    int8_t toUQuoteWindow, toUDynamicWindow;
    uint8_t toUByteOne;
    uint8_t toUPadding[3];

    
    UBool fromUIsSingleByteMode;
    int8_t fromUDynamicWindow;

    






    uint8_t locale;
    int8_t nextWindowUseIndex;
    int8_t windowUse[8];
} SCSUData;

static const int8_t initialWindowUse[8]={ 7, 0, 3, 2, 4, 5, 6, 1 };
static const int8_t initialWindowUse_ja[8]={ 3, 2, 4, 1, 0, 7, 5, 6 };

enum {
    lGeneric, l_ja
};



static void
_SCSUReset(UConverter *cnv, UConverterResetChoice choice) {
    SCSUData *scsu=(SCSUData *)cnv->extraInfo;

    if(choice<=UCNV_RESET_TO_UNICODE) {
        
        uprv_memcpy(scsu->toUDynamicOffsets, initialDynamicOffsets, 32);

        scsu->toUIsSingleByteMode=TRUE;
        scsu->toUState=readCommand;
        scsu->toUQuoteWindow=scsu->toUDynamicWindow=0;
        scsu->toUByteOne=0;

        cnv->toULength=0;
    }
    if(choice!=UCNV_RESET_TO_UNICODE) {
        
        uprv_memcpy(scsu->fromUDynamicOffsets, initialDynamicOffsets, 32);

        scsu->fromUIsSingleByteMode=TRUE;
        scsu->fromUDynamicWindow=0;

        scsu->nextWindowUseIndex=0;
        switch(scsu->locale) {
        case l_ja:
            uprv_memcpy(scsu->windowUse, initialWindowUse_ja, 8);
            break;
        default:
            uprv_memcpy(scsu->windowUse, initialWindowUse, 8);
            break;
        }

        cnv->fromUChar32=0;
    }
}

static void
_SCSUOpen(UConverter *cnv,
          UConverterLoadArgs *pArgs,
          UErrorCode *pErrorCode) {
    const char *locale=pArgs->locale;
    if(pArgs->onlyTestIsLoadable) {
        return;
    }
    cnv->extraInfo=uprv_malloc(sizeof(SCSUData));
    if(cnv->extraInfo!=NULL) {
        if(locale!=NULL && locale[0]=='j' && locale[1]=='a' && (locale[2]==0 || locale[2]=='_')) {
            ((SCSUData *)cnv->extraInfo)->locale=l_ja;
        } else {
            ((SCSUData *)cnv->extraInfo)->locale=lGeneric;
        }
        _SCSUReset(cnv, UCNV_RESET_BOTH);
    } else {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
    }

    
    cnv->subUChars[0]=0xfffd;
    cnv->subCharLen=-1;
}

static void
_SCSUClose(UConverter *cnv) {
    if(cnv->extraInfo!=NULL) {
        if(!cnv->isExtraLocal) {
            uprv_free(cnv->extraInfo);
        }
        cnv->extraInfo=NULL;
    }
}



static void
_SCSUToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                          UErrorCode *pErrorCode) {
    UConverter *cnv;
    SCSUData *scsu;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;
    int32_t *offsets;
    UBool isSingleByteMode;
    uint8_t state, byteOne;
    int8_t quoteWindow, dynamicWindow;

    int32_t sourceIndex, nextSourceIndex;

    uint8_t b;

    
    cnv=pArgs->converter;
    scsu=(SCSUData *)cnv->extraInfo;

    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;
    offsets=pArgs->offsets;

    
    isSingleByteMode=scsu->toUIsSingleByteMode;
    state=scsu->toUState;
    quoteWindow=scsu->toUQuoteWindow;
    dynamicWindow=scsu->toUDynamicWindow;
    byteOne=scsu->toUByteOne;

    
    sourceIndex=state==readCommand ? 0 : -1;
    nextSourceIndex=0;

    


















    if(isSingleByteMode) {
        
        if(state==readCommand) {
fastSingle:
            while(source<sourceLimit && target<targetLimit && (b=*source)>=0x20) {
                ++source;
                ++nextSourceIndex;
                if(b<=0x7f) {
                    
                    *target++=(UChar)b;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                } else {
                    
                    uint32_t c=scsu->toUDynamicOffsets[dynamicWindow]+(b&0x7f);
                    if(c<=0xffff) {
                        *target++=(UChar)c;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                        }
                    } else {
                        
                        *target++=(UChar)(0xd7c0+(c>>10));
                        if(target<targetLimit) {
                            *target++=(UChar)(0xdc00|(c&0x3ff));
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                                *offsets++=sourceIndex;
                            }
                        } else {
                            
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                            }
                            cnv->UCharErrorBuffer[0]=(UChar)(0xdc00|(c&0x3ff));
                            cnv->UCharErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            goto endloop;
                        }
                    }
                }
                sourceIndex=nextSourceIndex;
            }
        }

        
singleByteMode:
        while(source<sourceLimit) {
            if(target>=targetLimit) {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
            b=*source++;
            ++nextSourceIndex;
            switch(state) {
            case readCommand:
                
                
                if((1UL<<b)&0x2601 ) {
                    
                    *target++=(UChar)b;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                    sourceIndex=nextSourceIndex;
                    goto fastSingle;
                } else if(SC0<=b) {
                    if(b<=SC7) {
                        dynamicWindow=(int8_t)(b-SC0);
                        sourceIndex=nextSourceIndex;
                        goto fastSingle;
                    } else  {
                        dynamicWindow=(int8_t)(b-SD0);
                        state=defineOne;
                    }
                } else if( b<=SQ7) {
                    quoteWindow=(int8_t)(b-SQ0);
                    state=quoteOne;
                } else if(b==SDX) {
                    state=definePairOne;
                } else if(b==SQU) {
                    state=quotePairOne;
                } else if(b==SCU) {
                    sourceIndex=nextSourceIndex;
                    isSingleByteMode=FALSE;
                    goto fastUnicode;
                } else  {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    goto endloop;
                }

                
                cnv->toUBytes[0]=b;
                cnv->toULength=1;
                break;
            case quotePairOne:
                byteOne=b;
                cnv->toUBytes[1]=b;
                cnv->toULength=2;
                state=quotePairTwo;
                break;
            case quotePairTwo:
                *target++=(UChar)((byteOne<<8)|b);
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                sourceIndex=nextSourceIndex;
                state=readCommand;
                goto fastSingle;
            case quoteOne:
                if(b<0x80) {
                    
                    *target++=(UChar)(staticOffsets[quoteWindow]+b);
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                } else {
                    
                    uint32_t c=scsu->toUDynamicOffsets[quoteWindow]+(b&0x7f);
                    if(c<=0xffff) {
                        *target++=(UChar)c;
                        if(offsets!=NULL) {
                            *offsets++=sourceIndex;
                        }
                    } else {
                        
                        *target++=(UChar)(0xd7c0+(c>>10));
                        if(target<targetLimit) {
                            *target++=(UChar)(0xdc00|(c&0x3ff));
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                                *offsets++=sourceIndex;
                            }
                        } else {
                            
                            if(offsets!=NULL) {
                                *offsets++=sourceIndex;
                            }
                            cnv->UCharErrorBuffer[0]=(UChar)(0xdc00|(c&0x3ff));
                            cnv->UCharErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            goto endloop;
                        }
                    }
                }
                sourceIndex=nextSourceIndex;
                state=readCommand;
                goto fastSingle;
            case definePairOne:
                dynamicWindow=(int8_t)((b>>5)&7);
                byteOne=(uint8_t)(b&0x1f);
                cnv->toUBytes[1]=b;
                cnv->toULength=2;
                state=definePairTwo;
                break;
            case definePairTwo:
                scsu->toUDynamicOffsets[dynamicWindow]=0x10000+(byteOne<<15UL | b<<7UL);
                sourceIndex=nextSourceIndex;
                state=readCommand;
                goto fastSingle;
            case defineOne:
                if(b==0) {
                    
                    cnv->toUBytes[1]=b;
                    cnv->toULength=2;
                    goto endloop;
                } else if(b<gapThreshold) {
                    scsu->toUDynamicOffsets[dynamicWindow]=b<<7UL;
                } else if((uint8_t)(b-gapThreshold)<(reservedStart-gapThreshold)) {
                    scsu->toUDynamicOffsets[dynamicWindow]=(b<<7UL)+gapOffset;
                } else if(b>=fixedThreshold) {
                    scsu->toUDynamicOffsets[dynamicWindow]=fixedOffsets[b-fixedThreshold];
                } else {
                    
                    cnv->toUBytes[1]=b;
                    cnv->toULength=2;
                    goto endloop;
                }
                sourceIndex=nextSourceIndex;
                state=readCommand;
                goto fastSingle;
            }
        }
    } else {
        
        if(state==readCommand) {
fastUnicode:
            while(source+1<sourceLimit && target<targetLimit && (uint8_t)((b=*source)-UC0)>(Urs-UC0)) {
                *target++=(UChar)((b<<8)|source[1]);
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                sourceIndex=nextSourceIndex;
                nextSourceIndex+=2;
                source+=2;
            }
        }

        

        while(source<sourceLimit) {
            if(target>=targetLimit) {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
            b=*source++;
            ++nextSourceIndex;
            switch(state) {
            case readCommand:
                if((uint8_t)(b-UC0)>(Urs-UC0)) {
                    byteOne=b;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    state=quotePairTwo;
                } else if( b<=UC7) {
                    dynamicWindow=(int8_t)(b-UC0);
                    sourceIndex=nextSourceIndex;
                    isSingleByteMode=TRUE;
                    goto fastSingle;
                } else if( b<=UD7) {
                    dynamicWindow=(int8_t)(b-UD0);
                    isSingleByteMode=TRUE;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    state=defineOne;
                    goto singleByteMode;
                } else if(b==UDX) {
                    isSingleByteMode=TRUE;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    state=definePairOne;
                    goto singleByteMode;
                } else if(b==UQU) {
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    state=quotePairOne;
                } else  {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    goto endloop;
                }
                break;
            case quotePairOne:
                byteOne=b;
                cnv->toUBytes[1]=b;
                cnv->toULength=2;
                state=quotePairTwo;
                break;
            case quotePairTwo:
                *target++=(UChar)((byteOne<<8)|b);
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                sourceIndex=nextSourceIndex;
                state=readCommand;
                goto fastUnicode;
            }
        }
    }
endloop:

    
    if(U_FAILURE(*pErrorCode) && *pErrorCode!=U_BUFFER_OVERFLOW_ERROR) {
        
        state=readCommand;
    } else if(state==readCommand) {
        
        cnv->toULength=0;
    }
    scsu->toUIsSingleByteMode=isSingleByteMode;
    scsu->toUState=state;
    scsu->toUQuoteWindow=quoteWindow;
    scsu->toUDynamicWindow=dynamicWindow;
    scsu->toUByteOne=byteOne;

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    pArgs->offsets=offsets;
    return;
}








static void
_SCSUToUnicode(UConverterToUnicodeArgs *pArgs,
               UErrorCode *pErrorCode) {
    UConverter *cnv;
    SCSUData *scsu;
    const uint8_t *source, *sourceLimit;
    UChar *target;
    const UChar *targetLimit;
    UBool isSingleByteMode;
    uint8_t state, byteOne;
    int8_t quoteWindow, dynamicWindow;

    uint8_t b;

    
    cnv=pArgs->converter;
    scsu=(SCSUData *)cnv->extraInfo;

    source=(const uint8_t *)pArgs->source;
    sourceLimit=(const uint8_t *)pArgs->sourceLimit;
    target=pArgs->target;
    targetLimit=pArgs->targetLimit;

    
    isSingleByteMode=scsu->toUIsSingleByteMode;
    state=scsu->toUState;
    quoteWindow=scsu->toUQuoteWindow;
    dynamicWindow=scsu->toUDynamicWindow;
    byteOne=scsu->toUByteOne;

    


















    if(isSingleByteMode) {
        
        if(state==readCommand) {
fastSingle:
            while(source<sourceLimit && target<targetLimit && (b=*source)>=0x20) {
                ++source;
                if(b<=0x7f) {
                    
                    *target++=(UChar)b;
                } else {
                    
                    uint32_t c=scsu->toUDynamicOffsets[dynamicWindow]+(b&0x7f);
                    if(c<=0xffff) {
                        *target++=(UChar)c;
                    } else {
                        
                        *target++=(UChar)(0xd7c0+(c>>10));
                        if(target<targetLimit) {
                            *target++=(UChar)(0xdc00|(c&0x3ff));
                        } else {
                            
                            cnv->UCharErrorBuffer[0]=(UChar)(0xdc00|(c&0x3ff));
                            cnv->UCharErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            goto endloop;
                        }
                    }
                }
            }
        }

        
singleByteMode:
        while(source<sourceLimit) {
            if(target>=targetLimit) {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
            b=*source++;
            switch(state) {
            case readCommand:
                
                
                if((1UL<<b)&0x2601 ) {
                    
                    *target++=(UChar)b;
                    goto fastSingle;
                } else if(SC0<=b) {
                    if(b<=SC7) {
                        dynamicWindow=(int8_t)(b-SC0);
                        goto fastSingle;
                    } else  {
                        dynamicWindow=(int8_t)(b-SD0);
                        state=defineOne;
                    }
                } else if( b<=SQ7) {
                    quoteWindow=(int8_t)(b-SQ0);
                    state=quoteOne;
                } else if(b==SDX) {
                    state=definePairOne;
                } else if(b==SQU) {
                    state=quotePairOne;
                } else if(b==SCU) {
                    isSingleByteMode=FALSE;
                    goto fastUnicode;
                } else  {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    goto endloop;
                }

                
                cnv->toUBytes[0]=b;
                cnv->toULength=1;
                break;
            case quotePairOne:
                byteOne=b;
                cnv->toUBytes[1]=b;
                cnv->toULength=2;
                state=quotePairTwo;
                break;
            case quotePairTwo:
                *target++=(UChar)((byteOne<<8)|b);
                state=readCommand;
                goto fastSingle;
            case quoteOne:
                if(b<0x80) {
                    
                    *target++=(UChar)(staticOffsets[quoteWindow]+b);
                } else {
                    
                    uint32_t c=scsu->toUDynamicOffsets[quoteWindow]+(b&0x7f);
                    if(c<=0xffff) {
                        *target++=(UChar)c;
                    } else {
                        
                        *target++=(UChar)(0xd7c0+(c>>10));
                        if(target<targetLimit) {
                            *target++=(UChar)(0xdc00|(c&0x3ff));
                        } else {
                            
                            cnv->UCharErrorBuffer[0]=(UChar)(0xdc00|(c&0x3ff));
                            cnv->UCharErrorBufferLength=1;
                            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                            goto endloop;
                        }
                    }
                }
                state=readCommand;
                goto fastSingle;
            case definePairOne:
                dynamicWindow=(int8_t)((b>>5)&7);
                byteOne=(uint8_t)(b&0x1f);
                cnv->toUBytes[1]=b;
                cnv->toULength=2;
                state=definePairTwo;
                break;
            case definePairTwo:
                scsu->toUDynamicOffsets[dynamicWindow]=0x10000+(byteOne<<15UL | b<<7UL);
                state=readCommand;
                goto fastSingle;
            case defineOne:
                if(b==0) {
                    
                    cnv->toUBytes[1]=b;
                    cnv->toULength=2;
                    goto endloop;
                } else if(b<gapThreshold) {
                    scsu->toUDynamicOffsets[dynamicWindow]=b<<7UL;
                } else if((uint8_t)(b-gapThreshold)<(reservedStart-gapThreshold)) {
                    scsu->toUDynamicOffsets[dynamicWindow]=(b<<7UL)+gapOffset;
                } else if(b>=fixedThreshold) {
                    scsu->toUDynamicOffsets[dynamicWindow]=fixedOffsets[b-fixedThreshold];
                } else {
                    
                    cnv->toUBytes[1]=b;
                    cnv->toULength=2;
                    goto endloop;
                }
                state=readCommand;
                goto fastSingle;
            }
        }
    } else {
        
        if(state==readCommand) {
fastUnicode:
            while(source+1<sourceLimit && target<targetLimit && (uint8_t)((b=*source)-UC0)>(Urs-UC0)) {
                *target++=(UChar)((b<<8)|source[1]);
                source+=2;
            }
        }

        

        while(source<sourceLimit) {
            if(target>=targetLimit) {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
            b=*source++;
            switch(state) {
            case readCommand:
                if((uint8_t)(b-UC0)>(Urs-UC0)) {
                    byteOne=b;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    state=quotePairTwo;
                } else if( b<=UC7) {
                    dynamicWindow=(int8_t)(b-UC0);
                    isSingleByteMode=TRUE;
                    goto fastSingle;
                } else if( b<=UD7) {
                    dynamicWindow=(int8_t)(b-UD0);
                    isSingleByteMode=TRUE;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    state=defineOne;
                    goto singleByteMode;
                } else if(b==UDX) {
                    isSingleByteMode=TRUE;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    state=definePairOne;
                    goto singleByteMode;
                } else if(b==UQU) {
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    state=quotePairOne;
                } else  {
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    cnv->toUBytes[0]=b;
                    cnv->toULength=1;
                    goto endloop;
                }
                break;
            case quotePairOne:
                byteOne=b;
                cnv->toUBytes[1]=b;
                cnv->toULength=2;
                state=quotePairTwo;
                break;
            case quotePairTwo:
                *target++=(UChar)((byteOne<<8)|b);
                state=readCommand;
                goto fastUnicode;
            }
        }
    }
endloop:

    
    if(U_FAILURE(*pErrorCode) && *pErrorCode!=U_BUFFER_OVERFLOW_ERROR) {
        
        state=readCommand;
    } else if(state==readCommand) {
        
        cnv->toULength=0;
    }
    scsu->toUIsSingleByteMode=isSingleByteMode;
    scsu->toUState=state;
    scsu->toUQuoteWindow=quoteWindow;
    scsu->toUDynamicWindow=dynamicWindow;
    scsu->toUByteOne=byteOne;

    
    pArgs->source=(const char *)source;
    pArgs->target=target;
    return;
}















static int8_t
getWindow(const uint32_t offsets[8], uint32_t c) {
    int i;
    for(i=0; i<8; ++i) {
        if((uint32_t)(c-offsets[i])<=0x7f) {
            return (int8_t)(i);
        }
    }
    return -1;
}


static UBool
isInOffsetWindowOrDirect(uint32_t offset, uint32_t c) {
    return (UBool)(c<=offset+0x7f &&
          (c>=offset || (c<=0x7f &&
                        (c>=0x20 || (1UL<<c)&0x2601))));
                                

}




static int8_t
getNextDynamicWindow(SCSUData *scsu) {
    int8_t window=scsu->windowUse[scsu->nextWindowUseIndex];
    if(++scsu->nextWindowUseIndex==8) {
        scsu->nextWindowUseIndex=0;
    }
    return window;
}







static void
useDynamicWindow(SCSUData *scsu, int8_t window) {
    




    
    int i, j;

    i=scsu->nextWindowUseIndex;
    do {
        if(--i<0) {
            i=7;
        }
    } while(scsu->windowUse[i]!=window);

    
    j=i+1;
    if(j==8) {
        j=0;
    }
    while(j!=scsu->nextWindowUseIndex) {
        scsu->windowUse[i]=scsu->windowUse[j];
        i=j;
        if(++j==8) { j=0; }
    }

    
    scsu->windowUse[i]=window;
}









static int
getDynamicOffset(uint32_t c, uint32_t *pOffset) {
    int i;

    for(i=0; i<7; ++i) {
        if((uint32_t)(c-fixedOffsets[i])<=0x7f) {
            *pOffset=fixedOffsets[i];
            return 0xf9+i;
        }
    }

    if(c<0x80) {
        
        return -1;
    } else if(c<0x3400 ||
              (uint32_t)(c-0x10000)<(0x14000-0x10000) ||
              (uint32_t)(c-0x1d000)<=(0x1ffff-0x1d000)
    ) {
        
        *pOffset=c&0x7fffff80;
        return (int)(c>>7);
    } else if(0xe000<=c && c!=0xfeff && c<0xfff0) {
        
        *pOffset=c&0x7fffff80;
        return (int)((c-gapOffset)>>7);
    } else {
        return -1;
    }
}


















static void
_SCSUFromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                            UErrorCode *pErrorCode) {
    UConverter *cnv;
    SCSUData *scsu;
    const UChar *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;
    int32_t *offsets;

    UBool isSingleByteMode;
    uint8_t dynamicWindow;
    uint32_t currentOffset;

    uint32_t c, delta;

    int32_t sourceIndex, nextSourceIndex;

    int32_t length;

    
    uint32_t offset;
    UChar lead, trail;
    int code;
    int8_t window;

    
    cnv=pArgs->converter;
    scsu=(SCSUData *)cnv->extraInfo;

    
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);
    offsets=pArgs->offsets;

    
    isSingleByteMode=scsu->fromUIsSingleByteMode;
    dynamicWindow=scsu->fromUDynamicWindow;
    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];

    c=cnv->fromUChar32;

    
    sourceIndex= c==0 ? 0 : -1;
    nextSourceIndex=0;

    
loop:
    if(isSingleByteMode) {
        if(c!=0 && targetCapacity>0) {
            goto getTrailSingle;
        }

        

        while(source<sourceLimit) {
            if(targetCapacity<=0) {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
            c=*source++;
            ++nextSourceIndex;

            if((c-0x20)<=0x5f) {
                
                *target++=(uint8_t)c;
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                --targetCapacity;
            } else if(c<0x20) {
                if((1UL<<c)&0x2601 ) {
                    
                    *target++=(uint8_t)c;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                    --targetCapacity;
                } else {
                    
                    c|=SQ0<<8;
                    length=2;
                    goto outputBytes;
                }
            } else if((delta=c-currentOffset)<=0x7f) {
                
                *target++=(uint8_t)(delta|0x80);
                if(offsets!=NULL) {
                    *offsets++=sourceIndex;
                }
                --targetCapacity;
            } else if(U16_IS_SURROGATE(c)) {
                if(U16_IS_SURROGATE_LEAD(c)) {
getTrailSingle:
                    lead=(UChar)c;
                    if(source<sourceLimit) {
                        
                        trail=*source;
                        if(U16_IS_TRAIL(trail)) {
                            ++source;
                            ++nextSourceIndex;
                            c=U16_GET_SUPPLEMENTARY(c, trail);
                            
                            
                        } else {
                            
                            
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                    } else {
                        
                        break;
                    }
                } else {
                    
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    goto endloop;
                }

                
                if((delta=c-currentOffset)<=0x7f) {
                    
                    *target++=(uint8_t)(delta|0x80);
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                    --targetCapacity;
                } else if((window=getWindow(scsu->fromUDynamicOffsets, c))>=0) {
                    
                    dynamicWindow=window;
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)(SC0+dynamicWindow)<<8)|(c-currentOffset)|0x80;
                    length=2;
                    goto outputBytes;
                } else if((code=getDynamicOffset(c, &offset))>=0) {
                    
                    
                    code-=0x200;
                    dynamicWindow=getNextDynamicWindow(scsu);
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow]=offset;
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)SDX<<24)|((uint32_t)dynamicWindow<<21)|((uint32_t)code<<8)|(c-currentOffset)|0x80;
                    length=4;
                    goto outputBytes;
                } else {
                    
                    isSingleByteMode=FALSE;
                    *target++=(uint8_t)SCU;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                    }
                    --targetCapacity;
                    c=((uint32_t)lead<<16)|trail;
                    length=4;
                    goto outputBytes;
                }
            } else if(c<0xa0) {
                
                c=(c&0x7f)|(SQ0+1)<<8; 
                length=2;
                goto outputBytes;
            } else if(c==0xfeff || c>=0xfff0) {
                
                c|=SQU<<16;
                length=3;
                goto outputBytes;
            } else {
                
                if((window=getWindow(scsu->fromUDynamicOffsets, c))>=0) {
                    
                    if(source>=sourceLimit || isInOffsetWindowOrDirect(scsu->fromUDynamicOffsets[window], *source)) {
                        
                        dynamicWindow=window;
                        currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];
                        useDynamicWindow(scsu, dynamicWindow);
                        c=((uint32_t)(SC0+dynamicWindow)<<8)|(c-currentOffset)|0x80;
                        length=2;
                        goto outputBytes;
                    } else {
                        
                        c=((uint32_t)(SQ0+window)<<8)|(c-scsu->fromUDynamicOffsets[window])|0x80;
                        length=2;
                        goto outputBytes;
                    }
                } else if((window=getWindow(staticOffsets, c))>=0) {
                    
                    c=((uint32_t)(SQ0+window)<<8)|(c-staticOffsets[window]);
                    length=2;
                    goto outputBytes;
                } else if((code=getDynamicOffset(c, &offset))>=0) {
                    
                    dynamicWindow=getNextDynamicWindow(scsu);
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow]=offset;
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)(SD0+dynamicWindow)<<16)|((uint32_t)code<<8)|(c-currentOffset)|0x80;
                    length=3;
                    goto outputBytes;
                } else if((uint32_t)(c-0x3400)<(0xd800-0x3400) &&
                          (source>=sourceLimit || (uint32_t)(*source-0x3400)<(0xd800-0x3400))
                ) {
                    




                    isSingleByteMode=FALSE;
                    c|=SCU<<16;
                    length=3;
                    goto outputBytes;
                } else {
                    
                    c|=SQU<<16;
                    length=3;
                    goto outputBytes;
                }
            }

            
            c=0;
            sourceIndex=nextSourceIndex;
        }
    } else {
        if(c!=0 && targetCapacity>0) {
            goto getTrailUnicode;
        }

        

        while(source<sourceLimit) {
            if(targetCapacity<=0) {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
            c=*source++;
            ++nextSourceIndex;

            if((uint32_t)(c-0x3400)<(0xd800-0x3400)) {
                
                if(targetCapacity>=2) {
                    *target++=(uint8_t)(c>>8);
                    *target++=(uint8_t)c;
                    if(offsets!=NULL) {
                        *offsets++=sourceIndex;
                        *offsets++=sourceIndex;
                    }
                    targetCapacity-=2;
                } else {
                    length=2;
                    goto outputBytes;
                }
            } else if((uint32_t)(c-0x3400)>=(0xf300-0x3400) ) {
                
                if(!(source<sourceLimit && (uint32_t)(*source-0x3400)<(0xd800-0x3400))) {
                    if(((uint32_t)(c-0x30)<10 || (uint32_t)(c-0x61)<26 || (uint32_t)(c-0x41)<26)) {
                        
                        isSingleByteMode=TRUE;
                        c|=((uint32_t)(UC0+dynamicWindow)<<8)|c;
                        length=2;
                        goto outputBytes;
                    } else if((window=getWindow(scsu->fromUDynamicOffsets, c))>=0) {
                        
                        isSingleByteMode=TRUE;
                        dynamicWindow=window;
                        currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];
                        useDynamicWindow(scsu, dynamicWindow);
                        c=((uint32_t)(UC0+dynamicWindow)<<8)|(c-currentOffset)|0x80;
                        length=2;
                        goto outputBytes;
                    } else if((code=getDynamicOffset(c, &offset))>=0) {
                        
                        isSingleByteMode=TRUE;
                        dynamicWindow=getNextDynamicWindow(scsu);
                        currentOffset=scsu->fromUDynamicOffsets[dynamicWindow]=offset;
                        useDynamicWindow(scsu, dynamicWindow);
                        c=((uint32_t)(UD0+dynamicWindow)<<16)|((uint32_t)code<<8)|(c-currentOffset)|0x80;
                        length=3;
                        goto outputBytes;
                    }
                }

                
                length=2;
                goto outputBytes;
            } else if(c<0xe000) {
                
                if(U16_IS_SURROGATE_LEAD(c)) {
getTrailUnicode:
                    lead=(UChar)c;
                    if(source<sourceLimit) {
                        
                        trail=*source;
                        if(U16_IS_TRAIL(trail)) {
                            ++source;
                            ++nextSourceIndex;
                            c=U16_GET_SUPPLEMENTARY(c, trail);
                            
                            
                        } else {
                            
                            
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                    } else {
                        
                        break;
                    }
                } else {
                    
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    goto endloop;
                }

                
                if( (window=getWindow(scsu->fromUDynamicOffsets, c))>=0 &&
                    !(source<sourceLimit && (uint32_t)(*source-0x3400)<(0xd800-0x3400))
                ) {
                    




                    isSingleByteMode=TRUE;
                    dynamicWindow=window;
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)(UC0+dynamicWindow)<<8)|(c-currentOffset)|0x80;
                    length=2;
                    goto outputBytes;
                } else if(source<sourceLimit && lead==*source && 
                          (code=getDynamicOffset(c, &offset))>=0
                ) {
                    
                    isSingleByteMode=TRUE;
                    code-=0x200;
                    dynamicWindow=getNextDynamicWindow(scsu);
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow]=offset;
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)UDX<<24)|((uint32_t)dynamicWindow<<21)|((uint32_t)code<<8)|(c-currentOffset)|0x80;
                    length=4;
                    goto outputBytes;
                } else {
                    
                    c=((uint32_t)lead<<16)|trail;
                    length=4;
                    goto outputBytes;
                }
            } else  {
                
                c|=UQU<<16;
                length=3;
                goto outputBytes;
            }

            
            c=0;
            sourceIndex=nextSourceIndex;
        }
    }
endloop:

    
    scsu->fromUIsSingleByteMode=isSingleByteMode;
    scsu->fromUDynamicWindow=dynamicWindow;

    cnv->fromUChar32=c;

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    pArgs->offsets=offsets;
    return;

outputBytes:
    
    
    if(length<=targetCapacity) {
        if(offsets==NULL) {
            switch(length) {
                
            case 4:
                *target++=(uint8_t)(c>>24);
            case 3: 
                *target++=(uint8_t)(c>>16);
            case 2: 
                *target++=(uint8_t)(c>>8);
            case 1: 
                *target++=(uint8_t)c;
            default:
                
                break;
            }
        } else {
            switch(length) {
                
            case 4:
                *target++=(uint8_t)(c>>24);
                *offsets++=sourceIndex;
            case 3: 
                *target++=(uint8_t)(c>>16);
                *offsets++=sourceIndex;
            case 2: 
                *target++=(uint8_t)(c>>8);
                *offsets++=sourceIndex;
            case 1: 
                *target++=(uint8_t)c;
                *offsets++=sourceIndex;
            default:
                
                break;
            }
        }
        targetCapacity-=length;

        
        c=0;
        sourceIndex=nextSourceIndex;
        goto loop;
    } else {
        uint8_t *p;

        





        
        
        length-=targetCapacity;
        p=(uint8_t *)cnv->charErrorBuffer;
        switch(length) {
            
        case 4:
            *p++=(uint8_t)(c>>24);
        case 3: 
            *p++=(uint8_t)(c>>16);
        case 2: 
            *p++=(uint8_t)(c>>8);
        case 1: 
            *p=(uint8_t)c;
        default:
            
            break;
        }
        cnv->charErrorBufferLength=(int8_t)length;

        
        c>>=8*length; 
        switch(targetCapacity) {
            
        case 3:
            *target++=(uint8_t)(c>>16);
            if(offsets!=NULL) {
                *offsets++=sourceIndex;
            }
        case 2: 
            *target++=(uint8_t)(c>>8);
            if(offsets!=NULL) {
                *offsets++=sourceIndex;
            }
        case 1: 
            *target++=(uint8_t)c;
            if(offsets!=NULL) {
                *offsets++=sourceIndex;
            }
        default:
            break;
        }

        
        targetCapacity=0;
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        c=0;
        goto endloop;
    }
}








static void
_SCSUFromUnicode(UConverterFromUnicodeArgs *pArgs,
                 UErrorCode *pErrorCode) {
    UConverter *cnv;
    SCSUData *scsu;
    const UChar *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;

    UBool isSingleByteMode;
    uint8_t dynamicWindow;
    uint32_t currentOffset;

    uint32_t c, delta;

    int32_t length;

    
    uint32_t offset;
    UChar lead, trail;
    int code;
    int8_t window;

    
    cnv=pArgs->converter;
    scsu=(SCSUData *)cnv->extraInfo;

    
    source=pArgs->source;
    sourceLimit=pArgs->sourceLimit;
    target=(uint8_t *)pArgs->target;
    targetCapacity=(int32_t)(pArgs->targetLimit-pArgs->target);

    
    isSingleByteMode=scsu->fromUIsSingleByteMode;
    dynamicWindow=scsu->fromUDynamicWindow;
    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];

    c=cnv->fromUChar32;

    
loop:
    if(isSingleByteMode) {
        if(c!=0 && targetCapacity>0) {
            goto getTrailSingle;
        }

        

        while(source<sourceLimit) {
            if(targetCapacity<=0) {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
            c=*source++;

            if((c-0x20)<=0x5f) {
                
                *target++=(uint8_t)c;
                --targetCapacity;
            } else if(c<0x20) {
                if((1UL<<c)&0x2601 ) {
                    
                    *target++=(uint8_t)c;
                    --targetCapacity;
                } else {
                    
                    c|=SQ0<<8;
                    length=2;
                    goto outputBytes;
                }
            } else if((delta=c-currentOffset)<=0x7f) {
                
                *target++=(uint8_t)(delta|0x80);
                --targetCapacity;
            } else if(U16_IS_SURROGATE(c)) {
                if(U16_IS_SURROGATE_LEAD(c)) {
getTrailSingle:
                    lead=(UChar)c;
                    if(source<sourceLimit) {
                        
                        trail=*source;
                        if(U16_IS_TRAIL(trail)) {
                            ++source;
                            c=U16_GET_SUPPLEMENTARY(c, trail);
                            
                            
                        } else {
                            
                            
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                    } else {
                        
                        break;
                    }
                } else {
                    
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    goto endloop;
                }

                
                if((delta=c-currentOffset)<=0x7f) {
                    
                    *target++=(uint8_t)(delta|0x80);
                    --targetCapacity;
                } else if((window=getWindow(scsu->fromUDynamicOffsets, c))>=0) {
                    
                    dynamicWindow=window;
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)(SC0+dynamicWindow)<<8)|(c-currentOffset)|0x80;
                    length=2;
                    goto outputBytes;
                } else if((code=getDynamicOffset(c, &offset))>=0) {
                    
                    
                    code-=0x200;
                    dynamicWindow=getNextDynamicWindow(scsu);
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow]=offset;
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)SDX<<24)|((uint32_t)dynamicWindow<<21)|((uint32_t)code<<8)|(c-currentOffset)|0x80;
                    length=4;
                    goto outputBytes;
                } else {
                    
                    isSingleByteMode=FALSE;
                    *target++=(uint8_t)SCU;
                    --targetCapacity;
                    c=((uint32_t)lead<<16)|trail;
                    length=4;
                    goto outputBytes;
                }
            } else if(c<0xa0) {
                
                c=(c&0x7f)|(SQ0+1)<<8; 
                length=2;
                goto outputBytes;
            } else if(c==0xfeff || c>=0xfff0) {
                
                c|=SQU<<16;
                length=3;
                goto outputBytes;
            } else {
                
                if((window=getWindow(scsu->fromUDynamicOffsets, c))>=0) {
                    
                    if(source>=sourceLimit || isInOffsetWindowOrDirect(scsu->fromUDynamicOffsets[window], *source)) {
                        
                        dynamicWindow=window;
                        currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];
                        useDynamicWindow(scsu, dynamicWindow);
                        c=((uint32_t)(SC0+dynamicWindow)<<8)|(c-currentOffset)|0x80;
                        length=2;
                        goto outputBytes;
                    } else {
                        
                        c=((uint32_t)(SQ0+window)<<8)|(c-scsu->fromUDynamicOffsets[window])|0x80;
                        length=2;
                        goto outputBytes;
                    }
                } else if((window=getWindow(staticOffsets, c))>=0) {
                    
                    c=((uint32_t)(SQ0+window)<<8)|(c-staticOffsets[window]);
                    length=2;
                    goto outputBytes;
                } else if((code=getDynamicOffset(c, &offset))>=0) {
                    
                    dynamicWindow=getNextDynamicWindow(scsu);
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow]=offset;
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)(SD0+dynamicWindow)<<16)|((uint32_t)code<<8)|(c-currentOffset)|0x80;
                    length=3;
                    goto outputBytes;
                } else if((uint32_t)(c-0x3400)<(0xd800-0x3400) &&
                          (source>=sourceLimit || (uint32_t)(*source-0x3400)<(0xd800-0x3400))
                ) {
                    




                    isSingleByteMode=FALSE;
                    c|=SCU<<16;
                    length=3;
                    goto outputBytes;
                } else {
                    
                    c|=SQU<<16;
                    length=3;
                    goto outputBytes;
                }
            }

            
            c=0;
        }
    } else {
        if(c!=0 && targetCapacity>0) {
            goto getTrailUnicode;
        }

        

        while(source<sourceLimit) {
            if(targetCapacity<=0) {
                
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                break;
            }
            c=*source++;

            if((uint32_t)(c-0x3400)<(0xd800-0x3400)) {
                
                if(targetCapacity>=2) {
                    *target++=(uint8_t)(c>>8);
                    *target++=(uint8_t)c;
                    targetCapacity-=2;
                } else {
                    length=2;
                    goto outputBytes;
                }
            } else if((uint32_t)(c-0x3400)>=(0xf300-0x3400) ) {
                
                if(!(source<sourceLimit && (uint32_t)(*source-0x3400)<(0xd800-0x3400))) {
                    if(((uint32_t)(c-0x30)<10 || (uint32_t)(c-0x61)<26 || (uint32_t)(c-0x41)<26)) {
                        
                        isSingleByteMode=TRUE;
                        c|=((uint32_t)(UC0+dynamicWindow)<<8)|c;
                        length=2;
                        goto outputBytes;
                    } else if((window=getWindow(scsu->fromUDynamicOffsets, c))>=0) {
                        
                        isSingleByteMode=TRUE;
                        dynamicWindow=window;
                        currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];
                        useDynamicWindow(scsu, dynamicWindow);
                        c=((uint32_t)(UC0+dynamicWindow)<<8)|(c-currentOffset)|0x80;
                        length=2;
                        goto outputBytes;
                    } else if((code=getDynamicOffset(c, &offset))>=0) {
                        
                        isSingleByteMode=TRUE;
                        dynamicWindow=getNextDynamicWindow(scsu);
                        currentOffset=scsu->fromUDynamicOffsets[dynamicWindow]=offset;
                        useDynamicWindow(scsu, dynamicWindow);
                        c=((uint32_t)(UD0+dynamicWindow)<<16)|((uint32_t)code<<8)|(c-currentOffset)|0x80;
                        length=3;
                        goto outputBytes;
                    }
                }

                
                length=2;
                goto outputBytes;
            } else if(c<0xe000) {
                
                if(U16_IS_SURROGATE_LEAD(c)) {
getTrailUnicode:
                    lead=(UChar)c;
                    if(source<sourceLimit) {
                        
                        trail=*source;
                        if(U16_IS_TRAIL(trail)) {
                            ++source;
                            c=U16_GET_SUPPLEMENTARY(c, trail);
                            
                            
                        } else {
                            
                            
                            *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                            goto endloop;
                        }
                    } else {
                        
                        break;
                    }
                } else {
                    
                    
                    *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                    goto endloop;
                }

                
                if( (window=getWindow(scsu->fromUDynamicOffsets, c))>=0 &&
                    !(source<sourceLimit && (uint32_t)(*source-0x3400)<(0xd800-0x3400))
                ) {
                    




                    isSingleByteMode=TRUE;
                    dynamicWindow=window;
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow];
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)(UC0+dynamicWindow)<<8)|(c-currentOffset)|0x80;
                    length=2;
                    goto outputBytes;
                } else if(source<sourceLimit && lead==*source && 
                          (code=getDynamicOffset(c, &offset))>=0
                ) {
                    
                    isSingleByteMode=TRUE;
                    code-=0x200;
                    dynamicWindow=getNextDynamicWindow(scsu);
                    currentOffset=scsu->fromUDynamicOffsets[dynamicWindow]=offset;
                    useDynamicWindow(scsu, dynamicWindow);
                    c=((uint32_t)UDX<<24)|((uint32_t)dynamicWindow<<21)|((uint32_t)code<<8)|(c-currentOffset)|0x80;
                    length=4;
                    goto outputBytes;
                } else {
                    
                    c=((uint32_t)lead<<16)|trail;
                    length=4;
                    goto outputBytes;
                }
            } else  {
                
                c|=UQU<<16;
                length=3;
                goto outputBytes;
            }

            
            c=0;
        }
    }
endloop:

    
    scsu->fromUIsSingleByteMode=isSingleByteMode;
    scsu->fromUDynamicWindow=dynamicWindow;

    cnv->fromUChar32=c;

    
    pArgs->source=source;
    pArgs->target=(char *)target;
    return;

outputBytes:
    
    
    if(length<=targetCapacity) {
        switch(length) {
            
        case 4:
            *target++=(uint8_t)(c>>24);
        case 3: 
            *target++=(uint8_t)(c>>16);
        case 2: 
            *target++=(uint8_t)(c>>8);
        case 1: 
            *target++=(uint8_t)c;
        default:
            
            break;
        }
        targetCapacity-=length;

        
        c=0;
        goto loop;
    } else {
        uint8_t *p;

        





        
        
        length-=targetCapacity;
        p=(uint8_t *)cnv->charErrorBuffer;
        switch(length) {
            
        case 4:
            *p++=(uint8_t)(c>>24);
        case 3: 
            *p++=(uint8_t)(c>>16);
        case 2: 
            *p++=(uint8_t)(c>>8);
        case 1: 
            *p=(uint8_t)c;
        default:
            
            break;
        }
        cnv->charErrorBufferLength=(int8_t)length;

        
        c>>=8*length; 
        switch(targetCapacity) {
            
        case 3:
            *target++=(uint8_t)(c>>16);
        case 2: 
            *target++=(uint8_t)(c>>8);
        case 1: 
            *target++=(uint8_t)c;
        default:
            break;
        }

        
        targetCapacity=0;
        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        c=0;
        goto endloop;
    }
}



static const char *
_SCSUGetName(const UConverter *cnv) {
    SCSUData *scsu=(SCSUData *)cnv->extraInfo;

    switch(scsu->locale) {
    case l_ja:
        return "SCSU,locale=ja";
    default:
        return "SCSU";
    }
}


struct cloneSCSUStruct
{
    UConverter cnv;
    SCSUData mydata;
};

static UConverter * 
_SCSUSafeClone(const UConverter *cnv, 
               void *stackBuffer, 
               int32_t *pBufferSize, 
               UErrorCode *status)
{
    struct cloneSCSUStruct * localClone;
    int32_t bufferSizeNeeded = sizeof(struct cloneSCSUStruct);

    if (U_FAILURE(*status)){
        return 0;
    }

    if (*pBufferSize == 0){ 
        *pBufferSize = bufferSizeNeeded;
        return 0;
    }

    localClone = (struct cloneSCSUStruct *)stackBuffer;
    

    uprv_memcpy(&localClone->mydata, cnv->extraInfo, sizeof(SCSUData));
    localClone->cnv.extraInfo = &localClone->mydata;
    localClone->cnv.isExtraLocal = TRUE;

    return &localClone->cnv;
}


static const UConverterImpl _SCSUImpl={
    UCNV_SCSU,

    NULL,
    NULL,

    _SCSUOpen,
    _SCSUClose,
    _SCSUReset,

    _SCSUToUnicode,
    _SCSUToUnicodeWithOffsets,
    _SCSUFromUnicode,
    _SCSUFromUnicodeWithOffsets,
    NULL,

    NULL,
    _SCSUGetName,
    NULL,
    _SCSUSafeClone,
    ucnv_getCompleteUnicodeSet
};

static const UConverterStaticData _SCSUStaticData={
    sizeof(UConverterStaticData),
    "SCSU",
    1212, 
    UCNV_IBM, UCNV_SCSU,
    1, 3, 
    



    { 0x0e, 0xff, 0xfd, 0 }, 3,
    FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

const UConverterSharedData _SCSUData={
    sizeof(UConverterSharedData), ~((uint32_t)0),
    NULL, NULL, &_SCSUStaticData, FALSE, &_SCSUImpl,
    0
};

#endif
