















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "unicode/utf.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "cmemory.h"

#define MAXIMUM_UCS2            0x0000FFFF
#define MAXIMUM_UTF             0x0010FFFF
#define HALF_SHIFT              10
#define HALF_BASE               0x0010000
#define HALF_MASK               0x3FF
#define SURROGATE_HIGH_START    0xD800
#define SURROGATE_LOW_START     0xDC00


#define SURROGATE_LOW_BASE      9216

enum {
    UCNV_NEED_TO_WRITE_BOM=1
};



static void
T_UConverter_toUnicode_UTF32_BE(UConverterToUnicodeArgs * args,
                                UErrorCode * err)
{
    const unsigned char *mySource = (unsigned char *) args->source;
    UChar *myTarget = args->target;
    const unsigned char *sourceLimit = (unsigned char *) args->sourceLimit;
    const UChar *targetLimit = args->targetLimit;
    unsigned char *toUBytes = args->converter->toUBytes;
    uint32_t ch, i;

    
    if (args->converter->toUnicodeStatus && myTarget < targetLimit) {
        i = args->converter->toULength;       
        args->converter->toULength = 0;

        ch = args->converter->toUnicodeStatus - 1;
        args->converter->toUnicodeStatus = 0;
        goto morebytes;
    }

    while (mySource < sourceLimit && myTarget < targetLimit) {
        i = 0;
        ch = 0;
morebytes:
        while (i < sizeof(uint32_t)) {
            if (mySource < sourceLimit) {
                ch = (ch << 8) | (uint8_t)(*mySource);
                toUBytes[i++] = (char) *(mySource++);
            }
            else {
                
                
                args->converter->toUnicodeStatus = ch + 1;
                args->converter->toULength = (int8_t) i;
                goto donefornow;
            }
        }

        if (ch <= MAXIMUM_UTF && !U_IS_SURROGATE(ch)) {
            
            if (ch <= MAXIMUM_UCS2) 
            {
                
                *(myTarget++) = (UChar) ch;
            }
            else {
                
                *(myTarget++) = U16_LEAD(ch);
                ch = U16_TRAIL(ch);
                if (myTarget < targetLimit) {
                    *(myTarget++) = (UChar)ch;
                }
                else {
                    
                    args->converter->UCharErrorBuffer[0] = (UChar) ch;
                    args->converter->UCharErrorBufferLength = 1;
                    *err = U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            }
        }
        else {
            args->converter->toULength = (int8_t)i;
            *err = U_ILLEGAL_CHAR_FOUND;
            break;
        }
    }

donefornow:
    if (mySource < sourceLimit && myTarget >= targetLimit && U_SUCCESS(*err)) {
        
        *err = U_BUFFER_OVERFLOW_ERROR;
    }

    args->target = myTarget;
    args->source = (const char *) mySource;
}

static void
T_UConverter_toUnicode_UTF32_BE_OFFSET_LOGIC(UConverterToUnicodeArgs * args,
                                             UErrorCode * err)
{
    const unsigned char *mySource = (unsigned char *) args->source;
    UChar *myTarget = args->target;
    int32_t *myOffsets = args->offsets;
    const unsigned char *sourceLimit = (unsigned char *) args->sourceLimit;
    const UChar *targetLimit = args->targetLimit;
    unsigned char *toUBytes = args->converter->toUBytes;
    uint32_t ch, i;
    int32_t offsetNum = 0;

    
    if (args->converter->toUnicodeStatus && myTarget < targetLimit) {
        i = args->converter->toULength;       
        args->converter->toULength = 0;

        ch = args->converter->toUnicodeStatus - 1;
        args->converter->toUnicodeStatus = 0;
        goto morebytes;
    }

    while (mySource < sourceLimit && myTarget < targetLimit) {
        i = 0;
        ch = 0;
morebytes:
        while (i < sizeof(uint32_t)) {
            if (mySource < sourceLimit) {
                ch = (ch << 8) | (uint8_t)(*mySource);
                toUBytes[i++] = (char) *(mySource++);
            }
            else {
                
                
                args->converter->toUnicodeStatus = ch + 1;
                args->converter->toULength = (int8_t) i;
                goto donefornow;
            }
        }

        if (ch <= MAXIMUM_UTF && !U_IS_SURROGATE(ch)) {
            
            if (ch <= MAXIMUM_UCS2) {
                
                *(myTarget++) = (UChar) ch;
                *(myOffsets++) = offsetNum;
            }
            else {
                
                *(myTarget++) = U16_LEAD(ch);
                *myOffsets++ = offsetNum;
                ch = U16_TRAIL(ch);
                if (myTarget < targetLimit)
                {
                    *(myTarget++) = (UChar)ch;
                    *(myOffsets++) = offsetNum;
                }
                else {
                    
                    args->converter->UCharErrorBuffer[0] = (UChar) ch;
                    args->converter->UCharErrorBufferLength = 1;
                    *err = U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            }
        }
        else {
            args->converter->toULength = (int8_t)i;
            *err = U_ILLEGAL_CHAR_FOUND;
            break;
        }
        offsetNum += i;
    }

donefornow:
    if (mySource < sourceLimit && myTarget >= targetLimit && U_SUCCESS(*err))
    {
        
        *err = U_BUFFER_OVERFLOW_ERROR;
    }

    args->target = myTarget;
    args->source = (const char *) mySource;
    args->offsets = myOffsets;
}

static void
T_UConverter_fromUnicode_UTF32_BE(UConverterFromUnicodeArgs * args,
                                  UErrorCode * err)
{
    const UChar *mySource = args->source;
    unsigned char *myTarget;
    const UChar *sourceLimit = args->sourceLimit;
    const unsigned char *targetLimit = (unsigned char *) args->targetLimit;
    UChar32 ch, ch2;
    unsigned int indexToWrite;
    unsigned char temp[sizeof(uint32_t)];

    if(mySource >= sourceLimit) {
        
        return;
    }

    
    if(args->converter->fromUnicodeStatus==UCNV_NEED_TO_WRITE_BOM) {
        static const char bom[]={ 0, 0, (char)0xfe, (char)0xff };
        ucnv_fromUWriteBytes(args->converter,
                             bom, 4,
                             &args->target, args->targetLimit,
                             &args->offsets, -1,
                             err);
        args->converter->fromUnicodeStatus=0;
    }

    myTarget = (unsigned char *) args->target;
    temp[0] = 0;

    if (args->converter->fromUChar32) {
        ch = args->converter->fromUChar32;
        args->converter->fromUChar32 = 0;
        goto lowsurogate;
    }

    while (mySource < sourceLimit && myTarget < targetLimit) {
        ch = *(mySource++);

        if (U_IS_SURROGATE(ch)) {
            if (U_IS_LEAD(ch)) {
lowsurogate:
                if (mySource < sourceLimit) {
                    ch2 = *mySource;
                    if (U_IS_TRAIL(ch2)) {
                        ch = ((ch - SURROGATE_HIGH_START) << HALF_SHIFT) + ch2 + SURROGATE_LOW_BASE;
                        mySource++;
                    }
                    else {
                        
                        
                        args->converter->fromUChar32 = ch;
                        *err = U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                }
                else {
                    
                    args->converter->fromUChar32 = ch;
                    if (args->flush) {
                        
                        
                        *err = U_ILLEGAL_CHAR_FOUND;
                    }
                    break;
                }
            }
            else {
                
                
                args->converter->fromUChar32 = ch;
                *err = U_ILLEGAL_CHAR_FOUND;
                break;
            }
        }

        
        temp[1] = (uint8_t) (ch >> 16 & 0x1F);
        temp[2] = (uint8_t) (ch >> 8);  
        temp[3] = (uint8_t) (ch);       

        for (indexToWrite = 0; indexToWrite <= sizeof(uint32_t) - 1; indexToWrite++) {
            if (myTarget < targetLimit) {
                *(myTarget++) = temp[indexToWrite];
            }
            else {
                args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = temp[indexToWrite];
                *err = U_BUFFER_OVERFLOW_ERROR;
            }
        }
    }

    if (mySource < sourceLimit && myTarget >= targetLimit && U_SUCCESS(*err)) {
        *err = U_BUFFER_OVERFLOW_ERROR;
    }

    args->target = (char *) myTarget;
    args->source = mySource;
}

static void
T_UConverter_fromUnicode_UTF32_BE_OFFSET_LOGIC(UConverterFromUnicodeArgs * args,
                                               UErrorCode * err)
{
    const UChar *mySource = args->source;
    unsigned char *myTarget;
    int32_t *myOffsets;
    const UChar *sourceLimit = args->sourceLimit;
    const unsigned char *targetLimit = (unsigned char *) args->targetLimit;
    UChar32 ch, ch2;
    int32_t offsetNum = 0;
    unsigned int indexToWrite;
    unsigned char temp[sizeof(uint32_t)];

    if(mySource >= sourceLimit) {
        
        return;
    }

    
    if(args->converter->fromUnicodeStatus==UCNV_NEED_TO_WRITE_BOM) {
        static const char bom[]={ 0, 0, (char)0xfe, (char)0xff };
        ucnv_fromUWriteBytes(args->converter,
                             bom, 4,
                             &args->target, args->targetLimit,
                             &args->offsets, -1,
                             err);
        args->converter->fromUnicodeStatus=0;
    }

    myTarget = (unsigned char *) args->target;
    myOffsets = args->offsets;
    temp[0] = 0;

    if (args->converter->fromUChar32) {
        ch = args->converter->fromUChar32;
        args->converter->fromUChar32 = 0;
        goto lowsurogate;
    }

    while (mySource < sourceLimit && myTarget < targetLimit) {
        ch = *(mySource++);

        if (U_IS_SURROGATE(ch)) {
            if (U_IS_LEAD(ch)) {
lowsurogate:
                if (mySource < sourceLimit) {
                    ch2 = *mySource;
                    if (U_IS_TRAIL(ch2)) {
                        ch = ((ch - SURROGATE_HIGH_START) << HALF_SHIFT) + ch2 + SURROGATE_LOW_BASE;
                        mySource++;
                    }
                    else {
                        
                        
                        args->converter->fromUChar32 = ch;
                        *err = U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                }
                else {
                    
                    args->converter->fromUChar32 = ch;
                    if (args->flush) {
                        
                        
                        *err = U_ILLEGAL_CHAR_FOUND;
                    }
                    break;
                }
            }
            else {
                
                
                args->converter->fromUChar32 = ch;
                *err = U_ILLEGAL_CHAR_FOUND;
                break;
            }
        }

        
        temp[1] = (uint8_t) (ch >> 16 & 0x1F);
        temp[2] = (uint8_t) (ch >> 8);  
        temp[3] = (uint8_t) (ch);       

        for (indexToWrite = 0; indexToWrite <= sizeof(uint32_t) - 1; indexToWrite++) {
            if (myTarget < targetLimit) {
                *(myTarget++) = temp[indexToWrite];
                *(myOffsets++) = offsetNum;
            }
            else {
                args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = temp[indexToWrite];
                *err = U_BUFFER_OVERFLOW_ERROR;
            }
        }
        offsetNum = offsetNum + 1 + (temp[1] != 0);
    }

    if (mySource < sourceLimit && myTarget >= targetLimit && U_SUCCESS(*err)) {
        *err = U_BUFFER_OVERFLOW_ERROR;
    }

    args->target = (char *) myTarget;
    args->source = mySource;
    args->offsets = myOffsets;
}

static UChar32
T_UConverter_getNextUChar_UTF32_BE(UConverterToUnicodeArgs* args,
                                   UErrorCode* err)
{
    const uint8_t *mySource;
    UChar32 myUChar;
    int32_t length;

    mySource = (const uint8_t *)args->source;
    if (mySource >= (const uint8_t *)args->sourceLimit)
    {
        
        *err = U_INDEX_OUTOFBOUNDS_ERROR;
        return 0xffff;
    }

    length = (int32_t)((const uint8_t *)args->sourceLimit - mySource);
    if (length < 4) 
    {
        
        uprv_memcpy(args->converter->toUBytes, mySource, length);
        args->converter->toULength = (int8_t)length;
        args->source = (const char *)(mySource + length);
        *err = U_TRUNCATED_CHAR_FOUND;
        return 0xffff;
    }

    
    myUChar = ((UChar32)mySource[0] << 24)
            | ((UChar32)mySource[1] << 16)
            | ((UChar32)mySource[2] << 8)
            | ((UChar32)mySource[3]);

    args->source = (const char *)(mySource + 4);
    if ((uint32_t)myUChar <= MAXIMUM_UTF && !U_IS_SURROGATE(myUChar)) {
        return myUChar;
    }

    uprv_memcpy(args->converter->toUBytes, mySource, 4);
    args->converter->toULength = 4;

    *err = U_ILLEGAL_CHAR_FOUND;
    return 0xffff;
}

static const UConverterImpl _UTF32BEImpl = {
    UCNV_UTF32_BigEndian,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,

    T_UConverter_toUnicode_UTF32_BE,
    T_UConverter_toUnicode_UTF32_BE_OFFSET_LOGIC,
    T_UConverter_fromUnicode_UTF32_BE,
    T_UConverter_fromUnicode_UTF32_BE_OFFSET_LOGIC,
    T_UConverter_getNextUChar_UTF32_BE,

    NULL,
    NULL,
    NULL,
    NULL,
    ucnv_getNonSurrogateUnicodeSet
};


static const UConverterStaticData _UTF32BEStaticData = {
    sizeof(UConverterStaticData),
    "UTF-32BE",
    1232,
    UCNV_IBM, UCNV_UTF32_BigEndian, 4, 4,
    { 0, 0, 0xff, 0xfd }, 4, FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

const UConverterSharedData _UTF32BEData = {
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_UTF32BEStaticData, FALSE, &_UTF32BEImpl, 
    0
};



static void
T_UConverter_toUnicode_UTF32_LE(UConverterToUnicodeArgs * args,
                                UErrorCode * err)
{
    const unsigned char *mySource = (unsigned char *) args->source;
    UChar *myTarget = args->target;
    const unsigned char *sourceLimit = (unsigned char *) args->sourceLimit;
    const UChar *targetLimit = args->targetLimit;
    unsigned char *toUBytes = args->converter->toUBytes;
    uint32_t ch, i;

    
    if (args->converter->toUnicodeStatus && myTarget < targetLimit)
    {
        i = args->converter->toULength;       
        args->converter->toULength = 0;

        
        ch = args->converter->toUnicodeStatus - 1;
        args->converter->toUnicodeStatus = 0;
        goto morebytes;
    }

    while (mySource < sourceLimit && myTarget < targetLimit)
    {
        i = 0;
        ch = 0;
morebytes:
        while (i < sizeof(uint32_t))
        {
            if (mySource < sourceLimit)
            {
                ch |= ((uint8_t)(*mySource)) << (i * 8);
                toUBytes[i++] = (char) *(mySource++);
            }
            else
            {
                
                
                args->converter->toUnicodeStatus = ch + 1;
                args->converter->toULength = (int8_t) i;
                goto donefornow;
            }
        }

        if (ch <= MAXIMUM_UTF && !U_IS_SURROGATE(ch)) {
            
            if (ch <= MAXIMUM_UCS2) {
                
                *(myTarget++) = (UChar) ch;
            }
            else {
                
                *(myTarget++) = U16_LEAD(ch);
                ch = U16_TRAIL(ch);
                if (myTarget < targetLimit) {
                    *(myTarget++) = (UChar)ch;
                }
                else {
                    
                    args->converter->UCharErrorBuffer[0] = (UChar) ch;
                    args->converter->UCharErrorBufferLength = 1;
                    *err = U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            }
        }
        else {
            args->converter->toULength = (int8_t)i;
            *err = U_ILLEGAL_CHAR_FOUND;
            break;
        }
    }

donefornow:
    if (mySource < sourceLimit && myTarget >= targetLimit && U_SUCCESS(*err))
    {
        
        *err = U_BUFFER_OVERFLOW_ERROR;
    }

    args->target = myTarget;
    args->source = (const char *) mySource;
}

static void
T_UConverter_toUnicode_UTF32_LE_OFFSET_LOGIC(UConverterToUnicodeArgs * args,
                                             UErrorCode * err)
{
    const unsigned char *mySource = (unsigned char *) args->source;
    UChar *myTarget = args->target;
    int32_t *myOffsets = args->offsets;
    const unsigned char *sourceLimit = (unsigned char *) args->sourceLimit;
    const UChar *targetLimit = args->targetLimit;
    unsigned char *toUBytes = args->converter->toUBytes;
    uint32_t ch, i;
    int32_t offsetNum = 0;

    
    if (args->converter->toUnicodeStatus && myTarget < targetLimit)
    {
        i = args->converter->toULength;       
        args->converter->toULength = 0;

        
        ch = args->converter->toUnicodeStatus - 1;
        args->converter->toUnicodeStatus = 0;
        goto morebytes;
    }

    while (mySource < sourceLimit && myTarget < targetLimit)
    {
        i = 0;
        ch = 0;
morebytes:
        while (i < sizeof(uint32_t))
        {
            if (mySource < sourceLimit)
            {
                ch |= ((uint8_t)(*mySource)) << (i * 8);
                toUBytes[i++] = (char) *(mySource++);
            }
            else
            {
                
                
                args->converter->toUnicodeStatus = ch + 1;
                args->converter->toULength = (int8_t) i;
                goto donefornow;
            }
        }

        if (ch <= MAXIMUM_UTF && !U_IS_SURROGATE(ch))
        {
            
            if (ch <= MAXIMUM_UCS2) 
            {
                
                *(myTarget++) = (UChar) ch;
                *(myOffsets++) = offsetNum;
            }
            else {
                
                *(myTarget++) = U16_LEAD(ch);
                *(myOffsets++) = offsetNum;
                ch = U16_TRAIL(ch);
                if (myTarget < targetLimit)
                {
                    *(myTarget++) = (UChar)ch;
                    *(myOffsets++) = offsetNum;
                }
                else
                {
                    
                    args->converter->UCharErrorBuffer[0] = (UChar) ch;
                    args->converter->UCharErrorBufferLength = 1;
                    *err = U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
            }
        }
        else
        {
            args->converter->toULength = (int8_t)i;
            *err = U_ILLEGAL_CHAR_FOUND;
            break;
        }
        offsetNum += i;
    }

donefornow:
    if (mySource < sourceLimit && myTarget >= targetLimit && U_SUCCESS(*err))
    {
        
        *err = U_BUFFER_OVERFLOW_ERROR;
    }

    args->target = myTarget;
    args->source = (const char *) mySource;
    args->offsets = myOffsets;
}

static void
T_UConverter_fromUnicode_UTF32_LE(UConverterFromUnicodeArgs * args,
                                  UErrorCode * err)
{
    const UChar *mySource = args->source;
    unsigned char *myTarget;
    const UChar *sourceLimit = args->sourceLimit;
    const unsigned char *targetLimit = (unsigned char *) args->targetLimit;
    UChar32 ch, ch2;
    unsigned int indexToWrite;
    unsigned char temp[sizeof(uint32_t)];

    if(mySource >= sourceLimit) {
        
        return;
    }

    
    if(args->converter->fromUnicodeStatus==UCNV_NEED_TO_WRITE_BOM) {
        static const char bom[]={ (char)0xff, (char)0xfe, 0, 0 };
        ucnv_fromUWriteBytes(args->converter,
                             bom, 4,
                             &args->target, args->targetLimit,
                             &args->offsets, -1,
                             err);
        args->converter->fromUnicodeStatus=0;
    }

    myTarget = (unsigned char *) args->target;
    temp[3] = 0;

    if (args->converter->fromUChar32)
    {
        ch = args->converter->fromUChar32;
        args->converter->fromUChar32 = 0;
        goto lowsurogate;
    }

    while (mySource < sourceLimit && myTarget < targetLimit)
    {
        ch = *(mySource++);

        if (U16_IS_SURROGATE(ch)) {
            if (U16_IS_LEAD(ch))
            {
lowsurogate:
                if (mySource < sourceLimit)
                {
                    ch2 = *mySource;
                    if (U16_IS_TRAIL(ch2)) {
                        ch = ((ch - SURROGATE_HIGH_START) << HALF_SHIFT) + ch2 + SURROGATE_LOW_BASE;
                        mySource++;
                    }
                    else {
                        
                        
                        args->converter->fromUChar32 = ch;
                        *err = U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                }
                else {
                    
                    args->converter->fromUChar32 = ch;
                    if (args->flush) {
                        
                        
                        *err = U_ILLEGAL_CHAR_FOUND;
                    }
                    break;
                }
            }
            else {
                
                
                args->converter->fromUChar32 = ch;
                *err = U_ILLEGAL_CHAR_FOUND;
                break;
            }
        }

        
        temp[2] = (uint8_t) (ch >> 16 & 0x1F);
        temp[1] = (uint8_t) (ch >> 8);  
        temp[0] = (uint8_t) (ch);       

        for (indexToWrite = 0; indexToWrite <= sizeof(uint32_t) - 1; indexToWrite++)
        {
            if (myTarget < targetLimit)
            {
                *(myTarget++) = temp[indexToWrite];
            }
            else
            {
                args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = temp[indexToWrite];
                *err = U_BUFFER_OVERFLOW_ERROR;
            }
        }
    }

    if (mySource < sourceLimit && myTarget >= targetLimit && U_SUCCESS(*err))
    {
        *err = U_BUFFER_OVERFLOW_ERROR;
    }

    args->target = (char *) myTarget;
    args->source = mySource;
}

static void
T_UConverter_fromUnicode_UTF32_LE_OFFSET_LOGIC(UConverterFromUnicodeArgs * args,
                                               UErrorCode * err)
{
    const UChar *mySource = args->source;
    unsigned char *myTarget;
    int32_t *myOffsets;
    const UChar *sourceLimit = args->sourceLimit;
    const unsigned char *targetLimit = (unsigned char *) args->targetLimit;
    UChar32 ch, ch2;
    unsigned int indexToWrite;
    unsigned char temp[sizeof(uint32_t)];
    int32_t offsetNum = 0;

    if(mySource >= sourceLimit) {
        
        return;
    }

    
    if(args->converter->fromUnicodeStatus==UCNV_NEED_TO_WRITE_BOM) {
        static const char bom[]={ (char)0xff, (char)0xfe, 0, 0 };
        ucnv_fromUWriteBytes(args->converter,
                             bom, 4,
                             &args->target, args->targetLimit,
                             &args->offsets, -1,
                             err);
        args->converter->fromUnicodeStatus=0;
    }

    myTarget = (unsigned char *) args->target;
    myOffsets = args->offsets;
    temp[3] = 0;

    if (args->converter->fromUChar32)
    {
        ch = args->converter->fromUChar32;
        args->converter->fromUChar32 = 0;
        goto lowsurogate;
    }

    while (mySource < sourceLimit && myTarget < targetLimit)
    {
        ch = *(mySource++);

        if (U16_IS_SURROGATE(ch)) {
            if (U16_IS_LEAD(ch))
            {
lowsurogate:
                if (mySource < sourceLimit)
                {
                    ch2 = *mySource;
                    if (U16_IS_TRAIL(ch2))
                    {
                        ch = ((ch - SURROGATE_HIGH_START) << HALF_SHIFT) + ch2 + SURROGATE_LOW_BASE;
                        mySource++;
                    }
                    else {
                        
                        
                        args->converter->fromUChar32 = ch;
                        *err = U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                }
                else {
                    
                    args->converter->fromUChar32 = ch;
                    if (args->flush) {
                        
                        
                        *err = U_ILLEGAL_CHAR_FOUND;
                    }
                    break;
                }
            }
            else {
                
                
                args->converter->fromUChar32 = ch;
                *err = U_ILLEGAL_CHAR_FOUND;
                break;
            }
        }

        
        temp[2] = (uint8_t) (ch >> 16 & 0x1F);
        temp[1] = (uint8_t) (ch >> 8);  
        temp[0] = (uint8_t) (ch);       

        for (indexToWrite = 0; indexToWrite <= sizeof(uint32_t) - 1; indexToWrite++)
        {
            if (myTarget < targetLimit)
            {
                *(myTarget++) = temp[indexToWrite];
                *(myOffsets++) = offsetNum;
            }
            else
            {
                args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] = temp[indexToWrite];
                *err = U_BUFFER_OVERFLOW_ERROR;
            }
        }
        offsetNum = offsetNum + 1 + (temp[2] != 0);
    }

    if (mySource < sourceLimit && myTarget >= targetLimit && U_SUCCESS(*err))
    {
        *err = U_BUFFER_OVERFLOW_ERROR;
    }

    args->target = (char *) myTarget;
    args->source = mySource;
    args->offsets = myOffsets;
}

static UChar32
T_UConverter_getNextUChar_UTF32_LE(UConverterToUnicodeArgs* args,
                                   UErrorCode* err)
{
    const uint8_t *mySource;
    UChar32 myUChar;
    int32_t length;

    mySource = (const uint8_t *)args->source;
    if (mySource >= (const uint8_t *)args->sourceLimit)
    {
        
        *err = U_INDEX_OUTOFBOUNDS_ERROR;
        return 0xffff;
    }

    length = (int32_t)((const uint8_t *)args->sourceLimit - mySource);
    if (length < 4) 
    {
        
        uprv_memcpy(args->converter->toUBytes, mySource, length);
        args->converter->toULength = (int8_t)length;
        args->source = (const char *)(mySource + length);
        *err = U_TRUNCATED_CHAR_FOUND;
        return 0xffff;
    }

    
    myUChar = ((UChar32)mySource[3] << 24)
            | ((UChar32)mySource[2] << 16)
            | ((UChar32)mySource[1] << 8)
            | ((UChar32)mySource[0]);

    args->source = (const char *)(mySource + 4);
    if ((uint32_t)myUChar <= MAXIMUM_UTF && !U_IS_SURROGATE(myUChar)) {
        return myUChar;
    }

    uprv_memcpy(args->converter->toUBytes, mySource, 4);
    args->converter->toULength = 4;

    *err = U_ILLEGAL_CHAR_FOUND;
    return 0xffff;
}

static const UConverterImpl _UTF32LEImpl = {
    UCNV_UTF32_LittleEndian,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,

    T_UConverter_toUnicode_UTF32_LE,
    T_UConverter_toUnicode_UTF32_LE_OFFSET_LOGIC,
    T_UConverter_fromUnicode_UTF32_LE,
    T_UConverter_fromUnicode_UTF32_LE_OFFSET_LOGIC,
    T_UConverter_getNextUChar_UTF32_LE,

    NULL,
    NULL,
    NULL,
    NULL,
    ucnv_getNonSurrogateUnicodeSet
};


static const UConverterStaticData _UTF32LEStaticData = {
    sizeof(UConverterStaticData),
    "UTF-32LE",
    1234,
    UCNV_IBM, UCNV_UTF32_LittleEndian, 4, 4,
    { 0xfd, 0xff, 0, 0 }, 4, FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};


const UConverterSharedData _UTF32LEData = {
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_UTF32LEStaticData, FALSE, &_UTF32LEImpl, 
    0
};
























static void
_UTF32Reset(UConverter *cnv, UConverterResetChoice choice) {
    if(choice<=UCNV_RESET_TO_UNICODE) {
        
        cnv->mode=0;
    }
    if(choice!=UCNV_RESET_TO_UNICODE) {
        
        cnv->fromUnicodeStatus=UCNV_NEED_TO_WRITE_BOM;
    }
}

static void
_UTF32Open(UConverter *cnv,
           UConverterLoadArgs *pArgs,
           UErrorCode *pErrorCode) {
    _UTF32Reset(cnv, UCNV_RESET_BOTH);
}

static const char utf32BOM[8]={ 0, 0, (char)0xfe, (char)0xff,    (char)0xff, (char)0xfe, 0, 0 };

static void
_UTF32ToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                           UErrorCode *pErrorCode) {
    UConverter *cnv=pArgs->converter;
    const char *source=pArgs->source;
    const char *sourceLimit=pArgs->sourceLimit;
    int32_t *offsets=pArgs->offsets;

    int32_t state, offsetDelta;
    char b;

    state=cnv->mode;

    




    offsetDelta=0;

    while(source<sourceLimit && U_SUCCESS(*pErrorCode)) {
        switch(state) {
        case 0:
            b=*source;
            if(b==0) {
                state=1; 
            } else if(b==(char)0xff) {
                state=5; 
            } else {
                state=8; 
                continue;
            }
            ++source;
            break;
        case 1:
        case 2:
        case 3:
        case 5:
        case 6:
        case 7:
            if(*source==utf32BOM[state]) {
                ++state;
                ++source;
                if(state==4) {
                    state=8; 
                    offsetDelta=(int32_t)(source-pArgs->source);
                } else if(state==8) {
                    state=9; 
                    offsetDelta=(int32_t)(source-pArgs->source);
                }
            } else {
                
                int32_t count=(int32_t)(source-pArgs->source); 

                
                source=pArgs->source;

                if(count==(state&3)) {
                    
                } else {
                    UBool oldFlush=pArgs->flush;

                    
                    pArgs->source=utf32BOM+(state&4); 
                    pArgs->sourceLimit=pArgs->source+((state&3)-count); 
                    pArgs->flush=FALSE; 

                    
                    T_UConverter_toUnicode_UTF32_BE(pArgs, pErrorCode);

                    
                    pArgs->sourceLimit=sourceLimit;
                    pArgs->flush=oldFlush;
                }
                state=8;
                continue;
            }
            break;
        case 8:
            
            pArgs->source=source;
            if(offsets==NULL) {
                T_UConverter_toUnicode_UTF32_BE(pArgs, pErrorCode);
            } else {
                T_UConverter_toUnicode_UTF32_BE_OFFSET_LOGIC(pArgs, pErrorCode);
            }
            source=pArgs->source;
            break;
        case 9:
            
            pArgs->source=source;
            if(offsets==NULL) {
                T_UConverter_toUnicode_UTF32_LE(pArgs, pErrorCode);
            } else {
                T_UConverter_toUnicode_UTF32_LE_OFFSET_LOGIC(pArgs, pErrorCode);
            }
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
            T_UConverter_toUnicode_UTF32_BE(pArgs, pErrorCode);
            break;
        case 9:
            T_UConverter_toUnicode_UTF32_LE(pArgs, pErrorCode);
            break;
        default:
            
            pArgs->source=utf32BOM+(state&4); 
            pArgs->sourceLimit=pArgs->source+(state&3); 

            
            T_UConverter_toUnicode_UTF32_BE(pArgs, pErrorCode);
            pArgs->source=source;
            pArgs->sourceLimit=sourceLimit;
            state=8;
            break;
        }
    }

    cnv->mode=state;
}

static UChar32
_UTF32GetNextUChar(UConverterToUnicodeArgs *pArgs,
                   UErrorCode *pErrorCode) {
    switch(pArgs->converter->mode) {
    case 8:
        return T_UConverter_getNextUChar_UTF32_BE(pArgs, pErrorCode);
    case 9:
        return T_UConverter_getNextUChar_UTF32_LE(pArgs, pErrorCode);
    default:
        return UCNV_GET_NEXT_UCHAR_USE_TO_U;
    }
}

static const UConverterImpl _UTF32Impl = {
    UCNV_UTF32,

    NULL,
    NULL,

    _UTF32Open,
    NULL,
    _UTF32Reset,

    _UTF32ToUnicodeWithOffsets,
    _UTF32ToUnicodeWithOffsets,
#if U_IS_BIG_ENDIAN
    T_UConverter_fromUnicode_UTF32_BE,
    T_UConverter_fromUnicode_UTF32_BE_OFFSET_LOGIC,
#else
    T_UConverter_fromUnicode_UTF32_LE,
    T_UConverter_fromUnicode_UTF32_LE_OFFSET_LOGIC,
#endif
    _UTF32GetNextUChar,

    NULL, 
    NULL,
    NULL,
    NULL,
    ucnv_getNonSurrogateUnicodeSet
};


static const UConverterStaticData _UTF32StaticData = {
    sizeof(UConverterStaticData),
    "UTF-32",
    1236,
    UCNV_IBM, UCNV_UTF32, 4, 4,
#if U_IS_BIG_ENDIAN
    { 0, 0, 0xff, 0xfd }, 4,
#else
    { 0xfd, 0xff, 0, 0 }, 4,
#endif
    FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

const UConverterSharedData _UTF32Data = {
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_UTF32StaticData, FALSE, &_UTF32Impl, 
    0
};

#endif
