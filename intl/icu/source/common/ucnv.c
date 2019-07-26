




















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/ucnv_err.h"
#include "unicode/uset.h"
#include "unicode/utf.h"
#include "unicode/utf16.h"
#include "putilimp.h"
#include "cmemory.h"
#include "cstring.h"
#include "uassert.h"
#include "utracimp.h"
#include "ustr_imp.h"
#include "ucnv_imp.h"
#include "ucnv_cnv.h"
#include "ucnv_bld.h"


#define CHUNK_SIZE 1024

typedef struct UAmbiguousConverter {
    const char *name;
    const UChar variant5c;
} UAmbiguousConverter;

static const UAmbiguousConverter ambiguousConverters[]={
    { "ibm-897_P100-1995", 0xa5 },
    { "ibm-942_P120-1999", 0xa5 },
    { "ibm-943_P130-1999", 0xa5 },
    { "ibm-946_P100-1995", 0xa5 },
    { "ibm-33722_P120-1999", 0xa5 },
    { "ibm-1041_P100-1995", 0xa5 },
    
    
    
    { "ibm-944_P100-1995", 0x20a9 },
    { "ibm-949_P110-1999", 0x20a9 },
    { "ibm-1363_P110-1997", 0x20a9 },
    { "ISO_2022,locale=ko,version=0", 0x20a9 },
    { "ibm-1088_P100-1995", 0x20a9 }
};


U_CAPI UConverter* U_EXPORT2
ucnv_open (const char *name,
                       UErrorCode * err)
{
    UConverter *r;

    if (err == NULL || U_FAILURE (*err)) {
        return NULL;
    }

    r =  ucnv_createConverter(NULL, name, err);
    return r;
}

U_CAPI UConverter* U_EXPORT2 
ucnv_openPackage   (const char *packageName, const char *converterName, UErrorCode * err)
{
    return ucnv_createConverterFromPackage(packageName, converterName,  err);
}


U_CAPI UConverter*   U_EXPORT2
ucnv_openU (const UChar * name,
                         UErrorCode * err)
{
    char asciiName[UCNV_MAX_CONVERTER_NAME_LENGTH];

    if (err == NULL || U_FAILURE(*err))
        return NULL;
    if (name == NULL)
        return ucnv_open (NULL, err);
    if (u_strlen(name) >= UCNV_MAX_CONVERTER_NAME_LENGTH)
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    return ucnv_open(u_austrcpy(asciiName, name), err);
}






static int32_t
ucnv_copyPlatformString(char *platformString, UConverterPlatform pltfrm)
{
    switch (pltfrm)
    {
    case UCNV_IBM:
        uprv_strcpy(platformString, "ibm-");
        return 4;
    case UCNV_UNKNOWN:
        break;
    }

    
    *platformString = 0;
    return 0;
}



U_CAPI UConverter*   U_EXPORT2
ucnv_openCCSID (int32_t codepage,
                UConverterPlatform platform,
                UErrorCode * err)
{
    char myName[UCNV_MAX_CONVERTER_NAME_LENGTH];
    int32_t myNameLen;

    if (err == NULL || U_FAILURE (*err))
        return NULL;

    
    myNameLen = ucnv_copyPlatformString(myName, platform);
    T_CString_integerToString(myName + myNameLen, codepage, 10);

    return ucnv_createConverter(NULL, myName, err);
}





U_CAPI UConverter* U_EXPORT2
ucnv_safeClone(const UConverter* cnv, void *stackBuffer, int32_t *pBufferSize, UErrorCode *status)
{
    UConverter *localConverter, *allocatedConverter;
    int32_t bufferSizeNeeded;
    char *stackBufferChars = (char *)stackBuffer;
    UErrorCode cbErr;
    UConverterToUnicodeArgs toUArgs = {
        sizeof(UConverterToUnicodeArgs),
            TRUE,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
    };
    UConverterFromUnicodeArgs fromUArgs = {
        sizeof(UConverterFromUnicodeArgs),
            TRUE,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
    };

    UTRACE_ENTRY_OC(UTRACE_UCNV_CLONE);

    if (status == NULL || U_FAILURE(*status)){
        UTRACE_EXIT_STATUS(status? *status: U_ILLEGAL_ARGUMENT_ERROR);
        return 0;
    }

    if (!pBufferSize || !cnv){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        UTRACE_EXIT_STATUS(*status);
        return 0;
    }

    UTRACE_DATA3(UTRACE_OPEN_CLOSE, "clone converter %s at %p into stackBuffer %p",
                                    ucnv_getName(cnv, status), cnv, stackBuffer);

    if (cnv->sharedData->impl->safeClone != NULL) {
        
        bufferSizeNeeded = 0;
        cnv->sharedData->impl->safeClone(cnv, NULL, &bufferSizeNeeded, status);
    }
    else
    {
        
        bufferSizeNeeded = sizeof(UConverter);
    }

    if (*pBufferSize <= 0){ 
        *pBufferSize = bufferSizeNeeded;
        UTRACE_EXIT_VALUE(bufferSizeNeeded);
        return 0;
    }


    


    if (U_ALIGNMENT_OFFSET(stackBuffer) != 0) {
        int32_t offsetUp = (int32_t)U_ALIGNMENT_OFFSET_UP(stackBufferChars);
        if(*pBufferSize > offsetUp) {
            *pBufferSize -= offsetUp;
            stackBufferChars += offsetUp;
        } else {
            
            *pBufferSize = 1;
        }
    }

    stackBuffer = (void *)stackBufferChars;
    
    
    if (*pBufferSize < bufferSizeNeeded || stackBuffer == NULL)
    {
        
        localConverter = allocatedConverter = (UConverter *) uprv_malloc (bufferSizeNeeded);

        if(localConverter == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            UTRACE_EXIT_STATUS(*status);
            return NULL;
        }
        
        if (U_SUCCESS(*status)) {
            *status = U_SAFECLONE_ALLOCATED_WARNING;
        }
        
        
        *pBufferSize = bufferSizeNeeded;
    } else {
        
        localConverter = (UConverter*) stackBuffer;
        allocatedConverter = NULL;
    }

    uprv_memset(localConverter, 0, bufferSizeNeeded);

    
    uprv_memcpy(localConverter, cnv, sizeof(UConverter));
    localConverter->isCopyLocal = localConverter->isExtraLocal = FALSE;

    
    if (cnv->subChars == (uint8_t *)cnv->subUChars) {
        localConverter->subChars = (uint8_t *)localConverter->subUChars;
    } else {
        localConverter->subChars = (uint8_t *)uprv_malloc(UCNV_ERROR_BUFFER_LENGTH * U_SIZEOF_UCHAR);
        if (localConverter->subChars == NULL) {
            uprv_free(allocatedConverter);
            UTRACE_EXIT_STATUS(*status);
            return NULL;
        }
        uprv_memcpy(localConverter->subChars, cnv->subChars, UCNV_ERROR_BUFFER_LENGTH * U_SIZEOF_UCHAR);
    }

    
    if (cnv->sharedData->impl->safeClone != NULL) {
        
        localConverter = cnv->sharedData->impl->safeClone(cnv, localConverter, pBufferSize, status);
    }

    if(localConverter==NULL || U_FAILURE(*status)) {
        if (allocatedConverter != NULL && allocatedConverter->subChars != (uint8_t *)allocatedConverter->subUChars) {
            uprv_free(allocatedConverter->subChars);
        }
        uprv_free(allocatedConverter);
        UTRACE_EXIT_STATUS(*status);
        return NULL;
    }

    
    




    if (cnv->sharedData->referenceCounter != ~0) {
        ucnv_incrementRefCount(cnv->sharedData);
    }

    if(localConverter == (UConverter*)stackBuffer) {
        
        localConverter->isCopyLocal = TRUE;
    }

    
    toUArgs.converter = fromUArgs.converter = localConverter;
    cbErr = U_ZERO_ERROR;
    cnv->fromCharErrorBehaviour(cnv->toUContext, &toUArgs, NULL, 0, UCNV_CLONE, &cbErr);
    cbErr = U_ZERO_ERROR;
    cnv->fromUCharErrorBehaviour(cnv->fromUContext, &fromUArgs, NULL, 0, 0, UCNV_CLONE, &cbErr);

    UTRACE_EXIT_PTR_STATUS(localConverter, *status);
    return localConverter;
}






U_CAPI void  U_EXPORT2
ucnv_close (UConverter * converter)
{
    UErrorCode errorCode = U_ZERO_ERROR;

    UTRACE_ENTRY_OC(UTRACE_UCNV_CLOSE);

    if (converter == NULL)
    {
        UTRACE_EXIT();
        return;
    }

    UTRACE_DATA3(UTRACE_OPEN_CLOSE, "close converter %s at %p, isCopyLocal=%b",
        ucnv_getName(converter, &errorCode), converter, converter->isCopyLocal);

    


    
    if (converter->fromCharErrorBehaviour != UCNV_TO_U_DEFAULT_CALLBACK) {
        UConverterToUnicodeArgs toUArgs = {
            sizeof(UConverterToUnicodeArgs),
                TRUE,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL
        };

        toUArgs.converter = converter;
        errorCode = U_ZERO_ERROR;
        converter->fromCharErrorBehaviour(converter->toUContext, &toUArgs, NULL, 0, UCNV_CLOSE, &errorCode);
    }
    if (converter->fromUCharErrorBehaviour != UCNV_FROM_U_DEFAULT_CALLBACK) {
        UConverterFromUnicodeArgs fromUArgs = {
            sizeof(UConverterFromUnicodeArgs),
                TRUE,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL
        };
        fromUArgs.converter = converter;
        errorCode = U_ZERO_ERROR;
        converter->fromUCharErrorBehaviour(converter->fromUContext, &fromUArgs, NULL, 0, 0, UCNV_CLOSE, &errorCode);
    }

    if (converter->sharedData->impl->close != NULL) {
        converter->sharedData->impl->close(converter);
    }

    if (converter->subChars != (uint8_t *)converter->subUChars) {
        uprv_free(converter->subChars);
    }

    




    if (converter->sharedData->referenceCounter != ~0) {
        ucnv_unloadSharedDataIfReady(converter->sharedData);
    }

    if(!converter->isCopyLocal){
        uprv_free(converter);
    }

    UTRACE_EXIT();
}



U_CAPI const char*   U_EXPORT2
ucnv_getAvailableName (int32_t n)
{
    if (0 <= n && n <= 0xffff) {
        UErrorCode err = U_ZERO_ERROR;
        const char *name = ucnv_bld_getAvailableConverter((uint16_t)n, &err);
        if (U_SUCCESS(err)) {
            return name;
        }
    }
    return NULL;
}

U_CAPI int32_t   U_EXPORT2
ucnv_countAvailable ()
{
    UErrorCode err = U_ZERO_ERROR;
    return ucnv_bld_countAvailableConverters(&err);
}

U_CAPI void    U_EXPORT2
ucnv_getSubstChars (const UConverter * converter,
                    char *mySubChar,
                    int8_t * len,
                    UErrorCode * err)
{
    if (U_FAILURE (*err))
        return;

    if (converter->subCharLen <= 0) {
        
        *len = 0;
        return;
    }

    if (*len < converter->subCharLen) 
    {
        *err = U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    }

    uprv_memcpy (mySubChar, converter->subChars, converter->subCharLen);   
    *len = converter->subCharLen; 
}

U_CAPI void    U_EXPORT2
ucnv_setSubstChars (UConverter * converter,
                    const char *mySubChar,
                    int8_t len,
                    UErrorCode * err)
{
    if (U_FAILURE (*err))
        return;
    
    
    if ((len > converter->sharedData->staticData->maxBytesPerChar)
     || (len < converter->sharedData->staticData->minBytesPerChar))
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    uprv_memcpy (converter->subChars, mySubChar, len); 
    converter->subCharLen = len;  

    




    converter->subChar1 = 0;
    
    return;
}

U_CAPI void U_EXPORT2
ucnv_setSubstString(UConverter *cnv,
                    const UChar *s,
                    int32_t length,
                    UErrorCode *err) {
    UAlignedMemory cloneBuffer[U_CNV_SAFECLONE_BUFFERSIZE / sizeof(UAlignedMemory) + 1];
    char chars[UCNV_ERROR_BUFFER_LENGTH];

    UConverter *clone;
    uint8_t *subChars;
    int32_t cloneSize, length8;

    
    cloneSize = sizeof(cloneBuffer);
    clone = ucnv_safeClone(cnv, cloneBuffer, &cloneSize, err);
    ucnv_setFromUCallBack(clone, UCNV_FROM_U_CALLBACK_STOP, NULL, NULL, NULL, err);
    length8 = ucnv_fromUChars(clone, chars, (int32_t)sizeof(chars), s, length, err);
    ucnv_close(clone);
    if (U_FAILURE(*err)) {
        return;
    }

    if (cnv->sharedData->impl->writeSub == NULL
#if !UCONFIG_NO_LEGACY_CONVERSION
        || (cnv->sharedData->staticData->conversionType == UCNV_MBCS &&
         ucnv_MBCSGetType(cnv) != UCNV_EBCDIC_STATEFUL)
#endif
    ) {
        
        subChars = (uint8_t *)chars;
    } else {
        





        if (length > UCNV_ERROR_BUFFER_LENGTH) {
            




            *err = U_BUFFER_OVERFLOW_ERROR;
            return;
        }
        subChars = (uint8_t *)s;
        if (length < 0) {
            length = u_strlen(s);
        }
        length8 = length * U_SIZEOF_UCHAR;
    }

    



    if (length8 > UCNV_MAX_SUBCHAR_LEN) {
        
        if (cnv->subChars == (uint8_t *)cnv->subUChars) {
            
            cnv->subChars = (uint8_t *)uprv_malloc(UCNV_ERROR_BUFFER_LENGTH * U_SIZEOF_UCHAR);
            if (cnv->subChars == NULL) {
                cnv->subChars = (uint8_t *)cnv->subUChars;
                *err = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            uprv_memset(cnv->subChars, 0, UCNV_ERROR_BUFFER_LENGTH * U_SIZEOF_UCHAR);
        }
    }

    
    if (length8 == 0) {
        cnv->subCharLen = 0;
    } else {
        uprv_memcpy(cnv->subChars, subChars, length8);
        if (subChars == (uint8_t *)chars) {
            cnv->subCharLen = (int8_t)length8;
        } else  {
            cnv->subCharLen = (int8_t)-length;
        }
    }

    
    cnv->subChar1 = 0;
}




static void _reset(UConverter *converter, UConverterResetChoice choice,
                   UBool callCallback) {
    if(converter == NULL) {
        return;
    }

    if(callCallback) {
        
        UErrorCode errorCode;

        if(choice<=UCNV_RESET_TO_UNICODE && converter->fromCharErrorBehaviour != UCNV_TO_U_DEFAULT_CALLBACK) {
            UConverterToUnicodeArgs toUArgs = {
                sizeof(UConverterToUnicodeArgs),
                TRUE,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL
            };
            toUArgs.converter = converter;
            errorCode = U_ZERO_ERROR;
            converter->fromCharErrorBehaviour(converter->toUContext, &toUArgs, NULL, 0, UCNV_RESET, &errorCode);
        }
        if(choice!=UCNV_RESET_TO_UNICODE && converter->fromUCharErrorBehaviour != UCNV_FROM_U_DEFAULT_CALLBACK) {
            UConverterFromUnicodeArgs fromUArgs = {
                sizeof(UConverterFromUnicodeArgs),
                TRUE,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL
            };
            fromUArgs.converter = converter;
            errorCode = U_ZERO_ERROR;
            converter->fromUCharErrorBehaviour(converter->fromUContext, &fromUArgs, NULL, 0, 0, UCNV_RESET, &errorCode);
        }
    }

    
    if(choice<=UCNV_RESET_TO_UNICODE) {
        converter->toUnicodeStatus = converter->sharedData->toUnicodeStatus;
        converter->mode = 0;
        converter->toULength = 0;
        converter->invalidCharLength = converter->UCharErrorBufferLength = 0;
        converter->preToULength = 0;
    }
    if(choice!=UCNV_RESET_TO_UNICODE) {
        converter->fromUnicodeStatus = 0;
        converter->fromUChar32 = 0;
        converter->invalidUCharLength = converter->charErrorBufferLength = 0;
        converter->preFromUFirstCP = U_SENTINEL;
        converter->preFromULength = 0;
    }

    if (converter->sharedData->impl->reset != NULL) {
        
        converter->sharedData->impl->reset(converter, choice);
    }
}

U_CAPI void  U_EXPORT2
ucnv_reset(UConverter *converter)
{
    _reset(converter, UCNV_RESET_BOTH, TRUE);
}

U_CAPI void  U_EXPORT2
ucnv_resetToUnicode(UConverter *converter)
{
    _reset(converter, UCNV_RESET_TO_UNICODE, TRUE);
}

U_CAPI void  U_EXPORT2
ucnv_resetFromUnicode(UConverter *converter)
{
    _reset(converter, UCNV_RESET_FROM_UNICODE, TRUE);
}

U_CAPI int8_t   U_EXPORT2
ucnv_getMaxCharSize (const UConverter * converter)
{
    return converter->maxBytesPerUChar;
}


U_CAPI int8_t   U_EXPORT2
ucnv_getMinCharSize (const UConverter * converter)
{
    return converter->sharedData->staticData->minBytesPerChar;
}

U_CAPI const char*   U_EXPORT2
ucnv_getName (const UConverter * converter, UErrorCode * err)
     
{
    if (U_FAILURE (*err))
        return NULL;
    if(converter->sharedData->impl->getName){
        const char* temp= converter->sharedData->impl->getName(converter);
        if(temp)
            return temp;
    }
    return converter->sharedData->staticData->name;
}

U_CAPI int32_t U_EXPORT2
ucnv_getCCSID(const UConverter * converter,
              UErrorCode * err)
{
    int32_t ccsid;
    if (U_FAILURE (*err))
        return -1;

    ccsid = converter->sharedData->staticData->codepage;
    if (ccsid == 0) {
        

        const char *standardName = ucnv_getStandardName(ucnv_getName(converter, err), "IBM", err);
        if (U_SUCCESS(*err) && standardName) {
            const char *ccsidStr = uprv_strchr(standardName, '-');
            if (ccsidStr) {
                ccsid = (int32_t)atol(ccsidStr+1);  
            }
        }
    }
    return ccsid;
}


U_CAPI UConverterPlatform   U_EXPORT2
ucnv_getPlatform (const UConverter * converter,
                                      UErrorCode * err)
{
    if (U_FAILURE (*err))
        return UCNV_UNKNOWN;

    return (UConverterPlatform)converter->sharedData->staticData->platform;
}

U_CAPI void U_EXPORT2
    ucnv_getToUCallBack (const UConverter * converter,
                         UConverterToUCallback *action,
                         const void **context)
{
    *action = converter->fromCharErrorBehaviour;
    *context = converter->toUContext;
}

U_CAPI void U_EXPORT2
    ucnv_getFromUCallBack (const UConverter * converter,
                           UConverterFromUCallback *action,
                           const void **context)
{
    *action = converter->fromUCharErrorBehaviour;
    *context = converter->fromUContext;
}

U_CAPI void    U_EXPORT2
ucnv_setToUCallBack (UConverter * converter,
                            UConverterToUCallback newAction,
                            const void* newContext,
                            UConverterToUCallback *oldAction,
                            const void** oldContext,
                            UErrorCode * err)
{
    if (U_FAILURE (*err))
        return;
    if (oldAction) *oldAction = converter->fromCharErrorBehaviour;
    converter->fromCharErrorBehaviour = newAction;
    if (oldContext) *oldContext = converter->toUContext;
    converter->toUContext = newContext;
}

U_CAPI void  U_EXPORT2
ucnv_setFromUCallBack (UConverter * converter,
                            UConverterFromUCallback newAction,
                            const void* newContext,
                            UConverterFromUCallback *oldAction,
                            const void** oldContext,
                            UErrorCode * err)
{
    if (U_FAILURE (*err))
        return;
    if (oldAction) *oldAction = converter->fromUCharErrorBehaviour;
    converter->fromUCharErrorBehaviour = newAction;
    if (oldContext) *oldContext = converter->fromUContext;
    converter->fromUContext = newContext;
}

static void
_updateOffsets(int32_t *offsets, int32_t length,
               int32_t sourceIndex, int32_t errorInputLength) {
    int32_t *limit;
    int32_t delta, offset;

    if(sourceIndex>=0) {
        




        delta=sourceIndex-errorInputLength;
    } else {
        



        delta=-1;
    }

    limit=offsets+length;
    if(delta==0) {
        
    } else if(delta>0) {
        
        while(offsets<limit) {
            offset=*offsets;
            if(offset>=0) {
                *offsets=offset+delta;
            }
            ++offsets;
        }
    } else  {
        




        while(offsets<limit) {
            *offsets++=-1;
        }
    }
}

























static void
_fromUnicodeWithCallback(UConverterFromUnicodeArgs *pArgs, UErrorCode *err) {
    UConverterFromUnicode fromUnicode;
    UConverter *cnv;
    const UChar *s;
    char *t;
    int32_t *offsets;
    int32_t sourceIndex;
    int32_t errorInputLength;
    UBool converterSawEndOfInput, calledCallback;

    
    UChar replay[UCNV_EXT_MAX_UCHARS];
    const UChar *realSource, *realSourceLimit;
    int32_t realSourceIndex;
    UBool realFlush;

    cnv=pArgs->converter;
    s=pArgs->source;
    t=pArgs->target;
    offsets=pArgs->offsets;

    
    sourceIndex=0;
    if(offsets==NULL) {
        fromUnicode=cnv->sharedData->impl->fromUnicode;
    } else {
        fromUnicode=cnv->sharedData->impl->fromUnicodeWithOffsets;
        if(fromUnicode==NULL) {
            
            fromUnicode=cnv->sharedData->impl->fromUnicode;
            
            sourceIndex=-1;
        }
    }

    if(cnv->preFromULength>=0) {
        
        realSource=NULL;

        
        realSourceLimit=NULL;
        realFlush=FALSE;
        realSourceIndex=0;
    } else {
        




        realSource=pArgs->source;
        realSourceLimit=pArgs->sourceLimit;
        realFlush=pArgs->flush;
        realSourceIndex=sourceIndex;

        uprv_memcpy(replay, cnv->preFromU, -cnv->preFromULength*U_SIZEOF_UCHAR);
        pArgs->source=replay;
        pArgs->sourceLimit=replay-cnv->preFromULength;
        pArgs->flush=FALSE;
        sourceIndex=-1;

        cnv->preFromULength=0;
    }

    











    for(;;) {
        if(U_SUCCESS(*err)) {
            
            fromUnicode(pArgs, err);

            






            converterSawEndOfInput=
                (UBool)(U_SUCCESS(*err) &&
                        pArgs->flush && pArgs->source==pArgs->sourceLimit &&
                        cnv->fromUChar32==0);
        } else {
            
            converterSawEndOfInput=FALSE;
        }

        
        calledCallback=FALSE;

        
        errorInputLength=0;

        







        for(;;) {
            
            if(offsets!=NULL) {
                int32_t length=(int32_t)(pArgs->target-t);
                if(length>0) {
                    _updateOffsets(offsets, length, sourceIndex, errorInputLength);

                    






                    pArgs->offsets=offsets+=length;
                }

                if(sourceIndex>=0) {
                    sourceIndex+=(int32_t)(pArgs->source-s);
                }
            }

            if(cnv->preFromULength<0) {
                



                if(realSource==NULL) {
                    realSource=pArgs->source;
                    realSourceLimit=pArgs->sourceLimit;
                    realFlush=pArgs->flush;
                    realSourceIndex=sourceIndex;

                    uprv_memcpy(replay, cnv->preFromU, -cnv->preFromULength*U_SIZEOF_UCHAR);
                    pArgs->source=replay;
                    pArgs->sourceLimit=replay-cnv->preFromULength;
                    pArgs->flush=FALSE;
                    if((sourceIndex+=cnv->preFromULength)<0) {
                        sourceIndex=-1;
                    }

                    cnv->preFromULength=0;
                } else {
                    
                    U_ASSERT(realSource==NULL);
                    *err=U_INTERNAL_PROGRAM_ERROR;
                }
            }

            
            s=pArgs->source;
            t=pArgs->target;

            if(U_SUCCESS(*err)) {
                if(s<pArgs->sourceLimit) {
                    



                    break;
                } else if(realSource!=NULL) {
                    
                    pArgs->source=realSource;
                    pArgs->sourceLimit=realSourceLimit;
                    pArgs->flush=realFlush;
                    sourceIndex=realSourceIndex;

                    realSource=NULL;
                    break;
                } else if(pArgs->flush && cnv->fromUChar32!=0) {
                    




                    
                    *err=U_TRUNCATED_CHAR_FOUND;
                    calledCallback=FALSE; 
                } else {
                    
                    if(pArgs->flush) {
                        






                        if(!converterSawEndOfInput) {
                            break;
                        }

                        
                        _reset(cnv, UCNV_RESET_FROM_UNICODE, FALSE);
                    }

                    
                    return;
                }
            }

            
            {
                UErrorCode e;

                if( calledCallback ||
                    (e=*err)==U_BUFFER_OVERFLOW_ERROR ||
                    (e!=U_INVALID_CHAR_FOUND &&
                     e!=U_ILLEGAL_CHAR_FOUND &&
                     e!=U_TRUNCATED_CHAR_FOUND)
                ) {
                    











                    if(realSource!=NULL) {
                        int32_t length;

                        U_ASSERT(cnv->preFromULength==0);

                        length=(int32_t)(pArgs->sourceLimit-pArgs->source);
                        if(length>0) {
                            uprv_memcpy(cnv->preFromU, pArgs->source, length*U_SIZEOF_UCHAR);
                            cnv->preFromULength=(int8_t)-length;
                        }

                        pArgs->source=realSource;
                        pArgs->sourceLimit=realSourceLimit;
                        pArgs->flush=realFlush;
                    }

                    return;
                }
            }

            
            {
                UChar32 codePoint;

                
                codePoint=cnv->fromUChar32;
                errorInputLength=0;
                U16_APPEND_UNSAFE(cnv->invalidUCharBuffer, errorInputLength, codePoint);
                cnv->invalidUCharLength=(int8_t)errorInputLength;

                
                cnv->fromUChar32=0;

                
                cnv->fromUCharErrorBehaviour(cnv->fromUContext, pArgs,
                    cnv->invalidUCharBuffer, errorInputLength, codePoint,
                    *err==U_INVALID_CHAR_FOUND ? UCNV_UNASSIGNED : UCNV_ILLEGAL,
                    err);
            }

            






            calledCallback=TRUE;
        }
    }
}






static UBool
ucnv_outputOverflowFromUnicode(UConverter *cnv,
                               char **target, const char *targetLimit,
                               int32_t **pOffsets,
                               UErrorCode *err) {
    int32_t *offsets;
    char *overflow, *t;
    int32_t i, length;

    t=*target;
    if(pOffsets!=NULL) {
        offsets=*pOffsets;
    } else {
        offsets=NULL;
    }

    overflow=(char *)cnv->charErrorBuffer;
    length=cnv->charErrorBufferLength;
    i=0;
    while(i<length) {
        if(t==targetLimit) {
            
            int32_t j=0;

            do {
                overflow[j++]=overflow[i++];
            } while(i<length);

            cnv->charErrorBufferLength=(int8_t)j;
            *target=t;
            if(offsets!=NULL) {
                *pOffsets=offsets;
            }
            *err=U_BUFFER_OVERFLOW_ERROR;
            return TRUE;
        }

        
        *t++=overflow[i++];
        if(offsets!=NULL) {
            *offsets++=-1; 
        }
    }

    
    cnv->charErrorBufferLength=0;
    *target=t;
    if(offsets!=NULL) {
        *pOffsets=offsets;
    }
    return FALSE;
}

U_CAPI void U_EXPORT2
ucnv_fromUnicode(UConverter *cnv,
                 char **target, const char *targetLimit,
                 const UChar **source, const UChar *sourceLimit,
                 int32_t *offsets,
                 UBool flush,
                 UErrorCode *err) {
    UConverterFromUnicodeArgs args;
    const UChar *s;
    char *t;

    
    if(err==NULL || U_FAILURE(*err)) {
        return;
    }

    if(cnv==NULL || target==NULL || source==NULL) {
        *err=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    s=*source;
    t=*target;

    if ((const void *)U_MAX_PTR(sourceLimit) == (const void *)sourceLimit) {
        




        sourceLimit = (const UChar *)(((const char *)sourceLimit) - 1);
    }

    


















    if (sourceLimit<s || targetLimit<t ||
        ((size_t)(sourceLimit-s)>(size_t)0x3fffffff && sourceLimit>s) ||
        ((size_t)(targetLimit-t)>(size_t)0x7fffffff && targetLimit>t) ||
        (((const char *)sourceLimit-(const char *)s) & 1) != 0)
    {
        *err=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    
    if( cnv->charErrorBufferLength>0 &&
        ucnv_outputOverflowFromUnicode(cnv, target, targetLimit, &offsets, err)
    ) {
        
        return;
    }
    

    if(!flush && s==sourceLimit && cnv->preFromULength>=0) {
        
        return;
    }

    







    
    args.converter=cnv;
    args.flush=flush;
    args.offsets=offsets;
    args.source=s;
    args.sourceLimit=sourceLimit;
    args.target=*target;
    args.targetLimit=targetLimit;
    args.size=sizeof(args);

    _fromUnicodeWithCallback(&args, err);

    *source=args.source;
    *target=args.target;
}



static void
_toUnicodeWithCallback(UConverterToUnicodeArgs *pArgs, UErrorCode *err) {
    UConverterToUnicode toUnicode;
    UConverter *cnv;
    const char *s;
    UChar *t;
    int32_t *offsets;
    int32_t sourceIndex;
    int32_t errorInputLength;
    UBool converterSawEndOfInput, calledCallback;

    
    char replay[UCNV_EXT_MAX_BYTES];
    const char *realSource, *realSourceLimit;
    int32_t realSourceIndex;
    UBool realFlush;

    cnv=pArgs->converter;
    s=pArgs->source;
    t=pArgs->target;
    offsets=pArgs->offsets;

    
    sourceIndex=0;
    if(offsets==NULL) {
        toUnicode=cnv->sharedData->impl->toUnicode;
    } else {
        toUnicode=cnv->sharedData->impl->toUnicodeWithOffsets;
        if(toUnicode==NULL) {
            
            toUnicode=cnv->sharedData->impl->toUnicode;
            
            sourceIndex=-1;
        }
    }

    if(cnv->preToULength>=0) {
        
        realSource=NULL;

        
        realSourceLimit=NULL;
        realFlush=FALSE;
        realSourceIndex=0;
    } else {
        




        realSource=pArgs->source;
        realSourceLimit=pArgs->sourceLimit;
        realFlush=pArgs->flush;
        realSourceIndex=sourceIndex;

        uprv_memcpy(replay, cnv->preToU, -cnv->preToULength);
        pArgs->source=replay;
        pArgs->sourceLimit=replay-cnv->preToULength;
        pArgs->flush=FALSE;
        sourceIndex=-1;

        cnv->preToULength=0;
    }

    











    for(;;) {
        if(U_SUCCESS(*err)) {
            
            toUnicode(pArgs, err);

            






            converterSawEndOfInput=
                (UBool)(U_SUCCESS(*err) &&
                        pArgs->flush && pArgs->source==pArgs->sourceLimit &&
                        cnv->toULength==0);
        } else {
            
            converterSawEndOfInput=FALSE;
        }

        
        calledCallback=FALSE;

        
        errorInputLength=0;

        







        for(;;) {
            
            if(offsets!=NULL) {
                int32_t length=(int32_t)(pArgs->target-t);
                if(length>0) {
                    _updateOffsets(offsets, length, sourceIndex, errorInputLength);

                    






                    pArgs->offsets=offsets+=length;
                }

                if(sourceIndex>=0) {
                    sourceIndex+=(int32_t)(pArgs->source-s);
                }
            }

            if(cnv->preToULength<0) {
                



                if(realSource==NULL) {
                    realSource=pArgs->source;
                    realSourceLimit=pArgs->sourceLimit;
                    realFlush=pArgs->flush;
                    realSourceIndex=sourceIndex;

                    uprv_memcpy(replay, cnv->preToU, -cnv->preToULength);
                    pArgs->source=replay;
                    pArgs->sourceLimit=replay-cnv->preToULength;
                    pArgs->flush=FALSE;
                    if((sourceIndex+=cnv->preToULength)<0) {
                        sourceIndex=-1;
                    }

                    cnv->preToULength=0;
                } else {
                    
                    U_ASSERT(realSource==NULL);
                    *err=U_INTERNAL_PROGRAM_ERROR;
                }
            }

            
            s=pArgs->source;
            t=pArgs->target;

            if(U_SUCCESS(*err)) {
                if(s<pArgs->sourceLimit) {
                    



                    break;
                } else if(realSource!=NULL) {
                    
                    pArgs->source=realSource;
                    pArgs->sourceLimit=realSourceLimit;
                    pArgs->flush=realFlush;
                    sourceIndex=realSourceIndex;

                    realSource=NULL;
                    break;
                } else if(pArgs->flush && cnv->toULength>0) {
                    




                    
                    *err=U_TRUNCATED_CHAR_FOUND;
                    calledCallback=FALSE; 
                } else {
                    
                    if(pArgs->flush) {
                        






                        if(!converterSawEndOfInput) {
                            break;
                        }

                        
                        _reset(cnv, UCNV_RESET_TO_UNICODE, FALSE);
                    }

                    
                    return;
                }
            }

            
            {
                UErrorCode e;

                if( calledCallback ||
                    (e=*err)==U_BUFFER_OVERFLOW_ERROR ||
                    (e!=U_INVALID_CHAR_FOUND &&
                     e!=U_ILLEGAL_CHAR_FOUND &&
                     e!=U_TRUNCATED_CHAR_FOUND &&
                     e!=U_ILLEGAL_ESCAPE_SEQUENCE &&
                     e!=U_UNSUPPORTED_ESCAPE_SEQUENCE)
                ) {
                    











                    if(realSource!=NULL) {
                        int32_t length;

                        U_ASSERT(cnv->preToULength==0);

                        length=(int32_t)(pArgs->sourceLimit-pArgs->source);
                        if(length>0) {
                            uprv_memcpy(cnv->preToU, pArgs->source, length);
                            cnv->preToULength=(int8_t)-length;
                        }

                        pArgs->source=realSource;
                        pArgs->sourceLimit=realSourceLimit;
                        pArgs->flush=realFlush;
                    }

                    return;
                }
            }

            
            errorInputLength=cnv->invalidCharLength=cnv->toULength;
            if(errorInputLength>0) {
                uprv_memcpy(cnv->invalidCharBuffer, cnv->toUBytes, errorInputLength);
            }

            
            cnv->toULength=0;

            
            if(cnv->toUCallbackReason==UCNV_ILLEGAL && *err==U_INVALID_CHAR_FOUND) {
                cnv->toUCallbackReason = UCNV_UNASSIGNED;
            }
            cnv->fromCharErrorBehaviour(cnv->toUContext, pArgs,
                cnv->invalidCharBuffer, errorInputLength,
                cnv->toUCallbackReason,
                err);
            cnv->toUCallbackReason = UCNV_ILLEGAL; 

            






            calledCallback=TRUE;
        }
    }
}






static UBool
ucnv_outputOverflowToUnicode(UConverter *cnv,
                             UChar **target, const UChar *targetLimit,
                             int32_t **pOffsets,
                             UErrorCode *err) {
    int32_t *offsets;
    UChar *overflow, *t;
    int32_t i, length;

    t=*target;
    if(pOffsets!=NULL) {
        offsets=*pOffsets;
    } else {
        offsets=NULL;
    }

    overflow=cnv->UCharErrorBuffer;
    length=cnv->UCharErrorBufferLength;
    i=0;
    while(i<length) {
        if(t==targetLimit) {
            
            int32_t j=0;

            do {
                overflow[j++]=overflow[i++];
            } while(i<length);

            cnv->UCharErrorBufferLength=(int8_t)j;
            *target=t;
            if(offsets!=NULL) {
                *pOffsets=offsets;
            }
            *err=U_BUFFER_OVERFLOW_ERROR;
            return TRUE;
        }

        
        *t++=overflow[i++];
        if(offsets!=NULL) {
            *offsets++=-1; 
        }
    }

    
    cnv->UCharErrorBufferLength=0;
    *target=t;
    if(offsets!=NULL) {
        *pOffsets=offsets;
    }
    return FALSE;
}

U_CAPI void U_EXPORT2
ucnv_toUnicode(UConverter *cnv,
               UChar **target, const UChar *targetLimit,
               const char **source, const char *sourceLimit,
               int32_t *offsets,
               UBool flush,
               UErrorCode *err) {
    UConverterToUnicodeArgs args;
    const char *s;
    UChar *t;

    
    if(err==NULL || U_FAILURE(*err)) {
        return;
    }

    if(cnv==NULL || target==NULL || source==NULL) {
        *err=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    s=*source;
    t=*target;

    if ((const void *)U_MAX_PTR(targetLimit) == (const void *)targetLimit) {
        




        targetLimit = (const UChar *)(((const char *)targetLimit) - 1);
    }

    


















    if (sourceLimit<s || targetLimit<t ||
        ((size_t)(sourceLimit-s)>(size_t)0x7fffffff && sourceLimit>s) ||
        ((size_t)(targetLimit-t)>(size_t)0x3fffffff && targetLimit>t) ||
        (((const char *)targetLimit-(const char *)t) & 1) != 0
    ) {
        *err=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    
    if( cnv->UCharErrorBufferLength>0 &&
        ucnv_outputOverflowToUnicode(cnv, target, targetLimit, &offsets, err)
    ) {
        
        return;
    }
    

    if(!flush && s==sourceLimit && cnv->preToULength>=0) {
        
        return;
    }

    







    
    args.converter=cnv;
    args.flush=flush;
    args.offsets=offsets;
    args.source=s;
    args.sourceLimit=sourceLimit;
    args.target=*target;
    args.targetLimit=targetLimit;
    args.size=sizeof(args);

    _toUnicodeWithCallback(&args, err);

    *source=args.source;
    *target=args.target;
}



U_CAPI int32_t U_EXPORT2
ucnv_fromUChars(UConverter *cnv,
                char *dest, int32_t destCapacity,
                const UChar *src, int32_t srcLength,
                UErrorCode *pErrorCode) {
    const UChar *srcLimit;
    char *originalDest, *destLimit;
    int32_t destLength;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if( cnv==NULL ||
        destCapacity<0 || (destCapacity>0 && dest==NULL) ||
        srcLength<-1 || (srcLength!=0 && src==NULL)
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    ucnv_resetFromUnicode(cnv);
    originalDest=dest;
    if(srcLength==-1) {
        srcLength=u_strlen(src);
    }
    if(srcLength>0) {
        srcLimit=src+srcLength;
        destLimit=dest+destCapacity;

        
        if(destLimit<dest || (destLimit==NULL && dest!=NULL)) {
            destLimit=(char *)U_MAX_PTR(dest);
        }

        
        ucnv_fromUnicode(cnv, &dest, destLimit, &src, srcLimit, 0, TRUE, pErrorCode);
        destLength=(int32_t)(dest-originalDest);

        
        if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
            char buffer[1024];

            destLimit=buffer+sizeof(buffer);
            do {
                dest=buffer;
                *pErrorCode=U_ZERO_ERROR;
                ucnv_fromUnicode(cnv, &dest, destLimit, &src, srcLimit, 0, TRUE, pErrorCode);
                destLength+=(int32_t)(dest-buffer);
            } while(*pErrorCode==U_BUFFER_OVERFLOW_ERROR);
        }
    } else {
        destLength=0;
    }

    return u_terminateChars(originalDest, destCapacity, destLength, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
ucnv_toUChars(UConverter *cnv,
              UChar *dest, int32_t destCapacity,
              const char *src, int32_t srcLength,
              UErrorCode *pErrorCode) {
    const char *srcLimit;
    UChar *originalDest, *destLimit;
    int32_t destLength;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if( cnv==NULL ||
        destCapacity<0 || (destCapacity>0 && dest==NULL) ||
        srcLength<-1 || (srcLength!=0 && src==NULL))
    {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    ucnv_resetToUnicode(cnv);
    originalDest=dest;
    if(srcLength==-1) {
        srcLength=(int32_t)uprv_strlen(src);
    }
    if(srcLength>0) {
        srcLimit=src+srcLength;
        destLimit=dest+destCapacity;

        
        if(destLimit<dest || (destLimit==NULL && dest!=NULL)) {
            destLimit=(UChar *)U_MAX_PTR(dest);
        }

        
        ucnv_toUnicode(cnv, &dest, destLimit, &src, srcLimit, 0, TRUE, pErrorCode);
        destLength=(int32_t)(dest-originalDest);

        
        if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR)
        {
            UChar buffer[1024];

            destLimit=buffer+sizeof(buffer)/U_SIZEOF_UCHAR;
            do {
                dest=buffer;
                *pErrorCode=U_ZERO_ERROR;
                ucnv_toUnicode(cnv, &dest, destLimit, &src, srcLimit, 0, TRUE, pErrorCode);
                destLength+=(int32_t)(dest-buffer);
            }
            while(*pErrorCode==U_BUFFER_OVERFLOW_ERROR);
        }
    } else {
        destLength=0;
    }

    return u_terminateUChars(originalDest, destCapacity, destLength, pErrorCode);
}



U_CAPI UChar32 U_EXPORT2
ucnv_getNextUChar(UConverter *cnv,
                  const char **source, const char *sourceLimit,
                  UErrorCode *err) {
    UConverterToUnicodeArgs args;
    UChar buffer[U16_MAX_LENGTH];
    const char *s;
    UChar32 c;
    int32_t i, length;

    
    if(err==NULL || U_FAILURE(*err)) {
        return 0xffff;
    }

    if(cnv==NULL || source==NULL) {
        *err=U_ILLEGAL_ARGUMENT_ERROR;
        return 0xffff;
    }

    s=*source;
    if(sourceLimit<s) {
        *err=U_ILLEGAL_ARGUMENT_ERROR;
        return 0xffff;
    }

    











    if(((size_t)(sourceLimit-s)>(size_t)0x7fffffff && sourceLimit>s)) {
        *err=U_ILLEGAL_ARGUMENT_ERROR;
        return 0xffff;
    }

    c=U_SENTINEL;

    
    if(cnv->UCharErrorBufferLength>0) {
        UChar *overflow;

        overflow=cnv->UCharErrorBuffer;
        i=0;
        length=cnv->UCharErrorBufferLength;
        U16_NEXT(overflow, i, length, c);

        
        if((cnv->UCharErrorBufferLength=(int8_t)(length-i))>0) {
            uprv_memmove(cnv->UCharErrorBuffer, cnv->UCharErrorBuffer+i,
                         cnv->UCharErrorBufferLength*U_SIZEOF_UCHAR);
        }

        if(!U16_IS_LEAD(c) || i<length) {
            return c;
        }
        




    }

    






    
    args.converter=cnv;
    args.flush=TRUE;
    args.offsets=NULL;
    args.source=s;
    args.sourceLimit=sourceLimit;
    args.target=buffer;
    args.targetLimit=buffer+1;
    args.size=sizeof(args);

    if(c<0) {
        







        if(cnv->toULength==0 && cnv->sharedData->impl->getNextUChar!=NULL) {
            c=cnv->sharedData->impl->getNextUChar(&args, err);
            *source=s=args.source;
            if(*err==U_INDEX_OUTOFBOUNDS_ERROR) {
                
                _reset(cnv, UCNV_RESET_TO_UNICODE, FALSE);
                return 0xffff; 
            } else if(U_SUCCESS(*err) && c>=0) {
                return c;
            




            }
        }

        
        _toUnicodeWithCallback(&args, err);

        if(*err==U_BUFFER_OVERFLOW_ERROR) {
            *err=U_ZERO_ERROR;
        }

        i=0;
        length=(int32_t)(args.target-buffer);
    } else {
        
        buffer[0]=(UChar)c;
        args.target=buffer+1;
        i=0;
        length=1;
    }

    

    if(U_FAILURE(*err)) {
        c=0xffff; 
    } else if(length==0) {
        
        *err=U_INDEX_OUTOFBOUNDS_ERROR;
        
        c=0xffff; 
    } else {
        c=buffer[0];
        i=1;
        if(!U16_IS_LEAD(c)) {
            
        } else {
            
            UChar c2;

            if(cnv->UCharErrorBufferLength>0) {
                
                if(U16_IS_TRAIL(c2=cnv->UCharErrorBuffer[0])) {
                    
                    c=U16_GET_SUPPLEMENTARY(c, c2);

                    
                    if((--cnv->UCharErrorBufferLength)>0) {
                        uprv_memmove(cnv->UCharErrorBuffer, cnv->UCharErrorBuffer+1,
                                     cnv->UCharErrorBufferLength*U_SIZEOF_UCHAR);
                    }
                } else {
                    
                }
            } else if(args.source<sourceLimit) {
                
                args.targetLimit=buffer+2;
                _toUnicodeWithCallback(&args, err);
                if(*err==U_BUFFER_OVERFLOW_ERROR) {
                    *err=U_ZERO_ERROR;
                }

                length=(int32_t)(args.target-buffer);
                if(U_SUCCESS(*err) && length==2 && U16_IS_TRAIL(c2=buffer[1])) {
                    
                    c=U16_GET_SUPPLEMENTARY(c, c2);
                    i=2;
                }
            }
        }
    }

    



    if(i<length) {
        
        int32_t delta=length-i;
        if((length=cnv->UCharErrorBufferLength)>0) {
            uprv_memmove(cnv->UCharErrorBuffer+delta, cnv->UCharErrorBuffer,
                         length*U_SIZEOF_UCHAR);
        }
        cnv->UCharErrorBufferLength=(int8_t)(length+delta);

        cnv->UCharErrorBuffer[0]=buffer[i++];
        if(delta>1) {
            cnv->UCharErrorBuffer[1]=buffer[i];
        }
    }

    *source=args.source;
    return c;
}



U_CAPI void U_EXPORT2
ucnv_convertEx(UConverter *targetCnv, UConverter *sourceCnv,
               char **target, const char *targetLimit,
               const char **source, const char *sourceLimit,
               UChar *pivotStart, UChar **pivotSource,
               UChar **pivotTarget, const UChar *pivotLimit,
               UBool reset, UBool flush,
               UErrorCode *pErrorCode) {
    UChar pivotBuffer[CHUNK_SIZE];
    const UChar *myPivotSource;
    UChar *myPivotTarget;
    const char *s;
    char *t;

    UConverterToUnicodeArgs toUArgs;
    UConverterFromUnicodeArgs fromUArgs;
    UConverterConvert convert;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }

    if( targetCnv==NULL || sourceCnv==NULL ||
        source==NULL || *source==NULL ||
        target==NULL || *target==NULL || targetLimit==NULL
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    s=*source;
    t=*target;
    if((sourceLimit!=NULL && sourceLimit<s) || targetLimit<t) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    



    if(
        (sourceLimit!=NULL && ((size_t)(sourceLimit-s)>(size_t)0x7fffffff && sourceLimit>s)) ||
        ((size_t)(targetLimit-t)>(size_t)0x7fffffff && targetLimit>t)
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    if(pivotStart==NULL) {
        if(!flush) {
            
            *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }

        
        myPivotSource=myPivotTarget=pivotStart=pivotBuffer;
        pivotSource=(UChar **)&myPivotSource;
        pivotTarget=&myPivotTarget;
        pivotLimit=pivotBuffer+CHUNK_SIZE;
    } else if(  pivotStart>=pivotLimit ||
                pivotSource==NULL || *pivotSource==NULL ||
                pivotTarget==NULL || *pivotTarget==NULL ||
                pivotLimit==NULL
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if(sourceLimit==NULL) {
        
        sourceLimit=uprv_strchr(*source, 0);
    }

    if(reset) {
        ucnv_resetToUnicode(sourceCnv);
        ucnv_resetFromUnicode(targetCnv);
        *pivotSource=*pivotTarget=pivotStart;
    } else if(targetCnv->charErrorBufferLength>0) {
        
        if(ucnv_outputOverflowFromUnicode(targetCnv, target, targetLimit, NULL, pErrorCode)) {
            
            return;
        }
        

        if( !flush &&
            targetCnv->preFromULength>=0 && *pivotSource==*pivotTarget &&
            sourceCnv->UCharErrorBufferLength==0 && sourceCnv->preToULength>=0 && s==sourceLimit
        ) {
            
            return;
        }
    }

    
    if( sourceCnv->sharedData->staticData->conversionType==UCNV_UTF8 &&
        targetCnv->sharedData->impl->fromUTF8!=NULL
    ) {
        convert=targetCnv->sharedData->impl->fromUTF8;
    } else if( targetCnv->sharedData->staticData->conversionType==UCNV_UTF8 &&
               sourceCnv->sharedData->impl->toUTF8!=NULL
    ) {
        convert=sourceCnv->sharedData->impl->toUTF8;
    } else {
        convert=NULL;
    }

    


















    if(convert!=NULL && (pivotLimit-pivotStart)>32) {
        pivotLimit=pivotStart+32;
    }

    
    fromUArgs.converter=targetCnv;
    fromUArgs.flush=FALSE;
    fromUArgs.offsets=NULL;
    fromUArgs.target=*target;
    fromUArgs.targetLimit=targetLimit;
    fromUArgs.size=sizeof(fromUArgs);

    toUArgs.converter=sourceCnv;
    toUArgs.flush=flush;
    toUArgs.offsets=NULL;
    toUArgs.source=s;
    toUArgs.sourceLimit=sourceLimit;
    toUArgs.targetLimit=pivotLimit;
    toUArgs.size=sizeof(toUArgs);

    






    s=t=NULL;

    










    for(;;) {
        







        if( *pivotSource<*pivotTarget ||
            U_FAILURE(*pErrorCode) ||
            targetCnv->preFromULength<0 ||
            fromUArgs.flush
        ) {
            fromUArgs.source=*pivotSource;
            fromUArgs.sourceLimit=*pivotTarget;
            _fromUnicodeWithCallback(&fromUArgs, pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                
                *pivotSource=(UChar *)fromUArgs.source;
                break;
            }

            



        }

        
        *pivotSource=*pivotTarget=pivotStart;

        





        
        if(sourceCnv->UCharErrorBufferLength>0) {
            if(ucnv_outputOverflowToUnicode(sourceCnv, pivotTarget, pivotLimit, NULL, pErrorCode)) {
                
                *pErrorCode=U_ZERO_ERROR;
            }
            continue;
        }

        






        if( toUArgs.source==sourceLimit &&
            sourceCnv->preToULength>=0 && sourceCnv->toULength==0 &&
            (!flush || fromUArgs.flush)
        ) {
            
            break;
        }

        




        if(convert!=NULL && targetCnv->preFromUFirstCP<0 && sourceCnv->preToULength==0) {
            if(*pErrorCode==U_USING_DEFAULT_WARNING) {
                
                *pErrorCode=U_ZERO_ERROR;
            }
            convert(&fromUArgs, &toUArgs, pErrorCode);
            if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                break;
            } else if(U_FAILURE(*pErrorCode)) {
                if(sourceCnv->toULength>0) {
                    








                } else {
                    



                    *pivotSource=*pivotTarget=pivotStart+1;
                    



                    continue;
                }
            } else if(*pErrorCode==U_USING_DEFAULT_WARNING) {
                



                *pErrorCode=U_ZERO_ERROR;
            




            } else if(flush && sourceCnv->toULength>0) { 
                




                
                *pErrorCode=U_TRUNCATED_CHAR_FOUND;
            } else {
                
                if(flush) {
                    
                    _reset(sourceCnv, UCNV_RESET_TO_UNICODE, FALSE);
                    _reset(targetCnv, UCNV_RESET_FROM_UNICODE, FALSE);
                }

                
                break;
            }
        }
        
        








        toUArgs.target=pivotStart; 
        
        _toUnicodeWithCallback(&toUArgs, pErrorCode);
        *pivotTarget=toUArgs.target;
        if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
            
            *pErrorCode=U_ZERO_ERROR;
        } else if(U_FAILURE(*pErrorCode) || (!flush && *pivotTarget==pivotStart)) {
            
            break;
        }
        







        if( flush && toUArgs.source==sourceLimit &&
            sourceCnv->preToULength>=0 &&
            sourceCnv->UCharErrorBufferLength==0
        ) {
            fromUArgs.flush=TRUE;
        }
    }

    






    *source=toUArgs.source;
    *target=fromUArgs.target;

    
    if(flush && U_SUCCESS(*pErrorCode)) {
        if(*target!=targetLimit) {
            **target=0;
            if(*pErrorCode==U_STRING_NOT_TERMINATED_WARNING) {
                *pErrorCode=U_ZERO_ERROR;
            }
        } else {
            *pErrorCode=U_STRING_NOT_TERMINATED_WARNING;
        }
    }
}


static int32_t
ucnv_internalConvert(UConverter *outConverter, UConverter *inConverter,
                     char *target, int32_t targetCapacity,
                     const char *source, int32_t sourceLength,
                     UErrorCode *pErrorCode) {
    UChar pivotBuffer[CHUNK_SIZE];
    UChar *pivot, *pivot2;

    char *myTarget;
    const char *sourceLimit;
    const char *targetLimit;
    int32_t targetLength=0;

    
    if(sourceLength<0) {
        sourceLimit=uprv_strchr(source, 0);
    } else {
        sourceLimit=source+sourceLength;
    }

    
    if(source==sourceLimit) {
        return u_terminateChars(target, targetCapacity, 0, pErrorCode);
    }

    pivot=pivot2=pivotBuffer;
    myTarget=target;
    targetLength=0;

    if(targetCapacity>0) {
        
        targetLimit=target+targetCapacity;
        ucnv_convertEx(outConverter, inConverter,
                       &myTarget, targetLimit,
                       &source, sourceLimit,
                       pivotBuffer, &pivot, &pivot2, pivotBuffer+CHUNK_SIZE,
                       FALSE,
                       TRUE,
                       pErrorCode);
        targetLength=(int32_t)(myTarget-target);
    }

    




    if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR || targetCapacity==0)
    {
        char targetBuffer[CHUNK_SIZE];

        targetLimit=targetBuffer+CHUNK_SIZE;
        do {
            *pErrorCode=U_ZERO_ERROR;
            myTarget=targetBuffer;
            ucnv_convertEx(outConverter, inConverter,
                           &myTarget, targetLimit,
                           &source, sourceLimit,
                           pivotBuffer, &pivot, &pivot2, pivotBuffer+CHUNK_SIZE,
                           FALSE,
                           TRUE,
                           pErrorCode);
            targetLength+=(int32_t)(myTarget-targetBuffer);
        } while(*pErrorCode==U_BUFFER_OVERFLOW_ERROR);

        
        return u_terminateChars(target, targetCapacity, targetLength, pErrorCode);
    }

    
    return targetLength;
}

U_CAPI int32_t U_EXPORT2
ucnv_convert(const char *toConverterName, const char *fromConverterName,
             char *target, int32_t targetCapacity,
             const char *source, int32_t sourceLength,
             UErrorCode *pErrorCode) {
    UConverter in, out; 
    UConverter *inConverter, *outConverter;
    int32_t targetLength;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if( source==NULL || sourceLength<-1 ||
        targetCapacity<0 || (targetCapacity>0 && target==NULL)
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if(sourceLength==0 || (sourceLength<0 && *source==0)) {
        return u_terminateChars(target, targetCapacity, 0, pErrorCode);
    }

    
    inConverter=ucnv_createConverter(&in, fromConverterName, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    outConverter=ucnv_createConverter(&out, toConverterName, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        ucnv_close(inConverter);
        return 0;
    }

    targetLength=ucnv_internalConvert(outConverter, inConverter,
                                      target, targetCapacity,
                                      source, sourceLength,
                                      pErrorCode);

    ucnv_close(inConverter);
    ucnv_close(outConverter);

    return targetLength;
}


static int32_t
ucnv_convertAlgorithmic(UBool convertToAlgorithmic,
                        UConverterType algorithmicType,
                        UConverter *cnv,
                        char *target, int32_t targetCapacity,
                        const char *source, int32_t sourceLength,
                        UErrorCode *pErrorCode) {
    UConverter algoConverterStatic; 
    UConverter *algoConverter, *to, *from;
    int32_t targetLength;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if( cnv==NULL || source==NULL || sourceLength<-1 ||
        targetCapacity<0 || (targetCapacity>0 && target==NULL)
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if(sourceLength==0 || (sourceLength<0 && *source==0)) {
        return u_terminateChars(target, targetCapacity, 0, pErrorCode);
    }

    
    algoConverter=ucnv_createAlgorithmicConverter(&algoConverterStatic, algorithmicType,
                                                  "", 0, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    if(convertToAlgorithmic) {
        
        ucnv_resetToUnicode(cnv);
        to=algoConverter;
        from=cnv;
    } else {
        
        ucnv_resetFromUnicode(cnv);
        from=algoConverter;
        to=cnv;
    }

    targetLength=ucnv_internalConvert(to, from,
                                      target, targetCapacity,
                                      source, sourceLength,
                                      pErrorCode);

    ucnv_close(algoConverter);

    return targetLength;
}

U_CAPI int32_t U_EXPORT2
ucnv_toAlgorithmic(UConverterType algorithmicType,
                   UConverter *cnv,
                   char *target, int32_t targetCapacity,
                   const char *source, int32_t sourceLength,
                   UErrorCode *pErrorCode) {
    return ucnv_convertAlgorithmic(TRUE, algorithmicType, cnv,
                                   target, targetCapacity,
                                   source, sourceLength,
                                   pErrorCode);
}

U_CAPI int32_t U_EXPORT2
ucnv_fromAlgorithmic(UConverter *cnv,
                     UConverterType algorithmicType,
                     char *target, int32_t targetCapacity,
                     const char *source, int32_t sourceLength,
                     UErrorCode *pErrorCode) {
    return ucnv_convertAlgorithmic(FALSE, algorithmicType, cnv,
                                   target, targetCapacity,
                                   source, sourceLength,
                                   pErrorCode);
}

U_CAPI UConverterType  U_EXPORT2
ucnv_getType(const UConverter* converter)
{
    int8_t type = converter->sharedData->staticData->conversionType;
#if !UCONFIG_NO_LEGACY_CONVERSION
    if(type == UCNV_MBCS) {
        return ucnv_MBCSGetType(converter);
    }
#endif
    return (UConverterType)type;
}

U_CAPI void  U_EXPORT2
ucnv_getStarters(const UConverter* converter, 
                 UBool starters[256],
                 UErrorCode* err)
{
    if (err == NULL || U_FAILURE(*err)) {
        return;
    }

    if(converter->sharedData->impl->getStarters != NULL) {
        converter->sharedData->impl->getStarters(converter, starters, err);
    } else {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
    }
}

static const UAmbiguousConverter *ucnv_getAmbiguous(const UConverter *cnv)
{
    UErrorCode errorCode;
    const char *name;
    int32_t i;

    if(cnv==NULL) {
        return NULL;
    }

    errorCode=U_ZERO_ERROR;
    name=ucnv_getName(cnv, &errorCode);
    if(U_FAILURE(errorCode)) {
        return NULL;
    }

    for(i=0; i<(int32_t)(sizeof(ambiguousConverters)/sizeof(UAmbiguousConverter)); ++i)
    {
        if(0==uprv_strcmp(name, ambiguousConverters[i].name))
        {
            return ambiguousConverters+i;
        }
    }

    return NULL;
}

U_CAPI void  U_EXPORT2
ucnv_fixFileSeparator(const UConverter *cnv, 
                      UChar* source, 
                      int32_t sourceLength) {
    const UAmbiguousConverter *a;
    int32_t i;
    UChar variant5c;

    if(cnv==NULL || source==NULL || sourceLength<=0 || (a=ucnv_getAmbiguous(cnv))==NULL)
    {
        return;
    }

    variant5c=a->variant5c;
    for(i=0; i<sourceLength; ++i) {
        if(source[i]==variant5c) {
            source[i]=0x5c;
        }
    }
}

U_CAPI UBool  U_EXPORT2
ucnv_isAmbiguous(const UConverter *cnv) {
    return (UBool)(ucnv_getAmbiguous(cnv)!=NULL);
}

U_CAPI void  U_EXPORT2
ucnv_setFallback(UConverter *cnv, UBool usesFallback)
{
    cnv->useFallback = usesFallback;
}

U_CAPI UBool  U_EXPORT2
ucnv_usesFallback(const UConverter *cnv)
{
    return cnv->useFallback;
}

U_CAPI void  U_EXPORT2
ucnv_getInvalidChars (const UConverter * converter,
                      char *errBytes,
                      int8_t * len,
                      UErrorCode * err)
{
    if (err == NULL || U_FAILURE(*err))
    {
        return;
    }
    if (len == NULL || errBytes == NULL || converter == NULL)
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (*len < converter->invalidCharLength)
    {
        *err = U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    }
    if ((*len = converter->invalidCharLength) > 0)
    {
        uprv_memcpy (errBytes, converter->invalidCharBuffer, *len);
    }
}

U_CAPI void  U_EXPORT2
ucnv_getInvalidUChars (const UConverter * converter,
                       UChar *errChars,
                       int8_t * len,
                       UErrorCode * err)
{
    if (err == NULL || U_FAILURE(*err))
    {
        return;
    }
    if (len == NULL || errChars == NULL || converter == NULL)
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (*len < converter->invalidUCharLength)
    {
        *err = U_INDEX_OUTOFBOUNDS_ERROR;
        return;
    }
    if ((*len = converter->invalidUCharLength) > 0)
    {
        uprv_memcpy (errChars, converter->invalidUCharBuffer, sizeof(UChar) * (*len));
    }
}

#define SIG_MAX_LEN 5

U_CAPI const char* U_EXPORT2
ucnv_detectUnicodeSignature( const char* source,
                             int32_t sourceLength,
                             int32_t* signatureLength,
                             UErrorCode* pErrorCode) {
    int32_t dummy;

    


    char start[SIG_MAX_LEN]={ '\xa5', '\xa5', '\xa5', '\xa5', '\xa5' };
    int i = 0;

    if((pErrorCode==NULL) || U_FAILURE(*pErrorCode)){
        return NULL;
    }
    
    if(source == NULL || sourceLength < -1){
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    if(signatureLength == NULL) {
        signatureLength = &dummy;
    }

    if(sourceLength==-1){
        sourceLength=(int32_t)uprv_strlen(source);
    }

    
    while(i<sourceLength&& i<SIG_MAX_LEN){
        start[i]=source[i];
        i++;
    }

    if(start[0] == '\xFE' && start[1] == '\xFF') {
        *signatureLength=2;
        return  "UTF-16BE";
    } else if(start[0] == '\xFF' && start[1] == '\xFE') {
        if(start[2] == '\x00' && start[3] =='\x00') {
            *signatureLength=4;
            return "UTF-32LE";
        } else {
            *signatureLength=2;
            return  "UTF-16LE";
        }
    } else if(start[0] == '\xEF' && start[1] == '\xBB' && start[2] == '\xBF') {
        *signatureLength=3;
        return  "UTF-8";
    } else if(start[0] == '\x00' && start[1] == '\x00' && 
              start[2] == '\xFE' && start[3]=='\xFF') {
        *signatureLength=4;
        return  "UTF-32BE";
    } else if(start[0] == '\x0E' && start[1] == '\xFE' && start[2] == '\xFF') {
        *signatureLength=3;
        return "SCSU";
    } else if(start[0] == '\xFB' && start[1] == '\xEE' && start[2] == '\x28') {
        *signatureLength=3;
        return "BOCU-1";
    } else if(start[0] == '\x2B' && start[1] == '\x2F' && start[2] == '\x76') {
        







        if(start[3] == '\x38' && start[4] == '\x2D') {
            
            *signatureLength=5;
            return "UTF-7";
        } else if(start[3] == '\x38' || start[3] == '\x39' || start[3] == '\x2B' || start[3] == '\x2F') {
            
            *signatureLength=4;
            return "UTF-7";
        }
    }else if(start[0]=='\xDD' && start[1]== '\x73'&& start[2]=='\x66' && start[3]=='\x73'){
        *signatureLength=4;
        return "UTF-EBCDIC";
    }


    
    *signatureLength=0;
    return NULL;
}

U_CAPI int32_t U_EXPORT2
ucnv_fromUCountPending(const UConverter* cnv, UErrorCode* status)
{
    if(status == NULL || U_FAILURE(*status)){
        return -1;
    }
    if(cnv == NULL){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    }

    if(cnv->preFromUFirstCP >= 0){
        return U16_LENGTH(cnv->preFromUFirstCP)+cnv->preFromULength ;
    }else if(cnv->preFromULength < 0){
        return -cnv->preFromULength ;
    }else if(cnv->fromUChar32 > 0){
        return 1;
    }
    return 0; 

}

U_CAPI int32_t U_EXPORT2
ucnv_toUCountPending(const UConverter* cnv, UErrorCode* status){

    if(status == NULL || U_FAILURE(*status)){
        return -1;
    }
    if(cnv == NULL){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    }

    if(cnv->preToULength > 0){
        return cnv->preToULength ;
    }else if(cnv->preToULength < 0){
        return -cnv->preToULength;
    }else if(cnv->toULength > 0){
        return cnv->toULength;
    }
    return 0;
}

U_CAPI UBool U_EXPORT2
ucnv_isFixedWidth(UConverter *cnv, UErrorCode *status){
    if (U_FAILURE(*status)) {
        return FALSE;
    }

    if (cnv == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }

    switch (ucnv_getType(cnv)) {
        case UCNV_SBCS:
        case UCNV_DBCS:
        case UCNV_UTF32_BigEndian:
        case UCNV_UTF32_LittleEndian:
        case UCNV_UTF32:
        case UCNV_US_ASCII:
            return TRUE;
        default:
            return FALSE;
    }
}
#endif









