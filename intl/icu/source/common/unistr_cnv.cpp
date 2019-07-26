

















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/putil.h"
#include "cstring.h"
#include "cmemory.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/ucnv.h"
#include "ucnv_imp.h"
#include "putilimp.h"
#include "ustr_cnv.h"
#include "ustr_imp.h"

U_NAMESPACE_BEGIN





#if !U_CHARSET_IS_UTF8

UnicodeString::UnicodeString(const char *codepageData)
  : fShortLength(0),
    fFlags(kShortString)
{
    if(codepageData != 0) {
        doCodepageCreate(codepageData, (int32_t)uprv_strlen(codepageData), 0);
    }
}

UnicodeString::UnicodeString(const char *codepageData,
                             int32_t dataLength)
  : fShortLength(0),
    fFlags(kShortString)
{
    if(codepageData != 0) {
        doCodepageCreate(codepageData, dataLength, 0);
    }
}


#endif

UnicodeString::UnicodeString(const char *codepageData,
                             const char *codepage)
  : fShortLength(0),
    fFlags(kShortString)
{
    if(codepageData != 0) {
        doCodepageCreate(codepageData, (int32_t)uprv_strlen(codepageData), codepage);
    }
}

UnicodeString::UnicodeString(const char *codepageData,
                             int32_t dataLength,
                             const char *codepage)
  : fShortLength(0),
    fFlags(kShortString)
{
    if(codepageData != 0) {
        doCodepageCreate(codepageData, dataLength, codepage);
    }
}

UnicodeString::UnicodeString(const char *src, int32_t srcLength,
                             UConverter *cnv,
                             UErrorCode &errorCode)
  : fShortLength(0),
    fFlags(kShortString)
{
    if(U_SUCCESS(errorCode)) {
        
        if(src==NULL) {
            
        } else if(srcLength<-1) {
            errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        } else {
            
            if(srcLength==-1) {
                srcLength=(int32_t)uprv_strlen(src);
            }
            if(srcLength>0) {
                if(cnv!=0) {
                    
                    ucnv_resetToUnicode(cnv);
                    doCodepageCreate(src, srcLength, cnv, errorCode);
                } else {
                    
                    cnv=u_getDefaultConverter(&errorCode);
                    doCodepageCreate(src, srcLength, cnv, errorCode);
                    u_releaseDefaultConverter(cnv);
                }
            }
        }

        if(U_FAILURE(errorCode)) {
            setToBogus();
        }
    }
}





#if !U_CHARSET_IS_UTF8

int32_t
UnicodeString::extract(int32_t start,
                       int32_t length,
                       char *target,
                       uint32_t dstSize) const {
    return extract(start, length, target, dstSize, 0);
}


#endif

int32_t
UnicodeString::extract(int32_t start,
                       int32_t length,
                       char *target,
                       uint32_t dstSize,
                       const char *codepage) const
{
    
    if((dstSize > 0 && target == 0)) {
        return 0;
    }

    
    pinIndices(start, length);

    
    
    
    
    
    int32_t capacity;
    if(dstSize < 0x7fffffff) {
        
        capacity = (int32_t)dstSize;
    } else {
        
        char *targetLimit = (char *)U_MAX_PTR(target);
        
        
        capacity = (int32_t)(targetLimit - target);
    }

    
    UConverter *converter;
    UErrorCode status = U_ZERO_ERROR;

    
    if(length == 0) {
        return u_terminateChars(target, capacity, 0, &status);
    }

    
    
    if (codepage == 0) {
        const char *defaultName = ucnv_getDefaultName();
        if(UCNV_FAST_IS_UTF8(defaultName)) {
            return toUTF8(start, length, target, capacity);
        }
        converter = u_getDefaultConverter(&status);
    } else if (*codepage == 0) {
        
        int32_t destLength;
        if(length <= capacity) {
            destLength = length;
        } else {
            destLength = capacity;
        }
        u_UCharsToChars(getArrayStart() + start, target, destLength);
        return u_terminateChars(target, capacity, length, &status);
    } else {
        converter = ucnv_open(codepage, &status);
    }

    length = doExtract(start, length, target, capacity, converter, status);

    
    if (codepage == 0) {
        u_releaseDefaultConverter(converter);
    } else {
        ucnv_close(converter);
    }

    return length;
}

int32_t
UnicodeString::extract(char *dest, int32_t destCapacity,
                       UConverter *cnv,
                       UErrorCode &errorCode) const
{
    if(U_FAILURE(errorCode)) {
        return 0;
    }

    if(isBogus() || destCapacity<0 || (destCapacity>0 && dest==0)) {
        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    if(isEmpty()) {
        return u_terminateChars(dest, destCapacity, 0, &errorCode);
    }

    
    UBool isDefaultConverter;
    if(cnv==0) {
        isDefaultConverter=TRUE;
        cnv=u_getDefaultConverter(&errorCode);
        if(U_FAILURE(errorCode)) {
            return 0;
        }
    } else {
        isDefaultConverter=FALSE;
        ucnv_resetFromUnicode(cnv);
    }

    
    int32_t len=doExtract(0, length(), dest, destCapacity, cnv, errorCode);

    
    if(isDefaultConverter) {
        u_releaseDefaultConverter(cnv);
    }

    return len;
}

int32_t
UnicodeString::doExtract(int32_t start, int32_t length,
                         char *dest, int32_t destCapacity,
                         UConverter *cnv,
                         UErrorCode &errorCode) const
{
    if(U_FAILURE(errorCode)) {
        if(destCapacity!=0) {
            *dest=0;
        }
        return 0;
    }

    const UChar *src=getArrayStart()+start, *srcLimit=src+length;
    char *originalDest=dest;
    const char *destLimit;

    if(destCapacity==0) {
        destLimit=dest=0;
    } else if(destCapacity==-1) {
        
        destLimit=(char*)U_MAX_PTR(dest);
        
        destCapacity=0x7fffffff;
    } else {
        destLimit=dest+destCapacity;
    }

    
    ucnv_fromUnicode(cnv, &dest, destLimit, &src, srcLimit, 0, TRUE, &errorCode);
    length=(int32_t)(dest-originalDest);

    
    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
        char buffer[1024];

        destLimit=buffer+sizeof(buffer);
        do {
            dest=buffer;
            errorCode=U_ZERO_ERROR;
            ucnv_fromUnicode(cnv, &dest, destLimit, &src, srcLimit, 0, TRUE, &errorCode);
            length+=(int32_t)(dest-buffer);
        } while(errorCode==U_BUFFER_OVERFLOW_ERROR);
    }

    return u_terminateChars(originalDest, destCapacity, length, &errorCode);
}

void
UnicodeString::doCodepageCreate(const char *codepageData,
                                int32_t dataLength,
                                const char *codepage)
{
    
    if(codepageData == 0 || dataLength == 0 || dataLength < -1) {
        return;
    }
    if(dataLength == -1) {
        dataLength = (int32_t)uprv_strlen(codepageData);
    }

    UErrorCode status = U_ZERO_ERROR;

    
    
    
    UConverter *converter;
    if (codepage == 0) {
        const char *defaultName = ucnv_getDefaultName();
        if(UCNV_FAST_IS_UTF8(defaultName)) {
            setToUTF8(StringPiece(codepageData, dataLength));
            return;
        }
        converter = u_getDefaultConverter(&status);
    } else if(*codepage == 0) {
        
        if(cloneArrayIfNeeded(dataLength, dataLength, FALSE)) {
            u_charsToUChars(codepageData, getArrayStart(), dataLength);
            setLength(dataLength);
        } else {
            setToBogus();
        }
        return;
    } else {
        converter = ucnv_open(codepage, &status);
    }

    
    if(U_FAILURE(status)) {
        setToBogus();
        return;
    }

    
    doCodepageCreate(codepageData, dataLength, converter, status);
    if(U_FAILURE(status)) {
        setToBogus();
    }

    
    if(codepage == 0) {
        u_releaseDefaultConverter(converter);
    } else {
        ucnv_close(converter);
    }
}

void
UnicodeString::doCodepageCreate(const char *codepageData,
                                int32_t dataLength,
                                UConverter *converter,
                                UErrorCode &status)
{
    if(U_FAILURE(status)) {
        return;
    }

    
    const char *mySource     = codepageData;
    const char *mySourceEnd  = mySource + dataLength;
    UChar *array, *myTarget;

    
    int32_t arraySize;
    if(dataLength <= US_STACKBUF_SIZE) {
        
        arraySize = US_STACKBUF_SIZE;
    } else {
        
        arraySize = dataLength + (dataLength >> 2);
    }

    
    UBool doCopyArray = FALSE;
    for(;;) {
        if(!cloneArrayIfNeeded(arraySize, arraySize, doCopyArray)) {
            setToBogus();
            break;
        }

        
        array = getArrayStart();
        myTarget = array + length();
        ucnv_toUnicode(converter, &myTarget,  array + getCapacity(),
            &mySource, mySourceEnd, 0, TRUE, &status);

        
        setLength((int32_t)(myTarget - array));

        
        if(status == U_BUFFER_OVERFLOW_ERROR) {
            
            status = U_ZERO_ERROR;

            
            doCopyArray = TRUE;

            
            
            arraySize = (int32_t)(length() + 2 * (mySourceEnd - mySource));
        } else {
            break;
        }
    }
}

U_NAMESPACE_END

#endif
