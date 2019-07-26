














#include "unicode/utypes.h"
#include "unicode/uspoof.h"
#include "unicode/unorm.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "uspoof_impl.h"
#include "uassert.h"


#if !UCONFIG_NO_NORMALIZATION

U_NAMESPACE_USE


U_CAPI USpoofChecker * U_EXPORT2
uspoof_open(UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }
    SpoofImpl *si = new SpoofImpl(SpoofData::getDefault(*status), *status);
    if (U_FAILURE(*status)) {
        delete si;
        si = NULL;
    }
    return (USpoofChecker *)si;
}


U_CAPI USpoofChecker * U_EXPORT2
uspoof_openFromSerialized(const void *data, int32_t length, int32_t *pActualLength,
                          UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }
    SpoofData *sd = new SpoofData(data, length, *status);
    SpoofImpl *si = new SpoofImpl(sd, *status);
    if (U_FAILURE(*status)) {
        delete sd;
        delete si;
        return NULL;
    }
    if (sd == NULL || si == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        delete sd;
        delete si;
        return NULL;
    }
        
    if (pActualLength != NULL) {
        *pActualLength = sd->fRawData->fLength;
    }
    return reinterpret_cast<USpoofChecker *>(si);
}


U_CAPI USpoofChecker * U_EXPORT2
uspoof_clone(const USpoofChecker *sc, UErrorCode *status) {
    const SpoofImpl *src = SpoofImpl::validateThis(sc, *status);
    if (src == NULL) {
        return NULL;
    }
    SpoofImpl *result = new SpoofImpl(*src, *status);   
    if (U_FAILURE(*status)) {
        delete result;
        result = NULL;
    }
    return (USpoofChecker *)result;
}


U_CAPI void U_EXPORT2
uspoof_close(USpoofChecker *sc) {
    UErrorCode status = U_ZERO_ERROR;
    SpoofImpl *This = SpoofImpl::validateThis(sc, status);
    delete This;
}


U_CAPI void U_EXPORT2
uspoof_setChecks(USpoofChecker *sc, int32_t checks, UErrorCode *status) {
    SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (This == NULL) {
        return;
    }

    
    
    if (checks & ~USPOOF_ALL_CHECKS) {
        *status = U_ILLEGAL_ARGUMENT_ERROR; 
        return;
    }

    This->fChecks = checks;
}


U_CAPI int32_t U_EXPORT2
uspoof_getChecks(const USpoofChecker *sc, UErrorCode *status) {
    const SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (This == NULL) {
        return 0;
    }
    return This->fChecks;
}

U_CAPI void U_EXPORT2
uspoof_setAllowedLocales(USpoofChecker *sc, const char *localesList, UErrorCode *status) {
    SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (This == NULL) {
        return;
    }
    This->setAllowedLocales(localesList, *status);
}

U_CAPI const char * U_EXPORT2
uspoof_getAllowedLocales(USpoofChecker *sc, UErrorCode *status) {
    SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (This == NULL) {
        return NULL;
    }
    return This->getAllowedLocales(*status);
}


U_CAPI const USet * U_EXPORT2
uspoof_getAllowedChars(const USpoofChecker *sc, UErrorCode *status) {
    const UnicodeSet *result = uspoof_getAllowedUnicodeSet(sc, status);
    return reinterpret_cast<const USet *>(result);
}

U_CAPI const UnicodeSet * U_EXPORT2
uspoof_getAllowedUnicodeSet(const USpoofChecker *sc, UErrorCode *status) {
    const SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (This == NULL) {
        return NULL;
    }
    return This->fAllowedCharsSet;
}


U_CAPI void U_EXPORT2
uspoof_setAllowedChars(USpoofChecker *sc, const USet *chars, UErrorCode *status) {
    const UnicodeSet *set = reinterpret_cast<const UnicodeSet *>(chars);
    uspoof_setAllowedUnicodeSet(sc, set, status);
}


U_CAPI void U_EXPORT2
uspoof_setAllowedUnicodeSet(USpoofChecker *sc, const UnicodeSet *chars, UErrorCode *status) {
    SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (This == NULL) {
        return;
    }
    if (chars->isBogus()) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    UnicodeSet *clonedSet = static_cast<UnicodeSet *>(chars->clone());
    if (clonedSet == NULL || clonedSet->isBogus()) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    clonedSet->freeze();
    delete This->fAllowedCharsSet;
    This->fAllowedCharsSet = clonedSet;
    This->fChecks |= USPOOF_CHAR_LIMIT;
}


U_CAPI int32_t U_EXPORT2
uspoof_check(const USpoofChecker *sc,
             const UChar *text, int32_t length,
             int32_t *position,
             UErrorCode *status) {
             
    const SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (This == NULL) {
        return 0;
    }
    if (length < -1) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if (length == -1) {
        
        
        length = u_strlen(text);
    }

    int32_t result = 0;
    int32_t failPos = 0x7fffffff;   

    
    
    
    
    int32_t scriptCount = -1;

    if ((This->fChecks) & USPOOF_SINGLE_SCRIPT) {
        scriptCount = This->scriptScan(text, length, failPos, *status);
        
        if ( scriptCount >= 2) {
            
            result |= USPOOF_SINGLE_SCRIPT;
        }
    }

    if (This->fChecks & USPOOF_CHAR_LIMIT) {
        int32_t i;
        UChar32 c;
        for (i=0; i<length ;) {
            U16_NEXT(text, i, length, c);
            if (!This->fAllowedCharsSet->contains(c)) {
                result |= USPOOF_CHAR_LIMIT;
                if (i < failPos) {
                    failPos = i;
                }
                break;
            }
        }
    }

    if (This->fChecks & 
        (USPOOF_WHOLE_SCRIPT_CONFUSABLE | USPOOF_MIXED_SCRIPT_CONFUSABLE | USPOOF_INVISIBLE)) {
        
        NFDBuffer   normalizedInput(text, length, *status);
        const UChar  *nfdText = normalizedInput.getBuffer();
        int32_t      nfdLength = normalizedInput.getLength();

        if (This->fChecks & USPOOF_INVISIBLE) {
           
            
            
            int32_t     i;
            UChar32     c;
            UChar32     firstNonspacingMark = 0;
            UBool       haveMultipleMarks = FALSE;  
            UnicodeSet  marksSeenSoFar;   
            
            for (i=0; i<nfdLength ;) {
                U16_NEXT(nfdText, i, nfdLength, c);
                if (u_charType(c) != U_NON_SPACING_MARK) {
                    firstNonspacingMark = 0;
                    if (haveMultipleMarks) {
                        marksSeenSoFar.clear();
                        haveMultipleMarks = FALSE;
                    }
                    continue;
                }
                if (firstNonspacingMark == 0) {
                    firstNonspacingMark = c;
                    continue;
                }
                if (!haveMultipleMarks) {
                    marksSeenSoFar.add(firstNonspacingMark);
                    haveMultipleMarks = TRUE;
                }
                if (marksSeenSoFar.contains(c)) {
                    
                    
                    result |= USPOOF_INVISIBLE;
                    failPos = i;
                    
                    
                    if (failPos > length) {
                        failPos = length;
                    }
                    break;
                }
                marksSeenSoFar.add(c);
            }
        }
       
        
        if (This->fChecks & (USPOOF_WHOLE_SCRIPT_CONFUSABLE | USPOOF_MIXED_SCRIPT_CONFUSABLE)) {
            
            
            
            
            
            
            
            
            
            
            

            if (scriptCount == -1) {
                int32_t t;
                scriptCount = This->scriptScan(text, length, t, *status);
            }
            
            ScriptSet scripts;
            This->wholeScriptCheck(nfdText, nfdLength, &scripts, *status);
            int32_t confusableScriptCount = scripts.countMembers();
            
            
            if ((This->fChecks & USPOOF_WHOLE_SCRIPT_CONFUSABLE) &&
                confusableScriptCount >= 2 &&
                scriptCount == 1) {
                result |= USPOOF_WHOLE_SCRIPT_CONFUSABLE;
            }
        
            if ((This->fChecks & USPOOF_MIXED_SCRIPT_CONFUSABLE) &&
                confusableScriptCount >= 1 &&
                scriptCount > 1) {
                result |= USPOOF_MIXED_SCRIPT_CONFUSABLE;
            }
        }
    }
    if (position != NULL && failPos != 0x7fffffff) {
        *position = failPos;
    }
    return result;
}


U_CAPI int32_t U_EXPORT2
uspoof_checkUTF8(const USpoofChecker *sc,
                 const char *text, int32_t length,
                 int32_t *position,
                 UErrorCode *status) {

    if (U_FAILURE(*status)) {
        return 0;
    }
    UChar stackBuf[USPOOF_STACK_BUFFER_SIZE];
    UChar* text16 = stackBuf;
    int32_t len16;
    
    u_strFromUTF8(text16, USPOOF_STACK_BUFFER_SIZE, &len16, text, length, status);
    if (U_FAILURE(*status) && *status != U_BUFFER_OVERFLOW_ERROR) {
        return 0;
    }
    if (*status == U_BUFFER_OVERFLOW_ERROR) {
        text16 = static_cast<UChar *>(uprv_malloc(len16 * sizeof(UChar) + 2));
        if (text16 == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        *status = U_ZERO_ERROR;
        u_strFromUTF8(text16, len16+1, NULL, text, length, status);
    }

    int32_t position16 = -1;
    int32_t result = uspoof_check(sc, text16, len16, &position16, status);
    if (U_FAILURE(*status)) {
        return 0;
    }

    if (position16 > 0) {
        
        
        U_ASSERT(position16 <= len16);
        u_strToUTF8(NULL, 0, position, text16, position16, status);
        if (position > 0) {
            
            
            *position -= 1;
        }
        *status = U_ZERO_ERROR;   
    }

    if (text16 != stackBuf) {
        uprv_free(text16);
    }
    return result;
    
}




static UChar *getSkeleton(const USpoofChecker *sc, uint32_t type, const UChar *s, int32_t inputLength,
                         UChar *dest, int32_t destCapacity, int32_t *outputLength, UErrorCode *status) {
    int32_t requiredCapacity = 0;
    UChar *buf = dest;

    if (U_FAILURE(*status)) {
        return NULL;
    }
    requiredCapacity = uspoof_getSkeleton(sc, type, s, inputLength, dest, destCapacity, status);
    if (*status == U_BUFFER_OVERFLOW_ERROR) {
        buf = static_cast<UChar *>(uprv_malloc(requiredCapacity * sizeof(UChar)));
        if (buf == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        *status = U_ZERO_ERROR;
        uspoof_getSkeleton(sc, type, s, inputLength, buf, requiredCapacity, status);
    }
    *outputLength = requiredCapacity;
    return buf;
}


U_CAPI int32_t U_EXPORT2
uspoof_areConfusable(const USpoofChecker *sc,
                     const UChar *s1, int32_t length1,
                     const UChar *s2, int32_t length2,
                     UErrorCode *status) {
    const SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (U_FAILURE(*status)) {
        return 0;
    }
    
    
    
    
    
    
    
    
    if ((This->fChecks & (USPOOF_SINGLE_SCRIPT_CONFUSABLE | USPOOF_MIXED_SCRIPT_CONFUSABLE | 
                          USPOOF_WHOLE_SCRIPT_CONFUSABLE)) == 0) {
        *status = U_INVALID_STATE_ERROR;
        return 0;
    }
    int32_t  flagsForSkeleton = This->fChecks & USPOOF_ANY_CASE;
    UChar    s1SkeletonBuf[USPOOF_STACK_BUFFER_SIZE];
    UChar   *s1Skeleton;
    int32_t  s1SkeletonLength = 0;

    UChar    s2SkeletonBuf[USPOOF_STACK_BUFFER_SIZE];
    UChar   *s2Skeleton;
    int32_t  s2SkeletonLength = 0;

    int32_t  result = 0;
    int32_t  t;
    int32_t  s1ScriptCount = This->scriptScan(s1, length1, t, *status);
    int32_t  s2ScriptCount = This->scriptScan(s2, length2, t, *status);

    if (This->fChecks & USPOOF_SINGLE_SCRIPT_CONFUSABLE) {
        
        if (s1ScriptCount <= 1 && s2ScriptCount <= 1) {
            flagsForSkeleton |= USPOOF_SINGLE_SCRIPT_CONFUSABLE;
            s1Skeleton = getSkeleton(sc, flagsForSkeleton, s1, length1, s1SkeletonBuf, 
                                     sizeof(s1SkeletonBuf)/sizeof(UChar), &s1SkeletonLength, status);
            s2Skeleton = getSkeleton(sc, flagsForSkeleton, s2, length2, s2SkeletonBuf, 
                                     sizeof(s2SkeletonBuf)/sizeof(UChar), &s2SkeletonLength, status);
            if (s1SkeletonLength == s2SkeletonLength && u_strncmp(s1Skeleton, s2Skeleton, s1SkeletonLength) == 0) {
                result |= USPOOF_SINGLE_SCRIPT_CONFUSABLE;
            }
            if (s1Skeleton != s1SkeletonBuf) {
                uprv_free(s1Skeleton);
            }
            if (s2Skeleton != s2SkeletonBuf) {
                uprv_free(s2Skeleton);
            }
        }
    }

    if (result & USPOOF_SINGLE_SCRIPT_CONFUSABLE) {
         
         
         
         return result;
    }

    
    
    UBool possiblyWholeScriptConfusables = 
        s1ScriptCount <= 1 && s2ScriptCount <= 1 && (This->fChecks & USPOOF_WHOLE_SCRIPT_CONFUSABLE);

    
    
    
    if ((This->fChecks & USPOOF_MIXED_SCRIPT_CONFUSABLE) || possiblyWholeScriptConfusables ) {
        
        
        
        flagsForSkeleton &= ~USPOOF_SINGLE_SCRIPT_CONFUSABLE;
        s1Skeleton = getSkeleton(sc, flagsForSkeleton, s1, length1, s1SkeletonBuf, 
                                 sizeof(s1SkeletonBuf)/sizeof(UChar), &s1SkeletonLength, status);
        s2Skeleton = getSkeleton(sc, flagsForSkeleton, s2, length2, s2SkeletonBuf, 
                                 sizeof(s2SkeletonBuf)/sizeof(UChar), &s2SkeletonLength, status);
        if (s1SkeletonLength == s2SkeletonLength && u_strncmp(s1Skeleton, s2Skeleton, s1SkeletonLength) == 0) {
            result |= USPOOF_MIXED_SCRIPT_CONFUSABLE;
            if (possiblyWholeScriptConfusables) {
                result |= USPOOF_WHOLE_SCRIPT_CONFUSABLE;
            }
        }
        if (s1Skeleton != s1SkeletonBuf) {
            uprv_free(s1Skeleton);
        }
        if (s2Skeleton != s2SkeletonBuf) {
            uprv_free(s2Skeleton);
        }
    }

    return result;
}






static UChar * convertFromUTF8(UChar *outBuf, int32_t outBufCapacity, int32_t *outputLength,
                               const char *in, int32_t inLength, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }
    UChar *dest = outBuf;
    u_strFromUTF8(dest, outBufCapacity, outputLength, in, inLength, status);
    if (*status == U_BUFFER_OVERFLOW_ERROR) {
        dest = static_cast<UChar *>(uprv_malloc(*outputLength * sizeof(UChar)));
        if (dest == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        *status = U_ZERO_ERROR;
        u_strFromUTF8(dest, *outputLength, NULL, in, inLength, status);
    }
    return dest;
}

    

U_CAPI int32_t U_EXPORT2
uspoof_areConfusableUTF8(const USpoofChecker *sc,
                         const char *s1, int32_t length1,
                         const char *s2, int32_t length2,
                         UErrorCode *status) {

    SpoofImpl::validateThis(sc, *status);
    if (U_FAILURE(*status)) {
        return 0;
    }

    UChar    s1Buf[USPOOF_STACK_BUFFER_SIZE];
    int32_t  lengthS1U;
    UChar   *s1U = convertFromUTF8(s1Buf, USPOOF_STACK_BUFFER_SIZE, &lengthS1U, s1, length1, status);

    UChar    s2Buf[USPOOF_STACK_BUFFER_SIZE];
    int32_t  lengthS2U;
    UChar   *s2U = convertFromUTF8(s2Buf, USPOOF_STACK_BUFFER_SIZE, &lengthS2U, s2, length2, status);

    int32_t results = uspoof_areConfusable(sc, s1U, lengthS1U, s2U, lengthS2U, status);
    
    if (s1U != s1Buf) {
        uprv_free(s1U);
    }
    if (s2U != s2Buf) {
        uprv_free(s2U);
    }
    return results;
}
 

U_CAPI int32_t U_EXPORT2
uspoof_areConfusableUnicodeString(const USpoofChecker *sc,
                                  const icu::UnicodeString &s1,
                                  const icu::UnicodeString &s2,
                                  UErrorCode *status) {

    const UChar *u1  = s1.getBuffer();
    int32_t  length1 = s1.length();
    const UChar *u2  = s2.getBuffer();
    int32_t  length2 = s2.length();

    int32_t results  = uspoof_areConfusable(sc, u1, length1, u2, length2, status);
    return results;
}




U_CAPI int32_t U_EXPORT2
uspoof_checkUnicodeString(const USpoofChecker *sc,
                          const icu::UnicodeString &text, 
                          int32_t *position,
                          UErrorCode *status) {
    int32_t result = uspoof_check(sc, text.getBuffer(), text.length(), position, status);
    return result;
}


U_CAPI int32_t U_EXPORT2
uspoof_getSkeleton(const USpoofChecker *sc,
                   uint32_t type,
                   const UChar *s,  int32_t length,
                   UChar *dest, int32_t destCapacity,
                   UErrorCode *status) {

    
    
    
    
    

    const SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (U_FAILURE(*status)) {
        return 0;
    }
    if (length<-1 || destCapacity<0 || (destCapacity==0 && dest!=NULL) ||
        (type & ~(USPOOF_SINGLE_SCRIPT_CONFUSABLE | USPOOF_ANY_CASE)) != 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

   int32_t tableMask = 0;
   switch (type) {
      case 0:
        tableMask = USPOOF_ML_TABLE_FLAG;
        break;
      case USPOOF_SINGLE_SCRIPT_CONFUSABLE:
        tableMask = USPOOF_SL_TABLE_FLAG;
        break;
      case USPOOF_ANY_CASE:
        tableMask = USPOOF_MA_TABLE_FLAG;
        break;
      case USPOOF_SINGLE_SCRIPT_CONFUSABLE | USPOOF_ANY_CASE:
        tableMask = USPOOF_SA_TABLE_FLAG;
        break;
      default:
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    
    UChar nfdStackBuf[USPOOF_STACK_BUFFER_SIZE];
    UChar *nfdInput = nfdStackBuf;
    int32_t normalizedLen = unorm_normalize(
        s, length, UNORM_NFD, 0, nfdInput, USPOOF_STACK_BUFFER_SIZE, status);
    if (*status == U_BUFFER_OVERFLOW_ERROR) {
        nfdInput = (UChar *)uprv_malloc((normalizedLen+1)*sizeof(UChar));
        if (nfdInput == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        *status = U_ZERO_ERROR;
        normalizedLen = unorm_normalize(s, length, UNORM_NFD, 0,
                                        nfdInput, normalizedLen+1, status);
    }
    if (U_FAILURE(*status)) {
        if (nfdInput != nfdStackBuf) {
            uprv_free(nfdInput);
        }
        return 0;
    }

    
    UChar buf[USPOOF_MAX_SKELETON_EXPANSION];

    
    
    int32_t inputIndex = 0;
    UnicodeString skelStr;
    while (inputIndex < normalizedLen) {
        UChar32 c;
        U16_NEXT(nfdInput, inputIndex, normalizedLen, c);
        int32_t replaceLen = This->confusableLookup(c, tableMask, buf);
        skelStr.append(buf, replaceLen);
    }

    if (nfdInput != nfdStackBuf) {
        uprv_free(nfdInput);
    }
    
    const UChar *result = skelStr.getBuffer();
    int32_t  resultLen  = skelStr.length();
    UChar   *normedResult = NULL;

    
    
    if (!unorm_isNormalized(result, resultLen, UNORM_NFD, status)) {
        normalizedLen = unorm_normalize(result, resultLen, UNORM_NFD, 0, NULL, 0, status);
        normedResult = static_cast<UChar *>(uprv_malloc((normalizedLen+1)*sizeof(UChar)));
        if (normedResult == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        *status = U_ZERO_ERROR;
        unorm_normalize(result, resultLen, UNORM_NFD, 0, normedResult, normalizedLen+1, status);
        result = normedResult;
        resultLen = normalizedLen;
    }

    
    if (U_SUCCESS(*status)) {
        if (destCapacity == 0 || resultLen > destCapacity) {
            *status = resultLen>destCapacity ? U_BUFFER_OVERFLOW_ERROR : U_STRING_NOT_TERMINATED_WARNING;
        } else {
            u_memcpy(dest, result, resultLen);
            if (destCapacity > resultLen) {
                dest[resultLen] = 0;
            } else {
                *status = U_STRING_NOT_TERMINATED_WARNING;
            }
        }
     }       
     uprv_free(normedResult);
     return resultLen;
}



U_I18N_API UnicodeString &  U_EXPORT2
uspoof_getSkeletonUnicodeString(const USpoofChecker *sc,
                                uint32_t type,
                                const UnicodeString &s,
                                UnicodeString &dest,
                                UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return dest;
    }
    dest.remove();
    
    const UChar *str = s.getBuffer();
    int32_t      strLen = s.length();
    UChar        smallBuf[USPOOF_STACK_BUFFER_SIZE];
    UChar       *buf = smallBuf;
    int32_t outputSize = uspoof_getSkeleton(sc, type, str, strLen, smallBuf, USPOOF_STACK_BUFFER_SIZE, status);
    if (*status == U_BUFFER_OVERFLOW_ERROR) {
        buf = static_cast<UChar *>(uprv_malloc((outputSize+1)*sizeof(UChar)));
        if (buf == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return dest;
        }
        *status = U_ZERO_ERROR;
        uspoof_getSkeleton(sc, type, str, strLen, buf, outputSize+1, status);
    }
    if (U_SUCCESS(*status)) {
        dest.setTo(buf, outputSize);
    }

    if (buf != smallBuf) {
        uprv_free(buf);
    }
    return dest;
}


U_CAPI int32_t U_EXPORT2
uspoof_getSkeletonUTF8(const USpoofChecker *sc,
                       uint32_t type,
                       const char *s,  int32_t length,
                       char *dest, int32_t destCapacity,
                       UErrorCode *status) {
    
    
    
    if (U_FAILURE(*status)) {
        return 0;
    }
    
    UChar    smallInBuf[USPOOF_STACK_BUFFER_SIZE];
    UChar   *inBuf = smallInBuf;
    UChar    smallOutBuf[USPOOF_STACK_BUFFER_SIZE];
    UChar   *outBuf = smallOutBuf;

    int32_t  lengthInUChars = 0;
    int32_t  skelLengthInUChars = 0;
    int32_t  skelLengthInUTF8 = 0;
    
    u_strFromUTF8(inBuf, USPOOF_STACK_BUFFER_SIZE, &lengthInUChars,
                  s, length, status);
    if (*status == U_BUFFER_OVERFLOW_ERROR) {
        inBuf = static_cast<UChar *>(uprv_malloc((lengthInUChars+1)*sizeof(UChar)));
        if (inBuf == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        *status = U_ZERO_ERROR;
        u_strFromUTF8(inBuf, lengthInUChars+1, &lengthInUChars,
                      s, length, status);
    }
    
    skelLengthInUChars = uspoof_getSkeleton(sc, type, inBuf, lengthInUChars,
                                         outBuf, USPOOF_STACK_BUFFER_SIZE, status);
    if (*status == U_BUFFER_OVERFLOW_ERROR) {
        outBuf = static_cast<UChar *>(uprv_malloc((skelLengthInUChars+1)*sizeof(UChar)));
        if (outBuf == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        *status = U_ZERO_ERROR;
        skelLengthInUChars = uspoof_getSkeleton(sc, type, inBuf, lengthInUChars,
                                         outBuf, skelLengthInUChars+1, status);
    }

    u_strToUTF8(dest, destCapacity, &skelLengthInUTF8,
                outBuf, skelLengthInUChars, status);

  cleanup:
    if (inBuf != smallInBuf) {
        uprv_free(inBuf);
    }
    if (outBuf != smallOutBuf) {
        uprv_free(outBuf);
    }
    return skelLengthInUTF8;
}


U_CAPI int32_t U_EXPORT2
uspoof_serialize(USpoofChecker *sc,void *buf, int32_t capacity, UErrorCode *status) {
    SpoofImpl *This = SpoofImpl::validateThis(sc, *status);
    if (This == NULL) {
        U_ASSERT(U_FAILURE(*status));
        return 0;
    }
    int32_t dataSize = This->fSpoofData->fRawData->fLength;
    if (capacity < dataSize) {
        *status = U_BUFFER_OVERFLOW_ERROR;
        return dataSize;
    }
    uprv_memcpy(buf, This->fSpoofData->fRawData, dataSize);
    return dataSize;
}

#endif
