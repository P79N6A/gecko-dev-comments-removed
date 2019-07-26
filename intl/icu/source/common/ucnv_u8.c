



















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "unicode/utf.h"
#include "unicode/utf8.h"
#include "unicode/utf16.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "cmemory.h"





U_CFUNC void ucnv_fromUnicode_UTF8(UConverterFromUnicodeArgs *args,
                                           UErrorCode *err);
U_CFUNC void ucnv_fromUnicode_UTF8_OFFSETS_LOGIC(UConverterFromUnicodeArgs *args,
                                                        UErrorCode *err);








#define MAXIMUM_UCS2            0x0000FFFF
#define MAXIMUM_UTF             0x0010FFFF
#define MAXIMUM_UCS4            0x7FFFFFFF
#define HALF_SHIFT              10
#define HALF_BASE               0x0010000
#define HALF_MASK               0x3FF
#define SURROGATE_HIGH_START    0xD800
#define SURROGATE_HIGH_END      0xDBFF
#define SURROGATE_LOW_START     0xDC00
#define SURROGATE_LOW_END       0xDFFF


#define SURROGATE_LOW_BASE      9216

static const uint32_t offsetsFromUTF8[7] = {0,
  (uint32_t) 0x00000000, (uint32_t) 0x00003080, (uint32_t) 0x000E2080,
  (uint32_t) 0x03C82080, (uint32_t) 0xFA082080, (uint32_t) 0x82082080
};



static const int8_t bytesFromUTF8[256] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0
};







static const uint32_t
utf8_minChar32[7]={ 0, 0, 0x80, 0x800, 0x10000, 0xffffffff, 0xffffffff };

static void ucnv_toUnicode_UTF8 (UConverterToUnicodeArgs * args,
                                  UErrorCode * err)
{
    UConverter *cnv = args->converter;
    const unsigned char *mySource = (unsigned char *) args->source;
    UChar *myTarget = args->target;
    const unsigned char *sourceLimit = (unsigned char *) args->sourceLimit;
    const UChar *targetLimit = args->targetLimit;
    unsigned char *toUBytes = cnv->toUBytes;
    UBool isCESU8 = (UBool)(cnv->sharedData == &_CESU8Data);
    uint32_t ch, ch2 = 0;
    int32_t i, inBytes;
  
    
    if (cnv->toUnicodeStatus && myTarget < targetLimit)
    {
        inBytes = cnv->mode;            
        i = cnv->toULength;             
        cnv->toULength = 0;

        ch = cnv->toUnicodeStatus;
        cnv->toUnicodeStatus = 0;
        goto morebytes;
    }


    while (mySource < sourceLimit && myTarget < targetLimit)
    {
        ch = *(mySource++);
        if (ch < 0x80)        
        {
            *(myTarget++) = (UChar) ch;
        }
        else
        {
            
            toUBytes[0] = (char)ch;
            inBytes = bytesFromUTF8[ch]; 
            i = 1;

morebytes:
            while (i < inBytes)
            {
                if (mySource < sourceLimit)
                {
                    toUBytes[i] = (char) (ch2 = *mySource);
                    if (!U8_IS_TRAIL(ch2))
                    {
                        break; 
                    }
                    ch = (ch << 6) + ch2;
                    ++mySource;
                    i++;
                }
                else
                {
                    
                    cnv->toUnicodeStatus = ch;
                    cnv->mode = inBytes;
                    cnv->toULength = (int8_t) i;
                    goto donefornow;
                }
            }

            
            ch -= offsetsFromUTF8[inBytes];

            











            if (i == inBytes && ch <= MAXIMUM_UTF && ch >= utf8_minChar32[i] &&
                (isCESU8 ? i <= 3 : !U_IS_SURROGATE(ch)))
            {
                
                if (ch <= MAXIMUM_UCS2) 
                {
                    
                    *(myTarget++) = (UChar) ch;
                }
                else
                {
                    
                    ch -= HALF_BASE;
                    *(myTarget++) = (UChar) ((ch >> HALF_SHIFT) + SURROGATE_HIGH_START);
                    ch = (ch & HALF_MASK) + SURROGATE_LOW_START;
                    if (myTarget < targetLimit)
                    {
                        *(myTarget++) = (UChar)ch;
                    }
                    else
                    {
                        
                        cnv->UCharErrorBuffer[0] = (UChar) ch;
                        cnv->UCharErrorBufferLength = 1;
                        *err = U_BUFFER_OVERFLOW_ERROR;
                        break;
                    }
                }
            }
            else
            {
                cnv->toULength = (int8_t)i;
                *err = U_ILLEGAL_CHAR_FOUND;
                break;
            }
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

static void ucnv_toUnicode_UTF8_OFFSETS_LOGIC (UConverterToUnicodeArgs * args,
                                                UErrorCode * err)
{
    UConverter *cnv = args->converter;
    const unsigned char *mySource = (unsigned char *) args->source;
    UChar *myTarget = args->target;
    int32_t *myOffsets = args->offsets;
    int32_t offsetNum = 0;
    const unsigned char *sourceLimit = (unsigned char *) args->sourceLimit;
    const UChar *targetLimit = args->targetLimit;
    unsigned char *toUBytes = cnv->toUBytes;
    UBool isCESU8 = (UBool)(cnv->sharedData == &_CESU8Data);
    uint32_t ch, ch2 = 0;
    int32_t i, inBytes;

    
    if (cnv->toUnicodeStatus && myTarget < targetLimit)
    {
        inBytes = cnv->mode;            
        i = cnv->toULength;             
        cnv->toULength = 0;

        ch = cnv->toUnicodeStatus;
        cnv->toUnicodeStatus = 0;
        goto morebytes;
    }

    while (mySource < sourceLimit && myTarget < targetLimit)
    {
        ch = *(mySource++);
        if (ch < 0x80)        
        {
            *(myTarget++) = (UChar) ch;
            *(myOffsets++) = offsetNum++;
        }
        else
        {
            toUBytes[0] = (char)ch;
            inBytes = bytesFromUTF8[ch];
            i = 1;

morebytes:
            while (i < inBytes)
            {
                if (mySource < sourceLimit)
                {
                    toUBytes[i] = (char) (ch2 = *mySource);
                    if (!U8_IS_TRAIL(ch2))
                    {
                        break; 
                    }
                    ch = (ch << 6) + ch2;
                    ++mySource;
                    i++;
                }
                else
                {
                    cnv->toUnicodeStatus = ch;
                    cnv->mode = inBytes;
                    cnv->toULength = (int8_t)i;
                    goto donefornow;
                }
            }

            
            ch -= offsetsFromUTF8[inBytes];

            











            if (i == inBytes && ch <= MAXIMUM_UTF && ch >= utf8_minChar32[i] &&
                (isCESU8 ? i <= 3 : !U_IS_SURROGATE(ch)))
            {
                
                if (ch <= MAXIMUM_UCS2) 
                {
                    
                    *(myTarget++) = (UChar) ch;
                    *(myOffsets++) = offsetNum;
                }
                else
                {
                    
                    ch -= HALF_BASE;
                    *(myTarget++) = (UChar) ((ch >> HALF_SHIFT) + SURROGATE_HIGH_START);
                    *(myOffsets++) = offsetNum;
                    ch = (ch & HALF_MASK) + SURROGATE_LOW_START;
                    if (myTarget < targetLimit)
                    {
                        *(myTarget++) = (UChar)ch;
                        *(myOffsets++) = offsetNum;
                    }
                    else
                    {
                        cnv->UCharErrorBuffer[0] = (UChar) ch;
                        cnv->UCharErrorBufferLength = 1;
                        *err = U_BUFFER_OVERFLOW_ERROR;
                    }
                }
                offsetNum += i;
            }
            else
            {
                cnv->toULength = (int8_t)i;
                *err = U_ILLEGAL_CHAR_FOUND;
                break;
            }
        }
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

U_CFUNC void ucnv_fromUnicode_UTF8 (UConverterFromUnicodeArgs * args,
                                    UErrorCode * err)
{
    UConverter *cnv = args->converter;
    const UChar *mySource = args->source;
    const UChar *sourceLimit = args->sourceLimit;
    uint8_t *myTarget = (uint8_t *) args->target;
    const uint8_t *targetLimit = (uint8_t *) args->targetLimit;
    uint8_t *tempPtr;
    UChar32 ch;
    uint8_t tempBuf[4];
    int32_t indexToWrite;
    UBool isNotCESU8 = (UBool)(cnv->sharedData != &_CESU8Data);

    if (cnv->fromUChar32 && myTarget < targetLimit)
    {
        ch = cnv->fromUChar32;
        cnv->fromUChar32 = 0;
        goto lowsurrogate;
    }

    while (mySource < sourceLimit && myTarget < targetLimit)
    {
        ch = *(mySource++);

        if (ch < 0x80)        
        {
            *(myTarget++) = (uint8_t) ch;
        }
        else if (ch < 0x800)  
        {
            *(myTarget++) = (uint8_t) ((ch >> 6) | 0xc0);
            if (myTarget < targetLimit)
            {
                *(myTarget++) = (uint8_t) ((ch & 0x3f) | 0x80);
            }
            else
            {
                cnv->charErrorBuffer[0] = (uint8_t) ((ch & 0x3f) | 0x80);
                cnv->charErrorBufferLength = 1;
                *err = U_BUFFER_OVERFLOW_ERROR;
            }
        }
        else {
            
            if(U16_IS_SURROGATE(ch) && isNotCESU8) {
lowsurrogate:
                if (mySource < sourceLimit) {
                    
                    if(U16_IS_SURROGATE_LEAD(ch) && U16_IS_TRAIL(*mySource)) {
                        
                        ch=U16_GET_SUPPLEMENTARY(ch, *mySource);
                        ++mySource;
                        
                    }
                    else {
                        
                        
                        cnv->fromUChar32 = ch;
                        *err = U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                }
                else {
                    
                    cnv->fromUChar32 = ch;
                    break;
                }
            }

            

            tempPtr = (((targetLimit - myTarget) >= 4) ? myTarget : tempBuf);

            if (ch <= MAXIMUM_UCS2) {
                indexToWrite = 2;
                tempPtr[0] = (uint8_t) ((ch >> 12) | 0xe0);
            }
            else {
                indexToWrite = 3;
                tempPtr[0] = (uint8_t) ((ch >> 18) | 0xf0);
                tempPtr[1] = (uint8_t) (((ch >> 12) & 0x3f) | 0x80);
            }
            tempPtr[indexToWrite-1] = (uint8_t) (((ch >> 6) & 0x3f) | 0x80);
            tempPtr[indexToWrite] = (uint8_t) ((ch & 0x3f) | 0x80);

            if (tempPtr == myTarget) {
                
                myTarget += (indexToWrite + 1);
            }
            else {
                
                for (; tempPtr <= (tempBuf + indexToWrite); tempPtr++) {
                    if (myTarget < targetLimit) {
                        *(myTarget++) = *tempPtr;
                    }
                    else {
                        cnv->charErrorBuffer[cnv->charErrorBufferLength++] = *tempPtr;
                        *err = U_BUFFER_OVERFLOW_ERROR;
                    }
                }
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

U_CFUNC void ucnv_fromUnicode_UTF8_OFFSETS_LOGIC (UConverterFromUnicodeArgs * args,
                                                  UErrorCode * err)
{
    UConverter *cnv = args->converter;
    const UChar *mySource = args->source;
    int32_t *myOffsets = args->offsets;
    const UChar *sourceLimit = args->sourceLimit;
    uint8_t *myTarget = (uint8_t *) args->target;
    const uint8_t *targetLimit = (uint8_t *) args->targetLimit;
    uint8_t *tempPtr;
    UChar32 ch;
    int32_t offsetNum, nextSourceIndex;
    int32_t indexToWrite;
    uint8_t tempBuf[4];
    UBool isNotCESU8 = (UBool)(cnv->sharedData != &_CESU8Data);

    if (cnv->fromUChar32 && myTarget < targetLimit)
    {
        ch = cnv->fromUChar32;
        cnv->fromUChar32 = 0;
        offsetNum = -1;
        nextSourceIndex = 0;
        goto lowsurrogate;
    } else {
        offsetNum = 0;
    }

    while (mySource < sourceLimit && myTarget < targetLimit)
    {
        ch = *(mySource++);

        if (ch < 0x80)        
        {
            *(myOffsets++) = offsetNum++;
            *(myTarget++) = (char) ch;
        }
        else if (ch < 0x800)  
        {
            *(myOffsets++) = offsetNum;
            *(myTarget++) = (uint8_t) ((ch >> 6) | 0xc0);
            if (myTarget < targetLimit)
            {
                *(myOffsets++) = offsetNum++;
                *(myTarget++) = (uint8_t) ((ch & 0x3f) | 0x80);
            }
            else
            {
                cnv->charErrorBuffer[0] = (uint8_t) ((ch & 0x3f) | 0x80);
                cnv->charErrorBufferLength = 1;
                *err = U_BUFFER_OVERFLOW_ERROR;
            }
        }
        else
        
        {
            nextSourceIndex = offsetNum + 1;

            if(U16_IS_SURROGATE(ch) && isNotCESU8) {
lowsurrogate:
                if (mySource < sourceLimit) {
                    
                    if(U16_IS_SURROGATE_LEAD(ch) && U16_IS_TRAIL(*mySource)) {
                        
                        ch=U16_GET_SUPPLEMENTARY(ch, *mySource);
                        ++mySource;
                        ++nextSourceIndex;
                        
                    }
                    else {
                        
                        
                        cnv->fromUChar32 = ch;
                        *err = U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                }
                else {
                    
                    cnv->fromUChar32 = ch;
                    break;
                }
            }

            

            tempPtr = (((targetLimit - myTarget) >= 4) ? myTarget : tempBuf);

            if (ch <= MAXIMUM_UCS2) {
                indexToWrite = 2;
                tempPtr[0] = (uint8_t) ((ch >> 12) | 0xe0);
            }
            else {
                indexToWrite = 3;
                tempPtr[0] = (uint8_t) ((ch >> 18) | 0xf0);
                tempPtr[1] = (uint8_t) (((ch >> 12) & 0x3f) | 0x80);
            }
            tempPtr[indexToWrite-1] = (uint8_t) (((ch >> 6) & 0x3f) | 0x80);
            tempPtr[indexToWrite] = (uint8_t) ((ch & 0x3f) | 0x80);

            if (tempPtr == myTarget) {
                
                myTarget += (indexToWrite + 1);
                myOffsets[0] = offsetNum;
                myOffsets[1] = offsetNum;
                myOffsets[2] = offsetNum;
                if (indexToWrite >= 3) {
                    myOffsets[3] = offsetNum;
                }
                myOffsets += (indexToWrite + 1);
            }
            else {
                
                for (; tempPtr <= (tempBuf + indexToWrite); tempPtr++) {
                    if (myTarget < targetLimit)
                    {
                        *(myOffsets++) = offsetNum;
                        *(myTarget++) = *tempPtr;
                    }
                    else
                    {
                        cnv->charErrorBuffer[cnv->charErrorBufferLength++] = *tempPtr;
                        *err = U_BUFFER_OVERFLOW_ERROR;
                    }
                }
            }
            offsetNum = nextSourceIndex;
        }
    }

    if (mySource < sourceLimit && myTarget >= targetLimit && U_SUCCESS(*err))
    {
        *err = U_BUFFER_OVERFLOW_ERROR;
    }

    args->target = (char *) myTarget;
    args->source = mySource;
    args->offsets = myOffsets;
}

static UChar32 ucnv_getNextUChar_UTF8(UConverterToUnicodeArgs *args,
                                               UErrorCode *err) {
    UConverter *cnv;
    const uint8_t *sourceInitial;
    const uint8_t *source;
    uint16_t extraBytesToWrite;
    uint8_t myByte;
    UChar32 ch;
    int8_t i, isLegalSequence;

    

    cnv = args->converter;
    sourceInitial = source = (const uint8_t *)args->source;
    if (source >= (const uint8_t *)args->sourceLimit)
    {
        
        *err = U_INDEX_OUTOFBOUNDS_ERROR;
        return 0xffff;
    }

    myByte = (uint8_t)*(source++);
    if (myByte < 0x80)
    {
        args->source = (const char *)source;
        return (UChar32)myByte;
    }

    extraBytesToWrite = (uint16_t)bytesFromUTF8[myByte];
    if (extraBytesToWrite == 0) {
        cnv->toUBytes[0] = myByte;
        cnv->toULength = 1;
        *err = U_ILLEGAL_CHAR_FOUND;
        args->source = (const char *)source;
        return 0xffff;
    }

    
    if (((const char *)source + extraBytesToWrite - 1) > args->sourceLimit)
    {
        
        cnv->toUBytes[0] = myByte;
        i = 1;
        *err = U_TRUNCATED_CHAR_FOUND;
        while(source < (const uint8_t *)args->sourceLimit) {
            if(U8_IS_TRAIL(myByte = *source)) {
                cnv->toUBytes[i++] = myByte;
                ++source;
            } else {
                
                *err = U_ILLEGAL_CHAR_FOUND;
                break;
            }
        }
        cnv->toULength = i;
        args->source = (const char *)source;
        return 0xffff;
    }

    isLegalSequence = 1;
    ch = myByte << 6;
    switch(extraBytesToWrite)
    {     
       
    case 6:
        ch += (myByte = *source);
        ch <<= 6;
        if (!U8_IS_TRAIL(myByte))
        {
            isLegalSequence = 0;
            break;
        }
        ++source;
    case 5: 
        ch += (myByte = *source);
        ch <<= 6;
        if (!U8_IS_TRAIL(myByte))
        {
            isLegalSequence = 0;
            break;
        }
        ++source;
    case 4: 
        ch += (myByte = *source);
        ch <<= 6;
        if (!U8_IS_TRAIL(myByte))
        {
            isLegalSequence = 0;
            break;
        }
        ++source;
    case 3: 
        ch += (myByte = *source);
        ch <<= 6;
        if (!U8_IS_TRAIL(myByte))
        {
            isLegalSequence = 0;
            break;
        }
        ++source;
    case 2: 
        ch += (myByte = *source);
        if (!U8_IS_TRAIL(myByte))
        {
            isLegalSequence = 0;
            break;
        }
        ++source;
    };
    ch -= offsetsFromUTF8[extraBytesToWrite];
    args->source = (const char *)source;

    










    if (isLegalSequence &&
        (uint32_t)ch <= MAXIMUM_UTF &&
        (uint32_t)ch >= utf8_minChar32[extraBytesToWrite] &&
        !U_IS_SURROGATE(ch)
    ) {
        return ch; 
    }

    for(i = 0; sourceInitial < source; ++i) {
        cnv->toUBytes[i] = *sourceInitial++;
    }
    cnv->toULength = i;
    *err = U_ILLEGAL_CHAR_FOUND;
    return 0xffff;
} 




static const UChar32
utf8_minLegal[5]={ 0, 0, 0x80, 0x800, 0x10000 };


static const UChar32
utf8_offsets[7]={ 0, 0, 0x3080, 0xE2080, 0x3C82080 };


static void
ucnv_UTF8FromUTF8(UConverterFromUnicodeArgs *pFromUArgs,
                  UConverterToUnicodeArgs *pToUArgs,
                  UErrorCode *pErrorCode) {
    UConverter *utf8;
    const uint8_t *source, *sourceLimit;
    uint8_t *target;
    int32_t targetCapacity;
    int32_t count;

    int8_t oldToULength, toULength, toULimit;

    UChar32 c;
    uint8_t b, t1, t2;

    
    utf8=pToUArgs->converter;
    source=(uint8_t *)pToUArgs->source;
    sourceLimit=(uint8_t *)pToUArgs->sourceLimit;
    target=(uint8_t *)pFromUArgs->target;
    targetCapacity=(int32_t)(pFromUArgs->targetLimit-pFromUArgs->target);

    
    c=(UChar32)utf8->toUnicodeStatus;
    if(c!=0) {
        toULength=oldToULength=utf8->toULength;
        toULimit=(int8_t)utf8->mode;
    } else {
        toULength=oldToULength=toULimit=0;
    }

    count=(int32_t)(sourceLimit-source)+oldToULength;
    if(count<toULimit) {
        



    } else if(targetCapacity<toULimit) {
        



        *pErrorCode=U_USING_DEFAULT_WARNING;
        return;
    } else {
        















        int32_t i;

        if(count>targetCapacity) {
            count=targetCapacity;
        }

        i=0;
        while(i<3 && i<(count-toULimit)) {
            b=source[count-oldToULength-i-1];
            if(U8_IS_TRAIL(b)) {
                ++i;
            } else {
                if(i<U8_COUNT_TRAIL_BYTES(b)) {
                    
                    count-=i+1;
                }
                break;
            }
        }
    }

    if(c!=0) {
        utf8->toUnicodeStatus=0;
        utf8->toULength=0;
        goto moreBytes;
        
    }

    
    while(count>0) {
        b=*source++;
        if((int8_t)b>=0) {
            
            *target++=b;
            --count;
            continue;
        } else {
            if(b>0xe0) {
                if( 
                    (t1=source[0]) >= 0x80 && ((b<0xed && (t1 <= 0xbf)) ||
                                               (b==0xed && (t1 <= 0x9f))) &&
                    (t2=source[1]) >= 0x80 && t2 <= 0xbf
                ) {
                    source+=2;
                    *target++=b;
                    *target++=t1;
                    *target++=t2;
                    count-=3;
                    continue;
                }
            } else if(b<0xe0) {
                if( 
                    b>=0xc2 &&
                    (t1=*source) >= 0x80 && t1 <= 0xbf
                ) {
                    ++source;
                    *target++=b;
                    *target++=t1;
                    count-=2;
                    continue;
                }
            } else if(b==0xe0) {
                if( 
                    (t1=source[0]) >= 0xa0 && t1 <= 0xbf &&
                    (t2=source[1]) >= 0x80 && t2 <= 0xbf
                ) {
                    source+=2;
                    *target++=b;
                    *target++=t1;
                    *target++=t2;
                    count-=3;
                    continue;
                }
            }

            
            oldToULength=0;
            toULength=1;
            toULimit=U8_COUNT_TRAIL_BYTES(b)+1;
            c=b;
moreBytes:
            while(toULength<toULimit) {
                if(source<sourceLimit) {
                    b=*source;
                    if(U8_IS_TRAIL(b)) {
                        ++source;
                        ++toULength;
                        c=(c<<6)+b;
                    } else {
                        break; 
                    }
                } else {
                    
                    source-=(toULength-oldToULength);
                    while(oldToULength<toULength) {
                        utf8->toUBytes[oldToULength++]=*source++;
                    }
                    utf8->toUnicodeStatus=c;
                    utf8->toULength=toULength;
                    utf8->mode=toULimit;
                    pToUArgs->source=(char *)source;
                    pFromUArgs->target=(char *)target;
                    return;
                }
            }

            if( toULength==toULimit &&      
                (toULength==3 || toULength==2) &&             
                (c-=utf8_offsets[toULength])>=utf8_minLegal[toULength] &&
                (c<=0xd7ff || 0xe000<=c)    
            ) {
                
            } else if(
                toULength==toULimit && toULength==4 &&
                (0x10000<=(c-=utf8_offsets[4]) && c<=0x10ffff)
            ) {
                
            } else {
                
                source-=(toULength-oldToULength);
                while(oldToULength<toULength) {
                    utf8->toUBytes[oldToULength++]=*source++;
                }
                utf8->toULength=toULength;
                pToUArgs->source=(char *)source;
                pFromUArgs->target=(char *)target;
                *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                return;
            }

            
            {
                int8_t i;

                for(i=0; i<oldToULength; ++i) {
                    *target++=utf8->toUBytes[i];
                }
                source-=(toULength-oldToULength);
                for(; i<toULength; ++i) {
                    *target++=*source++;
                }
                count-=toULength;
            }
        }
    }

    if(U_SUCCESS(*pErrorCode) && source<sourceLimit) {
        if(target==(const uint8_t *)pFromUArgs->targetLimit) {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        } else {
            b=*source;
            toULimit=U8_COUNT_TRAIL_BYTES(b)+1;
            if(toULimit>(sourceLimit-source)) {
                
                toULength=0;
                c=b;
                for(;;) {
                    utf8->toUBytes[toULength++]=b;
                    if(++source==sourceLimit) {
                        
                        utf8->toUnicodeStatus=c;
                        utf8->toULength=toULength;
                        utf8->mode=toULimit;
                        break;
                    } else if(!U8_IS_TRAIL(b=*source)) {
                        
                        utf8->toULength=toULength;
                        *pErrorCode=U_ILLEGAL_CHAR_FOUND;
                        break;
                    }
                    c=(c<<6)+b;
                }
            } else {
                
                *pErrorCode=U_USING_DEFAULT_WARNING;
            }
        }
    }

    
    pToUArgs->source=(char *)source;
    pFromUArgs->target=(char *)target;
}



static const UConverterImpl _UTF8Impl={
    UCNV_UTF8,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,

    ucnv_toUnicode_UTF8,
    ucnv_toUnicode_UTF8_OFFSETS_LOGIC,
    ucnv_fromUnicode_UTF8,
    ucnv_fromUnicode_UTF8_OFFSETS_LOGIC,
    ucnv_getNextUChar_UTF8,

    NULL,
    NULL,
    NULL,
    NULL,
    ucnv_getNonSurrogateUnicodeSet,

    ucnv_UTF8FromUTF8,
    ucnv_UTF8FromUTF8
};


static const UConverterStaticData _UTF8StaticData={
    sizeof(UConverterStaticData),
    "UTF-8",
    1208, UCNV_IBM, UCNV_UTF8,
    1, 3, 
    { 0xef, 0xbf, 0xbd, 0 },3,FALSE,FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};


const UConverterSharedData _UTF8Data={
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_UTF8StaticData, FALSE, &_UTF8Impl,
    0
};



static const UConverterImpl _CESU8Impl={
    UCNV_CESU8,

    NULL,
    NULL,

    NULL,
    NULL,
    NULL,

    ucnv_toUnicode_UTF8,
    ucnv_toUnicode_UTF8_OFFSETS_LOGIC,
    ucnv_fromUnicode_UTF8,
    ucnv_fromUnicode_UTF8_OFFSETS_LOGIC,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    ucnv_getCompleteUnicodeSet
};

static const UConverterStaticData _CESU8StaticData={
    sizeof(UConverterStaticData),
    "CESU-8",
    9400, 
    UCNV_UNKNOWN, UCNV_CESU8, 1, 3,
    { 0xef, 0xbf, 0xbd, 0 },3,FALSE,FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};


const UConverterSharedData _CESU8Data={
    sizeof(UConverterSharedData), ~((uint32_t) 0),
    NULL, NULL, &_CESU8StaticData, FALSE, &_CESU8Impl,
    0
};

#endif
