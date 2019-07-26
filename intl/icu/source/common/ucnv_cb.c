



















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv_cb.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "cmemory.h"





U_CAPI void  U_EXPORT2
ucnv_cbFromUWriteBytes (UConverterFromUnicodeArgs *args,
                       const char* source,
                       int32_t length,
                       int32_t offsetIndex,
                       UErrorCode * err)
{
    if(U_FAILURE(*err)) {
        return;
    }

    ucnv_fromUWriteBytes(
        args->converter,
        source, length,
        &args->target, args->targetLimit,
        &args->offsets, offsetIndex,
        err);
}

U_CAPI void  U_EXPORT2
ucnv_cbFromUWriteUChars(UConverterFromUnicodeArgs *args,
                             const UChar** source,
                             const UChar*  sourceLimit,
                             int32_t offsetIndex,
                             UErrorCode * err)
{
    











    char *oldTarget;

    if(U_FAILURE(*err))
    {
        return;
    }

    oldTarget = args->target;

    ucnv_fromUnicode(args->converter,
        &args->target,
        args->targetLimit,
        source,
        sourceLimit,
        NULL, 
        FALSE, 
        err);

    if(args->offsets)
    {
        while (args->target != oldTarget)  
        {
            *(args->offsets)++ = offsetIndex;
            oldTarget++;
        }
    }

    



    if(*err == U_BUFFER_OVERFLOW_ERROR)
    

    {
        

        char *newTarget;
        const char *newTargetLimit;
        UErrorCode err2 = U_ZERO_ERROR;

        int8_t errBuffLen;

        errBuffLen  = args->converter->charErrorBufferLength;

        
        newTarget = (char *)(args->converter->charErrorBuffer + errBuffLen);

        newTargetLimit = (char *)(args->converter->charErrorBuffer +
            sizeof(args->converter->charErrorBuffer));

        if(newTarget >= newTargetLimit)
        {
            *err = U_INTERNAL_PROGRAM_ERROR;
            return;
        }

        




        args->converter->charErrorBufferLength = 0;

        ucnv_fromUnicode(args->converter,
                         &newTarget,
                         newTargetLimit,
                         source,
                         sourceLimit,
                         NULL,
                         FALSE,
                         &err2);

        


        args->converter->charErrorBufferLength = (int8_t)(
            newTarget - (char*)args->converter->charErrorBuffer);

        if((newTarget >= newTargetLimit) || (err2 == U_BUFFER_OVERFLOW_ERROR))
        {
            



            *err = U_INTERNAL_PROGRAM_ERROR;
            return;
        }
        
            




            



        
    }
}

U_CAPI void  U_EXPORT2
ucnv_cbFromUWriteSub (UConverterFromUnicodeArgs *args,
                           int32_t offsetIndex,
                           UErrorCode * err)
{
    UConverter *converter;
    int32_t length;

    if(U_FAILURE(*err)) {
        return;
    }
    converter = args->converter;
    length = converter->subCharLen;

    if(length == 0) {
        return;
    }

    if(length < 0) {
        







        const UChar *source = (const UChar *)converter->subChars;
        ucnv_cbFromUWriteUChars(args, &source, source - length, offsetIndex, err);
        return;
    }

    if(converter->sharedData->impl->writeSub!=NULL) {
        converter->sharedData->impl->writeSub(args, offsetIndex, err);
    }
    else if(converter->subChar1!=0 && (uint16_t)converter->invalidUCharBuffer[0]<=(uint16_t)0xffu) {
        



        ucnv_cbFromUWriteBytes(args,
                               (const char *)&converter->subChar1, 1,
                               offsetIndex, err);
    }
    else {
        ucnv_cbFromUWriteBytes(args,
                               (const char *)converter->subChars, length,
                               offsetIndex, err);
    }
}

U_CAPI void  U_EXPORT2
ucnv_cbToUWriteUChars (UConverterToUnicodeArgs *args,
                            const UChar* source,
                            int32_t length,
                            int32_t offsetIndex,
                            UErrorCode * err)
{
    if(U_FAILURE(*err)) {
        return;
    }

    ucnv_toUWriteUChars(
        args->converter,
        source, length,
        &args->target, args->targetLimit,
        &args->offsets, offsetIndex,
        err);
}

U_CAPI void  U_EXPORT2
ucnv_cbToUWriteSub (UConverterToUnicodeArgs *args,
                         int32_t offsetIndex,
                       UErrorCode * err)
{
    static const UChar kSubstituteChar1 = 0x1A, kSubstituteChar = 0xFFFD;

    
    if(args->converter->invalidCharLength == 1 && args->converter->subChar1 != 0) {
        ucnv_cbToUWriteUChars(args, &kSubstituteChar1, 1, offsetIndex, err);
    } else {
        ucnv_cbToUWriteUChars(args, &kSubstituteChar, 1, offsetIndex, err);
    }
}

#endif
