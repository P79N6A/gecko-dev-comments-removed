







#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/regex.h"
#include "unicode/uregex.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/uobject.h"
#include "unicode/utf16.h"
#include "umutex.h"
#include "uassert.h"
#include "cmemory.h"

#include "regextxt.h"

#include <stdio.h>

U_NAMESPACE_BEGIN

#define REMAINING_CAPACITY(idx,len) ((((len)-(idx))>0)?((len)-(idx)):0)

struct RegularExpression: public UMemory {
public:
    RegularExpression();
    ~RegularExpression();
    int32_t           fMagic;
    RegexPattern     *fPat;
    int32_t          *fPatRefCount;
    UChar            *fPatString;
    int32_t           fPatStringLen;
    RegexMatcher     *fMatcher;
    const UChar      *fText;         
    int32_t           fTextLength;   
                                     
    UBool             fOwnsText;
};

static const int32_t REXP_MAGIC = 0x72657870; 

RegularExpression::RegularExpression() {
    fMagic        = REXP_MAGIC;
    fPat          = NULL;
    fPatRefCount  = NULL;
    fPatString    = NULL;
    fPatStringLen = 0;
    fMatcher      = NULL;
    fText         = NULL;
    fTextLength   = 0;
    fOwnsText     = FALSE;
}

RegularExpression::~RegularExpression() {
    delete fMatcher;
    fMatcher = NULL;
    if (fPatRefCount!=NULL && umtx_atomic_dec(fPatRefCount)==0) {
        delete fPat;
        uprv_free(fPatString);
        uprv_free(fPatRefCount);
    }
    if (fOwnsText && fText!=NULL) {
        uprv_free((void *)fText);
    }
    fMagic = 0;
}

U_NAMESPACE_END

U_NAMESPACE_USE






static UBool validateRE(const RegularExpression *re, UBool requiresText, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return FALSE;
    }
    if (re == NULL || re->fMagic != REXP_MAGIC) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return FALSE;
    }
    
    if (requiresText && re->fText == NULL && !re->fOwnsText) {
        *status = U_REGEX_INVALID_STATE;
        return FALSE;
    }
    return TRUE;
}






U_CAPI URegularExpression *  U_EXPORT2
uregex_open( const  UChar          *pattern,
                    int32_t         patternLength,
                    uint32_t        flags,
                    UParseError    *pe,
                    UErrorCode     *status) {

    if (U_FAILURE(*status)) {
        return NULL;
    }
    if (pattern == NULL || patternLength < -1 || patternLength == 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    int32_t actualPatLen = patternLength;
    if (actualPatLen == -1) {
        actualPatLen = u_strlen(pattern);
    }

    RegularExpression *re     = new RegularExpression;
    int32_t            *refC   = (int32_t *)uprv_malloc(sizeof(int32_t));
    UChar              *patBuf = (UChar *)uprv_malloc(sizeof(UChar)*(actualPatLen+1));
    if (re == NULL || refC == NULL || patBuf == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        delete re;
        uprv_free(refC);
        uprv_free(patBuf);
        return NULL;
    }
    re->fPatRefCount = refC;
    *re->fPatRefCount = 1;

    
    
    
    
    
    re->fPatString    = patBuf;
    re->fPatStringLen = patternLength;
    u_memcpy(patBuf, pattern, actualPatLen);
    patBuf[actualPatLen] = 0;
    
    UText patText = UTEXT_INITIALIZER;
    utext_openUChars(&patText, patBuf, patternLength, status);

    
    
    
    if (pe != NULL) {
        re->fPat = RegexPattern::compile(&patText, flags, *pe, *status);
    } else {
        re->fPat = RegexPattern::compile(&patText, flags, *status);
    }
    utext_close(&patText);
    
    if (U_FAILURE(*status)) {
        goto ErrorExit;
    }

    
    
    
    re->fMatcher = re->fPat->matcher(*status);
    if (U_SUCCESS(*status)) {
        return (URegularExpression*)re;
    }

ErrorExit:
    delete re;
    return NULL;

}






U_CAPI URegularExpression *  U_EXPORT2
uregex_openUText(UText          *pattern,
                 uint32_t        flags,
                 UParseError    *pe,
                 UErrorCode     *status) {
    
    if (U_FAILURE(*status)) {
        return NULL;
    }
    if (pattern == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    
    int64_t patternNativeLength = utext_nativeLength(pattern);
    
    if (patternNativeLength == 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    
    RegularExpression *re     = new RegularExpression;
    
    UErrorCode lengthStatus = U_ZERO_ERROR;
    int32_t pattern16Length = utext_extract(pattern, 0, patternNativeLength, NULL, 0, &lengthStatus);
    
    int32_t            *refC   = (int32_t *)uprv_malloc(sizeof(int32_t));
    UChar              *patBuf = (UChar *)uprv_malloc(sizeof(UChar)*(pattern16Length+1));
    if (re == NULL || refC == NULL || patBuf == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        delete re;
        uprv_free(refC);
        uprv_free(patBuf);
        return NULL;
    }
    re->fPatRefCount = refC;
    *re->fPatRefCount = 1;
    
    
    
    
    
    
    re->fPatString    = patBuf;
    re->fPatStringLen = pattern16Length;
    utext_extract(pattern, 0, patternNativeLength, patBuf, pattern16Length+1, status);
    
    UText patText = UTEXT_INITIALIZER;
    utext_openUChars(&patText, patBuf, pattern16Length, status);
    
    
    
    
    if (pe != NULL) {
        re->fPat = RegexPattern::compile(&patText, flags, *pe, *status);
    } else {
        re->fPat = RegexPattern::compile(&patText, flags, *status);
    }
    utext_close(&patText);
    
    if (U_FAILURE(*status)) {
        goto ErrorExit;
    }
    
    
    
    
    re->fMatcher = re->fPat->matcher(*status);
    if (U_SUCCESS(*status)) {
        return (URegularExpression*)re;
    }
    
ErrorExit:
    delete re;
    return NULL;
    
}






U_CAPI void  U_EXPORT2
uregex_close(URegularExpression  *re2) {
    RegularExpression *re = (RegularExpression*)re2;
    UErrorCode  status = U_ZERO_ERROR;
    if (validateRE(re, FALSE, &status) == FALSE) {
        return;
    }
    delete re;
}







U_CAPI URegularExpression * U_EXPORT2 
uregex_clone(const URegularExpression *source2, UErrorCode *status)  {
    RegularExpression *source = (RegularExpression*)source2;
    if (validateRE(source, FALSE, status) == FALSE) {
        return NULL;
    }

    RegularExpression *clone = new RegularExpression;
    if (clone == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    clone->fMatcher = source->fPat->matcher(*status);
    if (U_FAILURE(*status)) {
        delete clone;
        return NULL;
    }

    clone->fPat          = source->fPat;
    clone->fPatRefCount  = source->fPatRefCount; 
    clone->fPatString    = source->fPatString;
    clone->fPatStringLen = source->fPatStringLen;
    umtx_atomic_inc(source->fPatRefCount);
    

    return (URegularExpression*)clone;
}









U_CAPI const UChar * U_EXPORT2 
uregex_pattern(const  URegularExpression *regexp2,
                      int32_t            *patLength,
                      UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return NULL;
    }
    if (patLength != NULL) {
        *patLength = regexp->fPatStringLen;
    }
    return regexp->fPatString;
}







U_CAPI UText * U_EXPORT2
uregex_patternUText(const URegularExpression *regexp2,
                          UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    return regexp->fPat->patternText(*status);
}







U_CAPI int32_t U_EXPORT2 
uregex_flags(const URegularExpression *regexp2, UErrorCode *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return 0;
    }
    int32_t flags = regexp->fPat->flags();
    return flags;
}







U_CAPI void U_EXPORT2 
uregex_setText(URegularExpression *regexp2,
               const UChar        *text,
               int32_t             textLength,
               UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return;
    }
    if (text == NULL || textLength < -1) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    if (regexp->fOwnsText && regexp->fText != NULL) {
        uprv_free((void *)regexp->fText);
    }
    
    regexp->fText       = text;
    regexp->fTextLength = textLength;
    regexp->fOwnsText   = FALSE;
    
    UText input = UTEXT_INITIALIZER;
    utext_openUChars(&input, text, textLength, status);
    regexp->fMatcher->reset(&input);
    utext_close(&input); 
}







U_CAPI void U_EXPORT2 
uregex_setUText(URegularExpression *regexp2,
                UText              *text,
                UErrorCode         *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return;
    }
    if (text == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    if (regexp->fOwnsText && regexp->fText != NULL) {
        uprv_free((void *)regexp->fText);
    }
    
    regexp->fText       = NULL; 
    regexp->fTextLength = -1;
    regexp->fOwnsText   = TRUE;
    regexp->fMatcher->reset(text);
}








U_CAPI const UChar * U_EXPORT2 
uregex_getText(URegularExpression *regexp2,
               int32_t            *textLength,
               UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return NULL;
    }
    
    if (regexp->fText == NULL) {
        
        UText *inputText = regexp->fMatcher->inputText();
        int64_t inputNativeLength = utext_nativeLength(inputText);
        if (UTEXT_FULL_TEXT_IN_CHUNK(inputText, inputNativeLength)) {
            regexp->fText = inputText->chunkContents;
            regexp->fTextLength = (int32_t)inputNativeLength;
            regexp->fOwnsText = FALSE; 
        } else {
            UErrorCode lengthStatus = U_ZERO_ERROR;
            regexp->fTextLength = utext_extract(inputText, 0, inputNativeLength, NULL, 0, &lengthStatus); 
            UChar *inputChars = (UChar *)uprv_malloc(sizeof(UChar)*(regexp->fTextLength+1));
            
            utext_extract(inputText, 0, inputNativeLength, inputChars, regexp->fTextLength+1, status);
            regexp->fText = inputChars;
            regexp->fOwnsText = TRUE; 
        }
    }
    
    if (textLength != NULL) {
        *textLength = regexp->fTextLength;
    }
    return regexp->fText;
}







U_CAPI UText * U_EXPORT2 
uregex_getUText(URegularExpression *regexp2,
                UText              *dest,
                UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return dest;
    }
    return regexp->fMatcher->getInput(dest, *status);
}







U_CAPI void U_EXPORT2 
uregex_refreshUText(URegularExpression *regexp2,
                    UText              *text,
                    UErrorCode         *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return;
    }
    regexp->fMatcher->refreshInputText(text, *status);
}







U_CAPI UBool U_EXPORT2 
uregex_matches(URegularExpression *regexp2,
               int32_t            startIndex,
               UErrorCode        *status)  {
    return uregex_matches64( regexp2, (int64_t)startIndex, status);
}

U_CAPI UBool U_EXPORT2 
uregex_matches64(URegularExpression *regexp2,
                 int64_t            startIndex,
                 UErrorCode        *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    UBool result = FALSE;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return result;
    }
    if (startIndex == -1) {
        result = regexp->fMatcher->matches(*status);
    } else {
        result = regexp->fMatcher->matches(startIndex, *status);
    }
    return result;
}







U_CAPI UBool U_EXPORT2 
uregex_lookingAt(URegularExpression *regexp2,
                 int32_t             startIndex,
                 UErrorCode         *status)  {
    return uregex_lookingAt64( regexp2, (int64_t)startIndex, status);
}

U_CAPI UBool U_EXPORT2 
uregex_lookingAt64(URegularExpression *regexp2,
                   int64_t             startIndex,
                   UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    UBool result = FALSE;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return result;
    }
    if (startIndex == -1) {
        result = regexp->fMatcher->lookingAt(*status);
    } else {
        result = regexp->fMatcher->lookingAt(startIndex, *status);
    }
    return result;
}








U_CAPI UBool U_EXPORT2 
uregex_find(URegularExpression *regexp2,
            int32_t             startIndex, 
            UErrorCode         *status)  {
    return uregex_find64( regexp2, (int64_t)startIndex, status);
}

U_CAPI UBool U_EXPORT2 
uregex_find64(URegularExpression *regexp2,
              int64_t             startIndex, 
              UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    UBool result = FALSE;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return result;
    }
    if (startIndex == -1) {
        regexp->fMatcher->resetPreserveRegion();
        result = regexp->fMatcher->find();
    } else {
        result = regexp->fMatcher->find(startIndex, *status);
    }
    return result;
}







U_CAPI UBool U_EXPORT2 
uregex_findNext(URegularExpression *regexp2,
                UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return FALSE;
    }
    UBool result = regexp->fMatcher->find();
    return result;
}






U_CAPI int32_t U_EXPORT2 
uregex_groupCount(URegularExpression *regexp2,
                  UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return 0;
    }
    int32_t  result = regexp->fMatcher->groupCount();
    return result;
}







U_CAPI int32_t U_EXPORT2 
uregex_group(URegularExpression *regexp2,
             int32_t             groupNum,
             UChar              *dest,
             int32_t             destCapacity,
             UErrorCode          *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    if (destCapacity < 0 || (destCapacity > 0 && dest == NULL)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    if (destCapacity == 0 || regexp->fText != NULL) {
        
        
        
        
        
        
        int32_t  startIx = regexp->fMatcher->start(groupNum, *status);
        int32_t  endIx   = regexp->fMatcher->end  (groupNum, *status);
        if (U_FAILURE(*status)) {
            return 0;
        }

        
        
        
        int32_t fullLength = endIx - startIx;
        int32_t copyLength = fullLength;
        if (copyLength < destCapacity) {
            dest[copyLength] = 0;
        } else if (copyLength == destCapacity) {
            *status = U_STRING_NOT_TERMINATED_WARNING;
        } else {
            copyLength = destCapacity;
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
        
        
        
        
        if (copyLength > 0) {
            u_memcpy(dest, &regexp->fText[startIx], copyLength);
        }
        return fullLength;
    } else {
        UText *groupText = uregex_groupUTextDeep(regexp2, groupNum, NULL, status);
        int32_t result = utext_extract(groupText, 0, utext_nativeLength(groupText), dest, destCapacity, status);
        utext_close(groupText);
        return result;
    }
}







U_CAPI UText * U_EXPORT2 
uregex_groupUText(URegularExpression *regexp2,
                  int32_t             groupNum,
                  UText              *dest,
                  int64_t            *groupLength,
                  UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        UErrorCode emptyTextStatus = U_ZERO_ERROR;
        return (dest ? dest : utext_openUChars(NULL, NULL, 0, &emptyTextStatus));
    }

    return regexp->fMatcher->group(groupNum, dest, *groupLength, *status);
}






U_CAPI UText * U_EXPORT2 
uregex_groupUTextDeep(URegularExpression *regexp2,
                  int32_t             groupNum,
                  UText              *dest,
                  UErrorCode         *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        UErrorCode emptyTextStatus = U_ZERO_ERROR;
        return (dest ? dest : utext_openUChars(NULL, NULL, 0, &emptyTextStatus));
    }

    if (regexp->fText != NULL) {
        
        
        
        
        int32_t  startIx = regexp->fMatcher->start(groupNum, *status);
        int32_t  endIx   = regexp->fMatcher->end  (groupNum, *status);
        if (U_FAILURE(*status)) {
            UErrorCode emptyTextStatus = U_ZERO_ERROR;
            return (dest ? dest : utext_openUChars(NULL, NULL, 0, &emptyTextStatus));
        }
        
        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), &regexp->fText[startIx], endIx - startIx, status);
        } else {
            UText groupText = UTEXT_INITIALIZER;
            utext_openUChars(&groupText, &regexp->fText[startIx], endIx - startIx, status);
            dest = utext_clone(NULL, &groupText, TRUE, FALSE, status);
            utext_close(&groupText);
        }
        
        return dest;
    } else {
        return regexp->fMatcher->group(groupNum, dest, *status);
    }
}






U_CAPI int32_t U_EXPORT2 
uregex_start(URegularExpression *regexp2,
             int32_t             groupNum,
             UErrorCode          *status)  {
    return (int32_t)uregex_start64( regexp2, groupNum, status);
}

U_CAPI int64_t U_EXPORT2 
uregex_start64(URegularExpression *regexp2,
               int32_t             groupNum,
               UErrorCode          *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    int32_t result = regexp->fMatcher->start(groupNum, *status);
    return result;
}






U_CAPI int32_t U_EXPORT2 
uregex_end(URegularExpression   *regexp2,
           int32_t               groupNum,
           UErrorCode           *status)  {
    return (int32_t)uregex_end64( regexp2, groupNum, status);
}

U_CAPI int64_t U_EXPORT2 
uregex_end64(URegularExpression   *regexp2,
             int32_t               groupNum,
             UErrorCode           *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    int32_t result = regexp->fMatcher->end(groupNum, *status);
    return result;
}






U_CAPI void U_EXPORT2 
uregex_reset(URegularExpression    *regexp2,
             int32_t               index,
             UErrorCode            *status)  {
    uregex_reset64( regexp2, (int64_t)index, status);
}

U_CAPI void U_EXPORT2 
uregex_reset64(URegularExpression    *regexp2,
               int64_t               index,
               UErrorCode            *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return;
    }
    regexp->fMatcher->reset(index, *status);
}







U_CAPI void U_EXPORT2 
uregex_setRegion(URegularExpression   *regexp2,
                 int32_t               regionStart,
                 int32_t               regionLimit,
                 UErrorCode           *status)  {
    uregex_setRegion64( regexp2, (int64_t)regionStart, (int64_t)regionLimit, status);
}

U_CAPI void U_EXPORT2 
uregex_setRegion64(URegularExpression   *regexp2,
                   int64_t               regionStart,
                   int64_t               regionLimit,
                   UErrorCode           *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return;
    }
    regexp->fMatcher->region(regionStart, regionLimit, *status);
}







U_CAPI void U_EXPORT2 
uregex_setRegionAndStart(URegularExpression   *regexp2,
                 int64_t               regionStart,
                 int64_t               regionLimit,
                 int64_t               startIndex,
                 UErrorCode           *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return;
    }
    regexp->fMatcher->region(regionStart, regionLimit, startIndex, *status);
}






U_CAPI int32_t U_EXPORT2 
uregex_regionStart(const  URegularExpression   *regexp2,
                          UErrorCode           *status)  {
    return (int32_t)uregex_regionStart64(regexp2, status);
}

U_CAPI int64_t U_EXPORT2 
uregex_regionStart64(const  URegularExpression   *regexp2,
                            UErrorCode           *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    return regexp->fMatcher->regionStart();
}







U_CAPI int32_t U_EXPORT2 
uregex_regionEnd(const  URegularExpression   *regexp2,
                        UErrorCode           *status)  {
    return (int32_t)uregex_regionEnd64(regexp2, status);
}

U_CAPI int64_t U_EXPORT2 
uregex_regionEnd64(const  URegularExpression   *regexp2,
                          UErrorCode           *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    return regexp->fMatcher->regionEnd();
}







U_CAPI UBool U_EXPORT2 
uregex_hasTransparentBounds(const  URegularExpression   *regexp2,
                                   UErrorCode           *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return FALSE;
    }
    return regexp->fMatcher->hasTransparentBounds();
}







U_CAPI void U_EXPORT2 
uregex_useTransparentBounds(URegularExpression    *regexp2,
                            UBool                  b,
                            UErrorCode            *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return;
    }
    regexp->fMatcher->useTransparentBounds(b);
}







U_CAPI UBool U_EXPORT2 
uregex_hasAnchoringBounds(const  URegularExpression   *regexp2,
                                 UErrorCode           *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return FALSE;
    }
    return regexp->fMatcher->hasAnchoringBounds();
}







U_CAPI void U_EXPORT2 
uregex_useAnchoringBounds(URegularExpression    *regexp2,
                          UBool                  b,
                          UErrorCode            *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status) == FALSE) {
        return;
    }
    regexp->fMatcher->useAnchoringBounds(b);
}







U_CAPI UBool U_EXPORT2 
uregex_hitEnd(const  URegularExpression   *regexp2,
                     UErrorCode           *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return FALSE;
    }
    return regexp->fMatcher->hitEnd();
}







U_CAPI UBool U_EXPORT2 
uregex_requireEnd(const  URegularExpression   *regexp2,
                         UErrorCode           *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return FALSE;
    }
    return regexp->fMatcher->requireEnd();
}







U_CAPI void U_EXPORT2 
uregex_setTimeLimit(URegularExpression   *regexp2,
                    int32_t               limit,
                    UErrorCode           *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status)) {
        regexp->fMatcher->setTimeLimit(limit, *status);
    }
}








U_CAPI int32_t U_EXPORT2 
uregex_getTimeLimit(const  URegularExpression   *regexp2,
                           UErrorCode           *status) {
    int32_t retVal = 0;
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status)) {
        retVal = regexp->fMatcher->getTimeLimit();
    }
    return retVal;
}








U_CAPI void U_EXPORT2 
uregex_setStackLimit(URegularExpression   *regexp2,
                     int32_t               limit,
                     UErrorCode           *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status)) {
        regexp->fMatcher->setStackLimit(limit, *status);
    }
}








U_CAPI int32_t U_EXPORT2 
uregex_getStackLimit(const  URegularExpression   *regexp2,
                            UErrorCode           *status) {
    int32_t retVal = 0;
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status)) {
        retVal = regexp->fMatcher->getStackLimit();
    }
    return retVal;
}







U_CAPI void U_EXPORT2
uregex_setMatchCallback(URegularExpression      *regexp2,
                        URegexMatchCallback     *callback,
                        const void              *context,
                        UErrorCode              *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status)) {
        regexp->fMatcher->setMatchCallback(callback, context, *status);
    }
}







U_CAPI void U_EXPORT2 
uregex_getMatchCallback(const URegularExpression    *regexp2,
                        URegexMatchCallback        **callback,
                        const void                 **context,
                        UErrorCode                  *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
     if (validateRE(regexp, FALSE, status)) {
         regexp->fMatcher->getMatchCallback(*callback, *context, *status);
     }
}







U_CAPI void U_EXPORT2
uregex_setFindProgressCallback(URegularExpression              *regexp2,
                                URegexFindProgressCallback      *callback,
                                const void                      *context,
                                UErrorCode                      *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, FALSE, status)) {
        regexp->fMatcher->setFindProgressCallback(callback, context, *status);
    }
}







U_CAPI void U_EXPORT2 
uregex_getFindProgressCallback(const URegularExpression          *regexp2,
                                URegexFindProgressCallback        **callback,
                                const void                        **context,
                                UErrorCode                        *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
     if (validateRE(regexp, FALSE, status)) {
         regexp->fMatcher->getFindProgressCallback(*callback, *context, *status);
     }
}







U_CAPI int32_t U_EXPORT2 
uregex_replaceAll(URegularExpression    *regexp2,
                  const UChar           *replacementText,
                  int32_t                replacementLength,
                  UChar                 *destBuf,
                  int32_t                destCapacity,
                  UErrorCode            *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    if (replacementText == NULL || replacementLength < -1 ||
        (destBuf == NULL && destCapacity > 0) ||
        destCapacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t   len = 0;

    uregex_reset(regexp2, 0, status);

    
    
    
    
    
    UErrorCode  findStatus = *status;
    while (uregex_findNext(regexp2, &findStatus)) {
        len += uregex_appendReplacement(regexp2, replacementText, replacementLength,
                                        &destBuf, &destCapacity, status);
    }
    len += uregex_appendTail(regexp2, &destBuf, &destCapacity, status);
    
    if (U_FAILURE(findStatus)) {
        
        
        
        *status = findStatus;
    }

    return len;
}







U_CAPI UText * U_EXPORT2 
uregex_replaceAllUText(URegularExpression    *regexp2,
                       UText                 *replacementText,
                       UText                 *dest,
                       UErrorCode            *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    if (replacementText == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    dest = regexp->fMatcher->replaceAll(replacementText, dest, *status);
    return dest;
}
    






U_CAPI int32_t U_EXPORT2 
uregex_replaceFirst(URegularExpression  *regexp2,
                    const UChar         *replacementText,
                    int32_t              replacementLength,
                    UChar               *destBuf,
                    int32_t              destCapacity,
                    UErrorCode          *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    if (replacementText == NULL || replacementLength < -1 ||
        (destBuf == NULL && destCapacity > 0) ||
        destCapacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t   len = 0;
    UBool     findSucceeded;
    uregex_reset(regexp2, 0, status);
    findSucceeded = uregex_find(regexp2, 0, status);
    if (findSucceeded) {
        len = uregex_appendReplacement(regexp2, replacementText, replacementLength, 
                                       &destBuf, &destCapacity, status);
    }
    len += uregex_appendTail(regexp2, &destBuf, &destCapacity, status);

    return len;
}







U_CAPI UText * U_EXPORT2 
uregex_replaceFirstUText(URegularExpression  *regexp2,
                         UText                 *replacementText,
                         UText                 *dest,
                         UErrorCode            *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    if (replacementText == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    dest = regexp->fMatcher->replaceFirst(replacementText, dest, *status);
    return dest;
}








U_NAMESPACE_BEGIN




class RegexCImpl {
 public:
   inline static  int32_t appendReplacement(RegularExpression    *regexp,
                      const UChar           *replacementText,
                      int32_t                replacementLength,
                      UChar                **destBuf,
                      int32_t               *destCapacity,
                      UErrorCode            *status);

   inline static int32_t appendTail(RegularExpression    *regexp,
        UChar                **destBuf,
        int32_t               *destCapacity,
        UErrorCode            *status);
                  
    inline static int32_t split(RegularExpression    *regexp,
        UChar                 *destBuf,
        int32_t                destCapacity,
        int32_t               *requiredCapacity,
        UChar                 *destFields[],
        int32_t                destFieldsCapacity,
        UErrorCode            *status);
};

U_NAMESPACE_END



static const UChar BACKSLASH  = 0x5c;
static const UChar DOLLARSIGN = 0x24;






static inline void appendToBuf(UChar c, int32_t *idx, UChar *buf, int32_t bufCapacity) {
    if (*idx < bufCapacity) {
        buf[*idx] = c;
    }
    (*idx)++;
}





int32_t RegexCImpl::appendReplacement(RegularExpression    *regexp,
                                      const UChar           *replacementText,
                                      int32_t                replacementLength,
                                      UChar                **destBuf,
                                      int32_t               *destCapacity,
                                      UErrorCode            *status)  {

    
    
    
    UBool pendingBufferOverflow = FALSE;
    if (*status == U_BUFFER_OVERFLOW_ERROR && destCapacity != NULL && *destCapacity == 0) {
        pendingBufferOverflow = TRUE;
        *status = U_ZERO_ERROR;
    }

    
    
    
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    if (replacementText == NULL || replacementLength < -1 ||
        destCapacity == NULL || destBuf == NULL || 
        (*destBuf == NULL && *destCapacity > 0) ||
        *destCapacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    RegexMatcher *m = regexp->fMatcher;
    if (m->fMatch == FALSE) {
        *status = U_REGEX_INVALID_STATE;
        return 0;
    }

    UChar    *dest             = *destBuf;
    int32_t   capacity         = *destCapacity;
    int32_t   destIdx          =  0;
    int32_t   i;
    
    
    
    
    if (replacementLength == -1) {
        replacementLength = u_strlen(replacementText);
    }

    
    if (regexp->fText != NULL) {
        int32_t matchStart;
        int32_t lastMatchEnd;
        if (UTEXT_USES_U16(m->fInputText)) {
            lastMatchEnd = (int32_t)m->fLastMatchEnd;
            matchStart = (int32_t)m->fMatchStart;
        } else {
            
            UErrorCode status = U_ZERO_ERROR;
            lastMatchEnd = utext_extract(m->fInputText, 0, m->fLastMatchEnd, NULL, 0, &status);
            status = U_ZERO_ERROR;
            matchStart = lastMatchEnd + utext_extract(m->fInputText, m->fLastMatchEnd, m->fMatchStart, NULL, 0, &status);
        }
        for (i=lastMatchEnd; i<matchStart; i++) {
            appendToBuf(regexp->fText[i], &destIdx, dest, capacity);
        }        
    } else {
        UErrorCode possibleOverflowError = U_ZERO_ERROR; 
        destIdx += utext_extract(m->fInputText, m->fLastMatchEnd, m->fMatchStart,
                                 dest==NULL?NULL:&dest[destIdx], REMAINING_CAPACITY(destIdx, capacity),
                                 &possibleOverflowError);
    }
    U_ASSERT(destIdx >= 0);

    
    int32_t  replIdx = 0;
    while (replIdx < replacementLength) {
        UChar  c = replacementText[replIdx];
        replIdx++;
        if (c != DOLLARSIGN && c != BACKSLASH) {
            
            
            appendToBuf(c, &destIdx, dest, capacity);
            continue;
        }

        if (c == BACKSLASH) {
            
            
            
            
            
            if (replIdx >= replacementLength) {
                break;
            }
            c = replacementText[replIdx];

            if (c==0x55 || c==0x75) {
                
                UChar32 escapedChar = 
                    u_unescapeAt(uregex_ucstr_unescape_charAt,
                       &replIdx,                   
                       replacementLength,          
                       (void *)replacementText);

                if (escapedChar != (UChar32)0xFFFFFFFF) {
                    if (escapedChar <= 0xffff) {
                        appendToBuf((UChar)escapedChar, &destIdx, dest, capacity);
                    } else {
                        appendToBuf(U16_LEAD(escapedChar), &destIdx, dest, capacity);
                        appendToBuf(U16_TRAIL(escapedChar), &destIdx, dest, capacity);
                    }
                    continue;
                }
                
                
            }

            
            appendToBuf(c, &destIdx, dest, capacity);

            replIdx++;
            continue;
        }



        
        
        

        int32_t numDigits = 0;
        int32_t groupNum  = 0;
        UChar32 digitC;
        for (;;) {
            if (replIdx >= replacementLength) {
                break;
            }
            U16_GET(replacementText, 0, replIdx, replacementLength, digitC);
            if (u_isdigit(digitC) == FALSE) {
                break;
            }

            U16_FWD_1(replacementText, replIdx, replacementLength);
            groupNum=groupNum*10 + u_charDigitValue(digitC);
            numDigits++;
            if (numDigits >= m->fPattern->fMaxCaptureDigits) {
                break;
            }
        }


        if (numDigits == 0) {
            
            
            appendToBuf(DOLLARSIGN, &destIdx, dest, capacity);
            continue;
        }

        
        destIdx += uregex_group((URegularExpression*)regexp, groupNum,
                                dest==NULL?NULL:&dest[destIdx], REMAINING_CAPACITY(destIdx, capacity), status);
        if (*status == U_BUFFER_OVERFLOW_ERROR) {
            
            
            
            *status = U_ZERO_ERROR;
        }

        if (U_FAILURE(*status)) {
            
            break;
        }

    }

    
    
    
    
    if (destIdx < capacity) {
        dest[destIdx] = 0;
    } else if (destIdx == *destCapacity) {
        *status = U_STRING_NOT_TERMINATED_WARNING;
    } else {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }
    
    
    
    
    if (destIdx > 0 &&  *destCapacity > 0) {
        if (destIdx < capacity) {
            *destBuf      += destIdx;
            *destCapacity -= destIdx;
        } else {
            *destBuf      += capacity;
            *destCapacity =  0;
        }
    }

    
    
    
    if (pendingBufferOverflow && U_SUCCESS(*status)) {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }

    return destIdx;
}




U_CAPI int32_t U_EXPORT2 
uregex_appendReplacement(URegularExpression    *regexp2,
                         const UChar           *replacementText,
                         int32_t                replacementLength,
                         UChar                **destBuf,
                         int32_t               *destCapacity,
                         UErrorCode            *status) {
    
    RegularExpression *regexp = (RegularExpression*)regexp2;
    return RegexCImpl::appendReplacement(
        regexp, replacementText, replacementLength,destBuf, destCapacity, status);
}




U_CAPI void U_EXPORT2 
uregex_appendReplacementUText(URegularExpression    *regexp2,
                              UText                 *replText,
                              UText                 *dest,
                              UErrorCode            *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    regexp->fMatcher->appendReplacement(dest, replText, *status);
}







int32_t RegexCImpl::appendTail(RegularExpression    *regexp,
                               UChar                **destBuf,
                               int32_t               *destCapacity,
                               UErrorCode            *status)
{

    
    
    
    UBool pendingBufferOverflow = FALSE;
    if (*status == U_BUFFER_OVERFLOW_ERROR && destCapacity != NULL && *destCapacity == 0) {
        pendingBufferOverflow = TRUE;
        *status = U_ZERO_ERROR;
    }

    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    
    if (destCapacity == NULL || destBuf == NULL || 
        (*destBuf == NULL && *destCapacity > 0) ||
        *destCapacity < 0)
    {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    RegexMatcher *m = regexp->fMatcher;

    int32_t  destIdx     = 0;
    int32_t  destCap     = *destCapacity;
    UChar    *dest       = *destBuf;
    
    if (regexp->fText != NULL) {
        int32_t srcIdx;
        int64_t nativeIdx = (m->fMatch ? m->fMatchEnd : m->fLastMatchEnd);
        if (nativeIdx == -1) {
            srcIdx = 0;
        } else if (UTEXT_USES_U16(m->fInputText)) {
            srcIdx = (int32_t)nativeIdx;
        } else {
            UErrorCode status = U_ZERO_ERROR;
            srcIdx = utext_extract(m->fInputText, 0, nativeIdx, NULL, 0, &status);
        }
            
        for (;;) {
            U_ASSERT(destIdx >= 0);

            if (srcIdx == regexp->fTextLength) {
                break;
            }
            UChar c = regexp->fText[srcIdx];
            if (c == 0 && regexp->fTextLength == -1) {
                regexp->fTextLength = srcIdx;
                break;
            }

            if (destIdx < destCap) {
                dest[destIdx] = c;
            } else {
                
                
                
                if (regexp->fTextLength > 0) {
                    destIdx += (regexp->fTextLength - srcIdx);
                    break;
                }
            }
            srcIdx++;
            destIdx++;
        }            
    } else {
        int64_t  srcIdx;
        if (m->fMatch) {
            
            srcIdx = m->fMatchEnd;
        } else {
            
            
            srcIdx = m->fLastMatchEnd;
            if (srcIdx == -1)  {
                
                
                srcIdx = 0;
            }
        }

        destIdx = utext_extract(m->fInputText, srcIdx, m->fInputLength, dest, destCap, status);
    }

    
    
    
    
    if (destIdx < destCap) {
        dest[destIdx] = 0;
    } else  if (destIdx == destCap) {
        *status = U_STRING_NOT_TERMINATED_WARNING;
    } else {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }

    
    
    
    
    if (destIdx < destCap) {
        *destBuf      += destIdx;
        *destCapacity -= destIdx;
    } else if (*destBuf != NULL) {
        *destBuf      += destCap;
        *destCapacity  = 0;
    }

    if (pendingBufferOverflow && U_SUCCESS(*status)) {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }

    return destIdx;
}





U_CAPI int32_t U_EXPORT2 
uregex_appendTail(URegularExpression    *regexp2,
                  UChar                **destBuf,
                  int32_t               *destCapacity,
                  UErrorCode            *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    return RegexCImpl::appendTail(regexp, destBuf, destCapacity, status);
}





U_CAPI UText * U_EXPORT2 
uregex_appendTailUText(URegularExpression    *regexp2,
                       UText                 *dest,
                       UErrorCode            *status)  {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    return regexp->fMatcher->appendTail(dest, *status);
}










#if 0
static void copyString(UChar        *destBuffer,    
                       int32_t       destCapacity,  
                       int32_t      *destIndex,     
                                                    
                       const UChar  *srcPtr,        
                       int32_t       srcLen)        
{
    int32_t  si;
    int32_t  di = *destIndex;
    UChar    c;

    for (si=0; si<srcLen;  si++) {
        c = srcPtr[si];
        if (di < destCapacity) {
            destBuffer[di] = c;
            di++;
        } else {
            di += srcLen - si;
            break;
        }
    }
    if (di<destCapacity) {
        destBuffer[di] = 0;
    }
    di++;
    *destIndex = di;
}
#endif






int32_t RegexCImpl::split(RegularExpression     *regexp,
                          UChar                 *destBuf,
                          int32_t                destCapacity,
                          int32_t               *requiredCapacity,
                          UChar                 *destFields[],
                          int32_t                destFieldsCapacity,
                          UErrorCode            *status) {
    
    
    
    regexp->fMatcher->reset();
    UText *inputText = regexp->fMatcher->fInputText;
    int64_t   nextOutputStringStart = 0;
    int64_t   inputLen = regexp->fMatcher->fInputLength;
    if (inputLen == 0) {
        return 0;
    }

    
    
    
    int32_t   i;             
    int32_t   destIdx = 0;   
    int32_t   numCaptureGroups = regexp->fMatcher->groupCount();
    UErrorCode  tStatus = U_ZERO_ERROR;   
    for (i=0; ; i++) {
        if (i>=destFieldsCapacity-1) {
            
            
            
            
            
            
            if (inputLen > nextOutputStringStart) {
                if (i != destFieldsCapacity-1) {
                    
                    
                    i = destFieldsCapacity-1;
                    destIdx = (int32_t)(destFields[i] - destFields[0]);
                }
                
                destFields[i] = &destBuf[destIdx];
                destIdx += 1 + utext_extract(inputText, nextOutputStringStart, inputLen,
                                             &destBuf[destIdx], REMAINING_CAPACITY(destIdx, destCapacity), status);
            }
            break;
        }
        
        if (regexp->fMatcher->find()) {
            
            
            destFields[i] = &destBuf[destIdx];
            
            destIdx += 1 + utext_extract(inputText, nextOutputStringStart, regexp->fMatcher->fMatchStart,
                                         &destBuf[destIdx], REMAINING_CAPACITY(destIdx, destCapacity), &tStatus);
            if (tStatus == U_BUFFER_OVERFLOW_ERROR) {
                tStatus = U_ZERO_ERROR;
            } else {
                *status = tStatus;
            }
            nextOutputStringStart = regexp->fMatcher->fMatchEnd;
            
            
            
            int32_t groupNum;
            for (groupNum=1; groupNum<=numCaptureGroups; groupNum++) {
                
                if (i==destFieldsCapacity-1) {
                    break;
                }
                i++;
                
                
                destFields[i] = &destBuf[destIdx];
                tStatus = U_ZERO_ERROR;
                int32_t t = uregex_group((URegularExpression*)regexp, 
                                         groupNum, 
                                         destFields[i], 
                                         REMAINING_CAPACITY(destIdx, destCapacity), 
                                         &tStatus);
                destIdx += t + 1;    
                                     
                if (tStatus == U_BUFFER_OVERFLOW_ERROR) {
                    tStatus = U_ZERO_ERROR;
                } else {
                    *status = tStatus;
                }
            }

            if (nextOutputStringStart == inputLen) {
                
                
                if (destIdx < destCapacity) {
                    destBuf[destIdx] = 0;
                }
                if (i < destFieldsCapacity-1) {
                   ++i;
                }
                if (destIdx < destCapacity) {
                    destFields[i] = destBuf + destIdx;
                }
                ++destIdx;
                break;
            }

        }
        else
        {
            
            
            destFields[i] = &destBuf[destIdx];
            destIdx += 1 + utext_extract(inputText, nextOutputStringStart, inputLen,
                                         &destBuf[destIdx], REMAINING_CAPACITY(destIdx, destCapacity), status);
            break;
        }
    }

    
    int j;
    for (j=i+1; j<destFieldsCapacity; j++) {
        destFields[j] = NULL;
    }

    if (requiredCapacity != NULL) {
        *requiredCapacity = destIdx;
    }
    if (destIdx > destCapacity) {
        *status = U_BUFFER_OVERFLOW_ERROR;
    }
    return i+1;
}




U_CAPI int32_t U_EXPORT2 
uregex_split(URegularExpression      *regexp2,
             UChar                   *destBuf,
             int32_t                  destCapacity,
             int32_t                 *requiredCapacity,
             UChar                   *destFields[],
             int32_t                  destFieldsCapacity,
             UErrorCode              *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    if (validateRE(regexp, TRUE, status) == FALSE) {
        return 0;
    }
    if ((destBuf == NULL && destCapacity > 0) ||
        destCapacity < 0 ||
        destFields == NULL ||
        destFieldsCapacity < 1 ) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    return RegexCImpl::split(regexp, destBuf, destCapacity, requiredCapacity, destFields, destFieldsCapacity, status);
}
    




U_CAPI int32_t U_EXPORT2 
uregex_splitUText(URegularExpression    *regexp2,
                  UText                 *destFields[],
                  int32_t                destFieldsCapacity,
                  UErrorCode            *status) {
    RegularExpression *regexp = (RegularExpression*)regexp2;
    return regexp->fMatcher->split(regexp->fMatcher->inputText(), destFields, destFieldsCapacity, *status);
}


#endif   

