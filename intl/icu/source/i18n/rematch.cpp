












#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "unicode/regex.h"
#include "unicode/uniset.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/rbbi.h"
#include "unicode/utf.h"
#include "unicode/utf16.h"
#include "uassert.h"
#include "cmemory.h"
#include "uvector.h"
#include "uvectr32.h"
#include "uvectr64.h"
#include "regeximp.h"
#include "regexst.h"
#include "regextxt.h"
#include "ucase.h"








#define REGEXFINDPROGRESS_INTERRUPT(pos, status)     \
    (fFindProgressCallbackFn != NULL) && (ReportFindProgress(pos, status) == FALSE)













U_NAMESPACE_BEGIN







static const int32_t DEFAULT_BACKTRACK_STACK_CAPACITY = 8000000;





static const int32_t TIMER_INITIAL_VALUE = 10000;






RegexMatcher::RegexMatcher(const RegexPattern *pat)  { 
    fDeferredStatus = U_ZERO_ERROR;
    init(fDeferredStatus);
    if (U_FAILURE(fDeferredStatus)) {
        return;
    }
    if (pat==NULL) {
        fDeferredStatus = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    fPattern = pat;
    init2(RegexStaticSets::gStaticSets->fEmptyText, fDeferredStatus);
}



RegexMatcher::RegexMatcher(const UnicodeString &regexp, const UnicodeString &input,
                           uint32_t flags, UErrorCode &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }
    UParseError    pe;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
    fPattern           = fPatternOwned;
    
    UText inputText = UTEXT_INITIALIZER;
    utext_openConstUnicodeString(&inputText, &input, &status);
    init2(&inputText, status);
    utext_close(&inputText);

    fInputUniStrMaybeMutable = TRUE;    
}


RegexMatcher::RegexMatcher(UText *regexp, UText *input,
                           uint32_t flags, UErrorCode &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }
    UParseError    pe;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
    if (U_FAILURE(status)) {
        return;
    }

    fPattern           = fPatternOwned;
    init2(input, status);
}


RegexMatcher::RegexMatcher(const UnicodeString &regexp, 
                           uint32_t flags, UErrorCode &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }
    UParseError    pe;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
    if (U_FAILURE(status)) {
        return;
    }
    fPattern           = fPatternOwned;
    init2(RegexStaticSets::gStaticSets->fEmptyText, status);
}

RegexMatcher::RegexMatcher(UText *regexp, 
                           uint32_t flags, UErrorCode &status) {
    init(status);
    if (U_FAILURE(status)) {
        return;
    }
    UParseError    pe;
    fPatternOwned      = RegexPattern::compile(regexp, flags, pe, status);
        if (U_FAILURE(status)) {
        return;
    }

    fPattern           = fPatternOwned;
    init2(RegexStaticSets::gStaticSets->fEmptyText, status);
}




RegexMatcher::~RegexMatcher() {
    delete fStack;
    if (fData != fSmallData) {
        uprv_free(fData);
        fData = NULL;
    }
    if (fPatternOwned) {
        delete fPatternOwned;
        fPatternOwned = NULL;
        fPattern = NULL;
    }
    
    if (fInput) {
        delete fInput;
    }
    if (fInputText) {
        utext_close(fInputText);
    }
    if (fAltInputText) {
        utext_close(fAltInputText);
    }
    
    #if UCONFIG_NO_BREAK_ITERATION==0
    delete fWordBreakItr;
    #endif
}








void RegexMatcher::init(UErrorCode &status) {
    fPattern           = NULL;
    fPatternOwned      = NULL;
    fFrameSize         = 0;
    fRegionStart       = 0;
    fRegionLimit       = 0;
    fAnchorStart       = 0;
    fAnchorLimit       = 0;
    fLookStart         = 0;
    fLookLimit         = 0;
    fActiveStart       = 0;
    fActiveLimit       = 0;
    fTransparentBounds = FALSE;
    fAnchoringBounds   = TRUE;
    fMatch             = FALSE;
    fMatchStart        = 0;
    fMatchEnd          = 0;
    fLastMatchEnd      = -1;
    fAppendPosition    = 0;
    fHitEnd            = FALSE;
    fRequireEnd        = FALSE;
    fStack             = NULL;
    fFrame             = NULL;
    fTimeLimit         = 0;
    fTime              = 0;
    fTickCounter       = 0;
    fStackLimit        = DEFAULT_BACKTRACK_STACK_CAPACITY;
    fCallbackFn        = NULL;
    fCallbackContext   = NULL;
    fFindProgressCallbackFn      = NULL;
    fFindProgressCallbackContext = NULL;
    fTraceDebug        = FALSE;
    fDeferredStatus    = status;
    fData              = fSmallData;
    fWordBreakItr      = NULL;
    
    fStack             = NULL;
    fInputText         = NULL;
    fAltInputText      = NULL;
    fInput             = NULL;
    fInputLength       = 0;
    fInputUniStrMaybeMutable = FALSE;

    if (U_FAILURE(status)) {
        fDeferredStatus = status;
    }
}





void RegexMatcher::init2(UText *input, UErrorCode &status) {
    if (U_FAILURE(status)) {
        fDeferredStatus = status;
        return;
    }

    if (fPattern->fDataSize > (int32_t)(sizeof(fSmallData)/sizeof(fSmallData[0]))) {
        fData = (int64_t *)uprv_malloc(fPattern->fDataSize * sizeof(int64_t)); 
        if (fData == NULL) {
            status = fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    fStack = new UVector64(status);
    if (fStack == NULL) {
        status = fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    reset(input);
    setStackLimit(DEFAULT_BACKTRACK_STACK_CAPACITY, status);
    if (U_FAILURE(status)) {
        fDeferredStatus = status;
        return;
    }
}


static const UChar BACKSLASH  = 0x5c;
static const UChar DOLLARSIGN = 0x24;





RegexMatcher &RegexMatcher::appendReplacement(UnicodeString &dest,
                                              const UnicodeString &replacement,
                                              UErrorCode &status) {
    UText replacementText = UTEXT_INITIALIZER;
    
    utext_openConstUnicodeString(&replacementText, &replacement, &status);
    if (U_SUCCESS(status)) {        
        UText resultText = UTEXT_INITIALIZER;
        utext_openUnicodeString(&resultText, &dest, &status);
        
        if (U_SUCCESS(status)) {
            appendReplacement(&resultText, &replacementText, status);
            utext_close(&resultText);
        }
        utext_close(&replacementText);
    }
    
    return *this;
}




RegexMatcher &RegexMatcher::appendReplacement(UText *dest,
                                              UText *replacement,
                                              UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return *this;
    }
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        return *this;
    }
    
    
    int64_t  destLen = utext_nativeLength(dest);
    if (fMatchStart > fAppendPosition) {
        if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
            destLen += utext_replace(dest, destLen, destLen, fInputText->chunkContents+fAppendPosition, 
                                     (int32_t)(fMatchStart-fAppendPosition), &status);
        } else {
            int32_t len16;
            if (UTEXT_USES_U16(fInputText)) {
                len16 = (int32_t)(fMatchStart-fAppendPosition);
            } else {
                UErrorCode lengthStatus = U_ZERO_ERROR;
                len16 = utext_extract(fInputText, fAppendPosition, fMatchStart, NULL, 0, &lengthStatus);
            }
            UChar *inputChars = (UChar *)uprv_malloc(sizeof(UChar)*(len16+1));
            if (inputChars == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return *this;
            }
            utext_extract(fInputText, fAppendPosition, fMatchStart, inputChars, len16+1, &status);
            destLen += utext_replace(dest, destLen, destLen, inputChars, len16, &status);
            uprv_free(inputChars);
        }
    }
    fAppendPosition = fMatchEnd;
    
    
    
    
    
    UTEXT_SETNATIVEINDEX(replacement, 0);
    UChar32 c = UTEXT_NEXT32(replacement);
    while (c != U_SENTINEL) {
        if (c == BACKSLASH) {
            
            
            
            
            
            c = UTEXT_CURRENT32(replacement);
            if (c == U_SENTINEL) {
                break;
            }
            
            if (c==0x55 || c==0x75) {
                
                int32_t offset = 0;
                struct URegexUTextUnescapeCharContext context = U_REGEX_UTEXT_UNESCAPE_CONTEXT(replacement);
                UChar32 escapedChar = u_unescapeAt(uregex_utext_unescape_charAt, &offset, INT32_MAX, &context);
                if (escapedChar != (UChar32)0xFFFFFFFF) {
                    if (U_IS_BMP(escapedChar)) {
                        UChar c16 = (UChar)escapedChar;
                        destLen += utext_replace(dest, destLen, destLen, &c16, 1, &status);
                    } else {
                        UChar surrogate[2];
                        surrogate[0] = U16_LEAD(escapedChar);
                        surrogate[1] = U16_TRAIL(escapedChar);
                        if (U_SUCCESS(status)) {
                            destLen += utext_replace(dest, destLen, destLen, surrogate, 2, &status);
                        }
                    }
                    
                    
                    if (context.lastOffset == offset) {
                        (void)UTEXT_PREVIOUS32(replacement);
                    } else if (context.lastOffset != offset-1) {
                        utext_moveIndex32(replacement, offset - context.lastOffset - 1);
                    }
                }
            } else {
                (void)UTEXT_NEXT32(replacement);
                
                if (U_IS_BMP(c)) {
                    UChar c16 = (UChar)c;
                    destLen += utext_replace(dest, destLen, destLen, &c16, 1, &status);
                } else {
                    UChar surrogate[2];
                    surrogate[0] = U16_LEAD(c);
                    surrogate[1] = U16_TRAIL(c);
                    if (U_SUCCESS(status)) {
                        destLen += utext_replace(dest, destLen, destLen, surrogate, 2, &status);
                    }
                }
            }
        } else if (c != DOLLARSIGN) {
            
            if (U_IS_BMP(c)) {
                UChar c16 = (UChar)c;
                destLen += utext_replace(dest, destLen, destLen, &c16, 1, &status);
            } else {
                UChar surrogate[2];
                surrogate[0] = U16_LEAD(c);
                surrogate[1] = U16_TRAIL(c);
                if (U_SUCCESS(status)) {
                    destLen += utext_replace(dest, destLen, destLen, surrogate, 2, &status);
                }
            }
        } else {
            
            
            
            
            int32_t numDigits = 0;
            int32_t groupNum  = 0;
            UChar32 digitC;
            for (;;) {
                digitC = UTEXT_CURRENT32(replacement);
                if (digitC == U_SENTINEL) {
                    break;
                }
                if (u_isdigit(digitC) == FALSE) {
                    break;
                }
                (void)UTEXT_NEXT32(replacement);
                groupNum=groupNum*10 + u_charDigitValue(digitC);
                numDigits++;
                if (numDigits >= fPattern->fMaxCaptureDigits) {
                    break;
                }
            }
            
            
            if (numDigits == 0) {
                
                
                UChar c16 = DOLLARSIGN;
                destLen += utext_replace(dest, destLen, destLen, &c16, 1, &status);
            } else {
                
                destLen += appendGroup(groupNum, dest, status);
                if (U_FAILURE(status)) {
                    
                    break;
                }
            }
        }
        
        if (U_FAILURE(status)) {
            break;
        } else {
            c = UTEXT_NEXT32(replacement);
        }
    }
    
    return *this;
}












UnicodeString &RegexMatcher::appendTail(UnicodeString &dest) {
    UErrorCode status = U_ZERO_ERROR;
    UText resultText = UTEXT_INITIALIZER;
    utext_openUnicodeString(&resultText, &dest, &status);
    
    if (U_SUCCESS(status)) {
        appendTail(&resultText, status);
        utext_close(&resultText);
    }
    
    return dest;
}




UText *RegexMatcher::appendTail(UText *dest, UErrorCode &status) {
    UBool bailOut = FALSE;
    if (U_FAILURE(status)) {
        bailOut = TRUE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        bailOut = TRUE;
    }
    
    if (bailOut) {
        
        if (dest) {
            utext_replace(dest, utext_nativeLength(dest), utext_nativeLength(dest), NULL, 0, &status);
            return dest;
        }
    }
    
    if (fInputLength > fAppendPosition) {
        if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
            int64_t destLen = utext_nativeLength(dest);
            utext_replace(dest, destLen, destLen, fInputText->chunkContents+fAppendPosition, 
                          (int32_t)(fInputLength-fAppendPosition), &status);
        } else {
            int32_t len16;
            if (UTEXT_USES_U16(fInputText)) {
                len16 = (int32_t)(fInputLength-fAppendPosition);
            } else {
                len16 = utext_extract(fInputText, fAppendPosition, fInputLength, NULL, 0, &status);
                status = U_ZERO_ERROR; 
            }
            
            UChar *inputChars = (UChar *)uprv_malloc(sizeof(UChar)*(len16));
            if (inputChars == NULL) {
                fDeferredStatus = U_MEMORY_ALLOCATION_ERROR;
            } else {
                utext_extract(fInputText, fAppendPosition, fInputLength, inputChars, len16, &status); 
                int64_t destLen = utext_nativeLength(dest);
                utext_replace(dest, destLen, destLen, inputChars, len16, &status);
                uprv_free(inputChars);
            }
        }
    }
    return dest;
}








int32_t RegexMatcher::end(UErrorCode &err) const {
    return end(0, err);
}

int64_t RegexMatcher::end64(UErrorCode &err) const {
    return end64(0, err);
}

int64_t RegexMatcher::end64(int32_t group, UErrorCode &err) const {
    if (U_FAILURE(err)) {
        return -1;
    }
    if (fMatch == FALSE) {
        err = U_REGEX_INVALID_STATE;
        return -1;
    }
    if (group < 0 || group > fPattern->fGroupMap->size()) {
        err = U_INDEX_OUTOFBOUNDS_ERROR;
        return -1;
    }
    int64_t e = -1;
    if (group == 0) {
        e = fMatchEnd; 
    } else {
        
        
        int32_t groupOffset = fPattern->fGroupMap->elementAti(group-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        e = fFrame->fExtra[groupOffset + 1];
    }
    
        return e;
}

int32_t RegexMatcher::end(int32_t group, UErrorCode &err) const {
    return (int32_t)end64(group, err);
}







UBool RegexMatcher::find() {
    
    
    
    if (U_FAILURE(fDeferredStatus)) {
        return FALSE;
    }
    
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        return findUsingChunk();
    }

    int64_t startPos = fMatchEnd;
    if (startPos==0) {
        startPos = fActiveStart;
    }

    if (fMatch) {
        
        fLastMatchEnd = fMatchEnd;

        if (fMatchStart == fMatchEnd) {
            
            
            if (startPos >= fActiveLimit) {
                fMatch = FALSE;
                fHitEnd = TRUE;
                return FALSE;
            }
            UTEXT_SETNATIVEINDEX(fInputText, startPos);
            (void)UTEXT_NEXT32(fInputText);
            startPos = UTEXT_GETNATIVEINDEX(fInputText);
        }
    } else {
        if (fLastMatchEnd >= 0) {
            
            
            
            fHitEnd = TRUE;
            return FALSE;
        }
    }


    
    
    
    
    int64_t testStartLimit;
    if (UTEXT_USES_U16(fInputText)) {
        testStartLimit = fActiveLimit - fPattern->fMinMatchLen;
        if (startPos > testStartLimit) {
            fMatch = FALSE;
            fHitEnd = TRUE;
            return FALSE;
        }
    } else {
        
        
        testStartLimit = fActiveLimit;
    }

    UChar32  c;
    U_ASSERT(startPos >= 0);

    switch (fPattern->fStartType) {
    case START_NO_INFO:
        
        
        for (;;) {
            MatchAt(startPos, FALSE, fDeferredStatus);
            if (U_FAILURE(fDeferredStatus)) {
                return FALSE;
            }
            if (fMatch) {
                return TRUE;
            }
            if (startPos >= testStartLimit) {
                fHitEnd = TRUE;
                return FALSE;
            }
            UTEXT_SETNATIVEINDEX(fInputText, startPos);
            (void)UTEXT_NEXT32(fInputText);
            startPos = UTEXT_GETNATIVEINDEX(fInputText);
            
            
            
            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                return FALSE;
        }
        U_ASSERT(FALSE);

    case START_START:
        
        
        if (startPos > fActiveStart) {
            fMatch = FALSE;
            return FALSE;
        }
        MatchAt(startPos, FALSE, fDeferredStatus);
        if (U_FAILURE(fDeferredStatus)) {
            return FALSE;
        }
        return fMatch;


    case START_SET:
        {
            
            U_ASSERT(fPattern->fMinMatchLen > 0);
            int64_t pos;
            UTEXT_SETNATIVEINDEX(fInputText, startPos);
            for (;;) {
                c = UTEXT_NEXT32(fInputText);
                pos = UTEXT_GETNATIVEINDEX(fInputText);
                
                
                
                if (c >= 0 && ((c<256 && fPattern->fInitialChars8->contains(c)) ||
                              (c>=256 && fPattern->fInitialChars->contains(c)))) {
                    MatchAt(startPos, FALSE, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                    UTEXT_SETNATIVEINDEX(fInputText, pos);
                }
                if (startPos >= testStartLimit) {
                    fMatch = FALSE;
                    fHitEnd = TRUE;
                    return FALSE;
                }
                startPos = pos;
	            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                    return FALSE;
            }
        }
        U_ASSERT(FALSE);

    case START_STRING:
    case START_CHAR:
        {
            
            U_ASSERT(fPattern->fMinMatchLen > 0);
            UChar32 theChar = fPattern->fInitialChar;
            int64_t pos;
            UTEXT_SETNATIVEINDEX(fInputText, startPos);
            for (;;) {
                c = UTEXT_NEXT32(fInputText);
                pos = UTEXT_GETNATIVEINDEX(fInputText);
                if (c == theChar) {
                    MatchAt(startPos, FALSE, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                    UTEXT_SETNATIVEINDEX(fInputText, pos);
                }
                if (startPos >= testStartLimit) {
                    fMatch = FALSE;
                    fHitEnd = TRUE;
                    return FALSE;
                }
                startPos = pos;
	            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                    return FALSE;
           }
        }
        U_ASSERT(FALSE);

    case START_LINE:
        {
            UChar32  c;
            if (startPos == fAnchorStart) {
                MatchAt(startPos, FALSE, fDeferredStatus);
                if (U_FAILURE(fDeferredStatus)) {
                    return FALSE;
                }
                if (fMatch) {
                    return TRUE;
                }
                UTEXT_SETNATIVEINDEX(fInputText, startPos);
                c = UTEXT_NEXT32(fInputText);
                startPos = UTEXT_GETNATIVEINDEX(fInputText);
            } else {
                UTEXT_SETNATIVEINDEX(fInputText, startPos);
                c = UTEXT_PREVIOUS32(fInputText);
                UTEXT_SETNATIVEINDEX(fInputText, startPos);
            }

            if (fPattern->fFlags & UREGEX_UNIX_LINES) {
                for (;;) {
                    if (c == 0x0a) {
                            MatchAt(startPos, FALSE, fDeferredStatus);
                            if (U_FAILURE(fDeferredStatus)) {
                                return FALSE;
                            }
                            if (fMatch) {
                                return TRUE;
                            }
                            UTEXT_SETNATIVEINDEX(fInputText, startPos);
                    }
                    if (startPos >= testStartLimit) {
                        fMatch = FALSE;
                        fHitEnd = TRUE;
                        return FALSE;
                    }
                    c = UTEXT_NEXT32(fInputText);
                    startPos = UTEXT_GETNATIVEINDEX(fInputText);
                    
                    
                    
		            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                        return FALSE;
                }
            } else {
                for (;;) {
                    if (((c & 0x7f) <= 0x29) &&     
                        ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029 )) {
                            if (c == 0x0d && startPos < fActiveLimit && UTEXT_CURRENT32(fInputText) == 0x0a) {
                                (void)UTEXT_NEXT32(fInputText);
                                startPos = UTEXT_GETNATIVEINDEX(fInputText);
                            }
                            MatchAt(startPos, FALSE, fDeferredStatus);
                            if (U_FAILURE(fDeferredStatus)) {
                                return FALSE;
                            }
                            if (fMatch) {
                                return TRUE;
                            }
                            UTEXT_SETNATIVEINDEX(fInputText, startPos);
                    }
                    if (startPos >= testStartLimit) {
                        fMatch = FALSE;
                        fHitEnd = TRUE;
                        return FALSE;
                    }
                    c = UTEXT_NEXT32(fInputText);
                    startPos = UTEXT_GETNATIVEINDEX(fInputText);
                    
                    
                    
		            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                        return FALSE;
                }
            }
        }

    default:
        U_ASSERT(FALSE);
    }

    U_ASSERT(FALSE);
    return FALSE;
}



UBool RegexMatcher::find(int64_t start, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    this->reset();                        
                                          
    if (start < 0) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    
    int64_t nativeStart = start;
    if (nativeStart < fActiveStart || nativeStart > fActiveLimit) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    fMatchEnd = nativeStart;  
    return find();
}








UBool RegexMatcher::findUsingChunk() {
    
    
    

    int32_t startPos = (int32_t)fMatchEnd;
    if (startPos==0) {
        startPos = (int32_t)fActiveStart;
    }
    
    const UChar *inputBuf = fInputText->chunkContents;

    if (fMatch) {
        
        fLastMatchEnd = fMatchEnd;
        
        if (fMatchStart == fMatchEnd) {
            
            
            if (startPos >= fActiveLimit) {
                fMatch = FALSE;
                fHitEnd = TRUE;
                return FALSE;
            }
            U16_FWD_1(inputBuf, startPos, fInputLength);
        }
    } else {
        if (fLastMatchEnd >= 0) {
            
            
            
            fHitEnd = TRUE;
            return FALSE;
        }
    }
    
    
    
    
    
    
    int32_t testLen  = (int32_t)(fActiveLimit - fPattern->fMinMatchLen);
    if (startPos > testLen) {
        fMatch = FALSE;
        fHitEnd = TRUE;
        return FALSE;
    }
    
    UChar32  c;
    U_ASSERT(startPos >= 0);
    
    switch (fPattern->fStartType) {
    case START_NO_INFO:
        
        
        for (;;) {
            MatchChunkAt(startPos, FALSE, fDeferredStatus);
            if (U_FAILURE(fDeferredStatus)) {
                return FALSE;
            }
            if (fMatch) {
                return TRUE;
            }
            if (startPos >= testLen) {
                fHitEnd = TRUE;
                return FALSE;
            }
            U16_FWD_1(inputBuf, startPos, fActiveLimit);
            
            
            
            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                return FALSE;
        }
        U_ASSERT(FALSE);
        
    case START_START:
        
        
        if (startPos > fActiveStart) {
            fMatch = FALSE;
            return FALSE;
        }
        MatchChunkAt(startPos, FALSE, fDeferredStatus);
        if (U_FAILURE(fDeferredStatus)) {
            return FALSE;
        }
        return fMatch;
        
        
    case START_SET:
    {
        
        U_ASSERT(fPattern->fMinMatchLen > 0);
        for (;;) {
            int32_t pos = startPos;
            U16_NEXT(inputBuf, startPos, fActiveLimit, c);  
            if ((c<256 && fPattern->fInitialChars8->contains(c)) ||
                (c>=256 && fPattern->fInitialChars->contains(c))) {
                MatchChunkAt(pos, FALSE, fDeferredStatus);
                if (U_FAILURE(fDeferredStatus)) {
                    return FALSE;
                }
                if (fMatch) {
                    return TRUE;
                }
            }
            if (pos >= testLen) {
                fMatch = FALSE;
                fHitEnd = TRUE;
                return FALSE;
            }
            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                return FALSE;
        }
    }
        U_ASSERT(FALSE);
        
    case START_STRING:
    case START_CHAR:
    {
        
        U_ASSERT(fPattern->fMinMatchLen > 0);
        UChar32 theChar = fPattern->fInitialChar;
        for (;;) {
            int32_t pos = startPos;
            U16_NEXT(inputBuf, startPos, fActiveLimit, c);  
            if (c == theChar) {
                MatchChunkAt(pos, FALSE, fDeferredStatus);
                if (U_FAILURE(fDeferredStatus)) {
                    return FALSE;
                }
                if (fMatch) {
                    return TRUE;
                }
            }
            if (pos >= testLen) {
                fMatch = FALSE;
                fHitEnd = TRUE;
                return FALSE;
            }
            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                return FALSE;
        }
    }
        U_ASSERT(FALSE);
        
    case START_LINE:
    {
        UChar32  c;
        if (startPos == fAnchorStart) {
            MatchChunkAt(startPos, FALSE, fDeferredStatus);
            if (U_FAILURE(fDeferredStatus)) {
                return FALSE;
            }
            if (fMatch) {
                return TRUE;
            }
            U16_FWD_1(inputBuf, startPos, fActiveLimit);
        }
        
        if (fPattern->fFlags & UREGEX_UNIX_LINES) {
            for (;;) {
                c = inputBuf[startPos-1];
                if (c == 0x0a) {
                    MatchChunkAt(startPos, FALSE, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                }
                if (startPos >= testLen) {
                    fMatch = FALSE;
                    fHitEnd = TRUE;
                    return FALSE;
                }
                U16_FWD_1(inputBuf, startPos, fActiveLimit);
                
                
                
	            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                    return FALSE;
            }
        } else {
            for (;;) {
                c = inputBuf[startPos-1];
                if (((c & 0x7f) <= 0x29) &&     
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029 )) {
                    if (c == 0x0d && startPos < fActiveLimit && inputBuf[startPos] == 0x0a) {
                        startPos++;
                    }
                    MatchChunkAt(startPos, FALSE, fDeferredStatus);
                    if (U_FAILURE(fDeferredStatus)) {
                        return FALSE;
                    }
                    if (fMatch) {
                        return TRUE;
                    }
                }
                if (startPos >= testLen) {
                    fMatch = FALSE;
                    fHitEnd = TRUE;
                    return FALSE;
                }
                U16_FWD_1(inputBuf, startPos, fActiveLimit);
                
                
                
	            if  (REGEXFINDPROGRESS_INTERRUPT(startPos, fDeferredStatus))
                    return FALSE;
            }
        }
    }
        
    default:
        U_ASSERT(FALSE);
    }
    
    U_ASSERT(FALSE);
    return FALSE;
}








UnicodeString RegexMatcher::group(UErrorCode &status) const {
    return group(0, status);
}


UText *RegexMatcher::group(UText *dest, int64_t &group_len, UErrorCode &status) const {
    return group(0, dest, group_len, status);
}


UText *RegexMatcher::group(int32_t groupNum, UText *dest, int64_t &group_len, UErrorCode &status) const {
    group_len = 0;
    UBool bailOut = FALSE;
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        bailOut = TRUE;
    }
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        bailOut = TRUE;
    }
    if (groupNum < 0 || groupNum > fPattern->fGroupMap->size()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        bailOut = TRUE;
    }
    
    if (bailOut) {
        return (dest) ? dest : utext_openUChars(NULL, NULL, 0, &status);
    }
    
    int64_t s, e;
    if (groupNum == 0) {
        s = fMatchStart;
        e = fMatchEnd;
    } else {
        int32_t groupOffset = fPattern->fGroupMap->elementAti(groupNum-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        s = fFrame->fExtra[groupOffset];
        e = fFrame->fExtra[groupOffset+1];
    }

    if (s < 0) {
        
        return utext_clone(dest, fInputText, FALSE, TRUE, &status);
    }
    U_ASSERT(s <= e);
    group_len = e - s;
    
    dest = utext_clone(dest, fInputText, FALSE, TRUE, &status);
    if (dest)
        UTEXT_SETNATIVEINDEX(dest, s);
    return dest;
}

UnicodeString RegexMatcher::group(int32_t groupNum, UErrorCode &status) const {
    UnicodeString result;
    if (U_FAILURE(status)) {
        return result;
    }
    UText resultText = UTEXT_INITIALIZER;
    utext_openUnicodeString(&resultText, &result, &status);
    group(groupNum, &resultText, status);
    utext_close(&resultText);
    return result;
}





UText *RegexMatcher::group(int32_t groupNum, UText *dest, UErrorCode &status) const {
    UBool bailOut = FALSE;
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        bailOut = TRUE;
    }
    
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        bailOut = TRUE;
    }
    if (groupNum < 0 || groupNum > fPattern->fGroupMap->size()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        bailOut = TRUE;
    }
    
    if (bailOut) {
        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), NULL, 0, &status);
            return dest;
        } else {
            return utext_openUChars(NULL, NULL, 0, &status);
        }
    }
    
    int64_t s, e;
    if (groupNum == 0) {
        s = fMatchStart;
        e = fMatchEnd;
    } else {
        int32_t groupOffset = fPattern->fGroupMap->elementAti(groupNum-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        s = fFrame->fExtra[groupOffset];
        e = fFrame->fExtra[groupOffset+1];
    }
    
    if (s < 0) {
        
        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), NULL, 0, &status);
            return dest;
        } else {
            return utext_openUChars(NULL, NULL, 0, &status);
        }
    }
    U_ASSERT(s <= e);
    
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        U_ASSERT(e <= fInputLength);
        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), fInputText->chunkContents+s, (int32_t)(e-s), &status);
        } else {
            UText groupText = UTEXT_INITIALIZER;
            utext_openUChars(&groupText, fInputText->chunkContents+s, e-s, &status);
            dest = utext_clone(NULL, &groupText, TRUE, FALSE, &status);
            utext_close(&groupText);
        }
    } else {
        int32_t len16;
        if (UTEXT_USES_U16(fInputText)) {
            len16 = (int32_t)(e-s);
        } else {
            UErrorCode lengthStatus = U_ZERO_ERROR;
            len16 = utext_extract(fInputText, s, e, NULL, 0, &lengthStatus);
        }
        UChar *groupChars = (UChar *)uprv_malloc(sizeof(UChar)*(len16+1));
        if (groupChars == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return dest;
        }
        utext_extract(fInputText, s, e, groupChars, len16+1, &status);

        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), groupChars, len16, &status);
        } else {
            UText groupText = UTEXT_INITIALIZER;
            utext_openUChars(&groupText, groupChars, len16, &status);
            dest = utext_clone(NULL, &groupText, TRUE, FALSE, &status);
            utext_close(&groupText);
        }
        
        uprv_free(groupChars);
    }
    return dest;
}








int64_t RegexMatcher::appendGroup(int32_t groupNum, UText *dest, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return 0;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return 0;
    }
    int64_t destLen = utext_nativeLength(dest);
    
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        return utext_replace(dest, destLen, destLen, NULL, 0, &status);
    }
    if (groupNum < 0 || groupNum > fPattern->fGroupMap->size()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return utext_replace(dest, destLen, destLen, NULL, 0, &status);
    }
    
    int64_t s, e;
    if (groupNum == 0) {
        s = fMatchStart;
        e = fMatchEnd;
    } else {
        int32_t groupOffset = fPattern->fGroupMap->elementAti(groupNum-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        s = fFrame->fExtra[groupOffset];
        e = fFrame->fExtra[groupOffset+1];
    }
    
    if (s < 0) {
        
        return utext_replace(dest, destLen, destLen, NULL, 0, &status);
    }
    U_ASSERT(s <= e);
    
    int64_t deltaLen;
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        U_ASSERT(e <= fInputLength);
        deltaLen = utext_replace(dest, destLen, destLen, fInputText->chunkContents+s, (int32_t)(e-s), &status);
    } else {
        int32_t len16;
        if (UTEXT_USES_U16(fInputText)) {
            len16 = (int32_t)(e-s);
        } else {
            UErrorCode lengthStatus = U_ZERO_ERROR;
            len16 = utext_extract(fInputText, s, e, NULL, 0, &lengthStatus);
        }
        UChar *groupChars = (UChar *)uprv_malloc(sizeof(UChar)*(len16+1));
        if (groupChars == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        utext_extract(fInputText, s, e, groupChars, len16+1, &status);
    
        deltaLen = utext_replace(dest, destLen, destLen, groupChars, len16, &status);
        uprv_free(groupChars);
    }
    return deltaLen;
}








int32_t RegexMatcher::groupCount() const {
    return fPattern->fGroupMap->size();
}








UBool RegexMatcher::hasAnchoringBounds() const {
    return fAnchoringBounds;
}







UBool RegexMatcher::hasTransparentBounds() const {
    return fTransparentBounds;
}








UBool RegexMatcher::hitEnd() const {
    return fHitEnd;
}







const UnicodeString &RegexMatcher::input() const {
    if (!fInput) {
        UErrorCode status = U_ZERO_ERROR;
        int32_t len16;
        if (UTEXT_USES_U16(fInputText)) {
            len16 = (int32_t)fInputLength;
        } else {
            len16 = utext_extract(fInputText, 0, fInputLength, NULL, 0, &status);
            status = U_ZERO_ERROR; 
        }
        UnicodeString *result = new UnicodeString(len16, 0, 0);
        
        UChar *inputChars = result->getBuffer(len16);
        utext_extract(fInputText, 0, fInputLength, inputChars, len16, &status); 
        result->releaseBuffer(len16);
        
        (*(const UnicodeString **)&fInput) = result; 
    }
    
    return *fInput;
}






UText *RegexMatcher::inputText() const {
    return fInputText;
}







UText *RegexMatcher::getInput (UText *dest, UErrorCode &status) const {
    UBool bailOut = FALSE;
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        bailOut = TRUE;
    }
    
    if (bailOut) {
        if (dest) {
            utext_replace(dest, 0, utext_nativeLength(dest), NULL, 0, &status);
            return dest;
        } else {
            return utext_clone(NULL, fInputText, FALSE, TRUE, &status);
        }
    }
    
    if (dest) {
        if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
            utext_replace(dest, 0, utext_nativeLength(dest), fInputText->chunkContents, (int32_t)fInputLength, &status);
        } else {
            int32_t input16Len;
            if (UTEXT_USES_U16(fInputText)) {
                input16Len = (int32_t)fInputLength;
            } else {
                UErrorCode lengthStatus = U_ZERO_ERROR;
                input16Len = utext_extract(fInputText, 0, fInputLength, NULL, 0, &lengthStatus); 
            }
            UChar *inputChars = (UChar *)uprv_malloc(sizeof(UChar)*(input16Len));
            if (inputChars == NULL) {
                return dest;
            }
            
            status = U_ZERO_ERROR;
            utext_extract(fInputText, 0, fInputLength, inputChars, input16Len, &status); 
            status = U_ZERO_ERROR;
            utext_replace(dest, 0, utext_nativeLength(dest), inputChars, input16Len, &status);
            
            uprv_free(inputChars);
        }
        return dest;
    } else {
        return utext_clone(NULL, fInputText, FALSE, TRUE, &status);
    }
}


static UBool compat_SyncMutableUTextContents(UText *ut);
static UBool compat_SyncMutableUTextContents(UText *ut) {
    UBool retVal = FALSE;
    
    
    
    
    if (utext_nativeLength(ut) != ut->nativeIndexingLimit) {
        UnicodeString *us=(UnicodeString *)ut->context;
    
        
        
        int32_t newLength = us->length();
    
        
        
        ut->chunkContents    = us->getBuffer();
        ut->chunkLength      = newLength;
        ut->chunkNativeLimit = newLength;
        ut->nativeIndexingLimit = newLength;
        retVal = TRUE;
    }

    return retVal;
}






UBool RegexMatcher::lookingAt(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    
    if (fInputUniStrMaybeMutable) {
        if (compat_SyncMutableUTextContents(fInputText)) {
        fInputLength = utext_nativeLength(fInputText);
        reset();
        }
    }
    else {
        resetPreserveRegion();
    }
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        MatchChunkAt((int32_t)fActiveStart, FALSE, status);
    } else {
        MatchAt(fActiveStart, FALSE, status);
    }
    return fMatch;
}


UBool RegexMatcher::lookingAt(int64_t start, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    reset();
    
    if (start < 0) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    
    if (fInputUniStrMaybeMutable) {
        if (compat_SyncMutableUTextContents(fInputText)) {
        fInputLength = utext_nativeLength(fInputText);
        reset();
        }
    }

    int64_t nativeStart;
    nativeStart = start;
    if (nativeStart < fActiveStart || nativeStart > fActiveLimit) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }
    
    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        MatchChunkAt((int32_t)nativeStart, FALSE, status);
    } else {
        MatchAt(nativeStart, FALSE, status);
    }
    return fMatch;
}








UBool RegexMatcher::matches(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }

    if (fInputUniStrMaybeMutable) {
        if (compat_SyncMutableUTextContents(fInputText)) {
        fInputLength = utext_nativeLength(fInputText);
        reset();
        }
    }
    else {
        resetPreserveRegion();
    }

    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        MatchChunkAt((int32_t)fActiveStart, TRUE, status);
    } else {
        MatchAt(fActiveStart, TRUE, status);
    }
    return fMatch;
}


UBool RegexMatcher::matches(int64_t start, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return FALSE;
    }
    reset();
    
    if (start < 0) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }

    if (fInputUniStrMaybeMutable) {
        if (compat_SyncMutableUTextContents(fInputText)) {
        fInputLength = utext_nativeLength(fInputText);
        reset();
        }
    }

    int64_t nativeStart;
    nativeStart = start;
    if (nativeStart < fActiveStart || nativeStart > fActiveLimit) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return FALSE;
    }

    if (UTEXT_FULL_TEXT_IN_CHUNK(fInputText, fInputLength)) {
        MatchChunkAt((int32_t)nativeStart, TRUE, status);
    } else {
        MatchAt(nativeStart, TRUE, status);
    }
    return fMatch;
}








const RegexPattern &RegexMatcher::pattern() const {
    return *fPattern;
}








RegexMatcher &RegexMatcher::region(int64_t regionStart, int64_t regionLimit, int64_t startIndex, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    
    if (regionStart>regionLimit || regionStart<0 || regionLimit<0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
        
    int64_t nativeStart = regionStart;
    int64_t nativeLimit = regionLimit;
    if (nativeStart > fInputLength || nativeLimit > fInputLength) {
      status = U_ILLEGAL_ARGUMENT_ERROR;
    }

    if (startIndex == -1)
      this->reset();
    else
      resetPreserveRegion();    
    
    fRegionStart = nativeStart;
    fRegionLimit = nativeLimit;
    fActiveStart = nativeStart;
    fActiveLimit = nativeLimit;

    if (startIndex != -1) {
      if (startIndex < fActiveStart || startIndex > fActiveLimit) {
          status = U_INDEX_OUTOFBOUNDS_ERROR;
      }
      fMatchEnd = startIndex;  
    }

    if (!fTransparentBounds) {
        fLookStart = nativeStart;
        fLookLimit = nativeLimit;
    }
    if (fAnchoringBounds) {
        fAnchorStart = nativeStart;
        fAnchorLimit = nativeLimit;
    }
    return *this;
}

RegexMatcher &RegexMatcher::region(int64_t start, int64_t limit, UErrorCode &status) {
  return region(start, limit, -1, status);
}






int32_t RegexMatcher::regionEnd() const {
    return (int32_t)fRegionLimit;
}

int64_t RegexMatcher::regionEnd64() const {
    return fRegionLimit;
}






int32_t RegexMatcher::regionStart() const {
    return (int32_t)fRegionStart;
}

int64_t RegexMatcher::regionStart64() const {
    return fRegionStart;
}







UnicodeString RegexMatcher::replaceAll(const UnicodeString &replacement, UErrorCode &status) {
    UText replacementText = UTEXT_INITIALIZER;
    UText resultText = UTEXT_INITIALIZER;
    UnicodeString resultString;
    if (U_FAILURE(status)) {
        return resultString;
    }
    
    utext_openConstUnicodeString(&replacementText, &replacement, &status);
    utext_openUnicodeString(&resultText, &resultString, &status);
        
    replaceAll(&replacementText, &resultText, status);

    utext_close(&resultText);
    utext_close(&replacementText);
    
    return resultString;
}





UText *RegexMatcher::replaceAll(UText *replacement, UText *dest, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return dest;
    }
    
    if (dest == NULL) {
        UnicodeString emptyString;
        UText empty = UTEXT_INITIALIZER;
        
        utext_openUnicodeString(&empty, &emptyString, &status);
        dest = utext_clone(NULL, &empty, TRUE, FALSE, &status);
        utext_close(&empty);
    }

    if (U_SUCCESS(status)) {
        reset();
        while (find()) {
            appendReplacement(dest, replacement, status);
            if (U_FAILURE(status)) {
                break;
            }
        }
        appendTail(dest, status);
    }
    
    return dest;
}







UnicodeString RegexMatcher::replaceFirst(const UnicodeString &replacement, UErrorCode &status) {
    UText replacementText = UTEXT_INITIALIZER;
    UText resultText = UTEXT_INITIALIZER;
    UnicodeString resultString;
    
    utext_openConstUnicodeString(&replacementText, &replacement, &status);
    utext_openUnicodeString(&resultText, &resultString, &status);
    
    replaceFirst(&replacementText, &resultText, status);
    
    utext_close(&resultText);
    utext_close(&replacementText);
    
    return resultString;
}




UText *RegexMatcher::replaceFirst(UText *replacement, UText *dest, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return dest;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return dest;
    }

    reset();
    if (!find()) {
        return getInput(dest, status);
    }
    
    if (dest == NULL) {
        UnicodeString emptyString;
        UText empty = UTEXT_INITIALIZER;
        
        utext_openUnicodeString(&empty, &emptyString, &status);
        dest = utext_clone(NULL, &empty, TRUE, FALSE, &status);
        utext_close(&empty);
    }
    
    appendReplacement(dest, replacement, status);
    appendTail(dest, status);
    
    return dest;
}







UBool RegexMatcher::requireEnd() const {
    return fRequireEnd;
}







RegexMatcher &RegexMatcher::reset() {
    fRegionStart    = 0;
    fRegionLimit    = fInputLength;
    fActiveStart    = 0;
    fActiveLimit    = fInputLength;
    fAnchorStart    = 0;
    fAnchorLimit    = fInputLength;
    fLookStart      = 0;
    fLookLimit      = fInputLength;
    resetPreserveRegion();
    return *this;
}



void RegexMatcher::resetPreserveRegion() {
    fMatchStart     = 0;
    fMatchEnd       = 0;
    fLastMatchEnd   = -1;
    fAppendPosition = 0;
    fMatch          = FALSE;
    fHitEnd         = FALSE;
    fRequireEnd     = FALSE;
    fTime           = 0;
    fTickCounter    = TIMER_INITIAL_VALUE;
    
}


RegexMatcher &RegexMatcher::reset(const UnicodeString &input) {
    fInputText = utext_openConstUnicodeString(fInputText, &input, &fDeferredStatus);
    if (fPattern->fNeedsAltInput) {
        fAltInputText = utext_clone(fAltInputText, fInputText, FALSE, TRUE, &fDeferredStatus);
    }
    fInputLength = utext_nativeLength(fInputText);
    
    reset();
    delete fInput;
    fInput = NULL;

    
    
    fInputUniStrMaybeMutable = TRUE;    
    
    if (fWordBreakItr != NULL) {
#if UCONFIG_NO_BREAK_ITERATION==0
        UErrorCode status = U_ZERO_ERROR;
        fWordBreakItr->setText(fInputText, status);
#endif
    }
    return *this;
}


RegexMatcher &RegexMatcher::reset(UText *input) {
    if (fInputText != input) {
        fInputText = utext_clone(fInputText, input, FALSE, TRUE, &fDeferredStatus);
        if (fPattern->fNeedsAltInput) fAltInputText = utext_clone(fAltInputText, fInputText, FALSE, TRUE, &fDeferredStatus);
        fInputLength = utext_nativeLength(fInputText);
    
        delete fInput;
        fInput = NULL;
        
        if (fWordBreakItr != NULL) {
#if UCONFIG_NO_BREAK_ITERATION==0
            UErrorCode status = U_ZERO_ERROR;
            fWordBreakItr->setText(input, status);
#endif
        }
    }
    reset();
    fInputUniStrMaybeMutable = FALSE;

    return *this;
}






RegexMatcher &RegexMatcher::reset(int64_t position, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    reset();       
    
    if (position < 0 || position > fActiveLimit) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return *this;
    }
    fMatchEnd = position;
    return *this;
}







RegexMatcher &RegexMatcher::refreshInputText(UText *input, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return *this;
    }
    if (input == NULL) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return *this;
    }
    if (utext_nativeLength(fInputText) != utext_nativeLength(input)) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return *this;
    }
    int64_t  pos = utext_getNativeIndex(fInputText);
    
    fInputText = utext_clone(fInputText, input, FALSE, TRUE, &status);
    if (U_FAILURE(status)) {
        return *this;
    }
    utext_setNativeIndex(fInputText, pos);

    if (fAltInputText != NULL) {
        pos = utext_getNativeIndex(fAltInputText);
        fAltInputText = utext_clone(fAltInputText, input, FALSE, TRUE, &status);
        if (U_FAILURE(status)) {
            return *this;
        }
        utext_setNativeIndex(fAltInputText, pos);
    }
    return *this;
}








void RegexMatcher::setTrace(UBool state) {
    fTraceDebug = state;
}








int32_t  RegexMatcher::split(const UnicodeString &input,
        UnicodeString    dest[],
        int32_t          destCapacity,
        UErrorCode      &status)
{
    UText inputText = UTEXT_INITIALIZER;
    utext_openConstUnicodeString(&inputText, &input, &status);
    if (U_FAILURE(status)) {
        return 0;
    }

    UText **destText = (UText **)uprv_malloc(sizeof(UText*)*destCapacity);
    if (destText == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    int32_t i;
    for (i = 0; i < destCapacity; i++) {
        destText[i] = utext_openUnicodeString(NULL, &dest[i], &status);
    }
    
    int32_t fieldCount = split(&inputText, destText, destCapacity, status);
    
    for (i = 0; i < destCapacity; i++) {
        utext_close(destText[i]);
    }

    uprv_free(destText);
    utext_close(&inputText);
    return fieldCount;
}




int32_t  RegexMatcher::split(UText *input,
        UText           *dest[],
        int32_t          destCapacity,
        UErrorCode      &status)
{
    
    
    
    if (U_FAILURE(status)) {
        return 0;
    };

    if (destCapacity < 1) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    
    
    reset(input);
    int64_t   nextOutputStringStart = 0;
    if (fActiveLimit == 0) {
        return 0;
    }

    
    
    
    int32_t i;
    int32_t numCaptureGroups = fPattern->fGroupMap->size();
    for (i=0; ; i++) {
        if (i>=destCapacity-1) {
            
            
            
            
            
            
            i = destCapacity-1;
            if (fActiveLimit > nextOutputStringStart) {
                if (UTEXT_FULL_TEXT_IN_CHUNK(input, fInputLength)) {
                    if (dest[i]) {
                        utext_replace(dest[i], 0, utext_nativeLength(dest[i]), 
                                      input->chunkContents+nextOutputStringStart, 
                                      (int32_t)(fActiveLimit-nextOutputStringStart), &status);
                    } else {
                        UText remainingText = UTEXT_INITIALIZER;
                        utext_openUChars(&remainingText, input->chunkContents+nextOutputStringStart, 
                                         fActiveLimit-nextOutputStringStart, &status);
                        dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                        utext_close(&remainingText);
                    }
                } else {
                    UErrorCode lengthStatus = U_ZERO_ERROR;
                    int32_t remaining16Length = 
                        utext_extract(input, nextOutputStringStart, fActiveLimit, NULL, 0, &lengthStatus);
                    UChar *remainingChars = (UChar *)uprv_malloc(sizeof(UChar)*(remaining16Length+1));
                    if (remainingChars == NULL) {
                        status = U_MEMORY_ALLOCATION_ERROR;
                        break;
                    }

                    utext_extract(input, nextOutputStringStart, fActiveLimit, remainingChars, remaining16Length+1, &status);
                    if (dest[i]) {
                        utext_replace(dest[i], 0, utext_nativeLength(dest[i]), remainingChars, remaining16Length, &status);
                    } else {
                        UText remainingText = UTEXT_INITIALIZER;
                        utext_openUChars(&remainingText, remainingChars, remaining16Length, &status);
                        dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                        utext_close(&remainingText);
                    }
                    
                    uprv_free(remainingChars);
                }
            }
            break;
        }
        if (find()) {
            
            
            if (UTEXT_FULL_TEXT_IN_CHUNK(input, fInputLength)) {
                if (dest[i]) {
                    utext_replace(dest[i], 0, utext_nativeLength(dest[i]), 
                                  input->chunkContents+nextOutputStringStart, 
                                  (int32_t)(fMatchStart-nextOutputStringStart), &status);
                } else {
                    UText remainingText = UTEXT_INITIALIZER;
                    utext_openUChars(&remainingText, input->chunkContents+nextOutputStringStart, 
                                      fMatchStart-nextOutputStringStart, &status);
                    dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                    utext_close(&remainingText);
                }
            } else {
                UErrorCode lengthStatus = U_ZERO_ERROR;
                int32_t remaining16Length = utext_extract(input, nextOutputStringStart, fMatchStart, NULL, 0, &lengthStatus);
                UChar *remainingChars = (UChar *)uprv_malloc(sizeof(UChar)*(remaining16Length+1));
                if (remainingChars == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    break;
                }
                utext_extract(input, nextOutputStringStart, fMatchStart, remainingChars, remaining16Length+1, &status);
                if (dest[i]) {
                    utext_replace(dest[i], 0, utext_nativeLength(dest[i]), remainingChars, remaining16Length, &status);
                } else {
                    UText remainingText = UTEXT_INITIALIZER;
                    utext_openUChars(&remainingText, remainingChars, remaining16Length, &status);
                    dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                    utext_close(&remainingText);
                }
                
                uprv_free(remainingChars);
            }
            nextOutputStringStart = fMatchEnd;

            
            
            int32_t groupNum;
            for (groupNum=1; groupNum<=numCaptureGroups; groupNum++) {
                if (i >= destCapacity-2) {
                    
                    
                    
                    break;
                }
                i++;
                dest[i] = group(groupNum, dest[i], status);
            }

            if (nextOutputStringStart == fActiveLimit) {
                
                
                
                if (i+1 < destCapacity) {
                    ++i;
                    if (dest[i] == NULL) {
                        dest[i] = utext_openUChars(NULL, NULL, 0, &status);
                    } else {
                        static UChar emptyString[] = {(UChar)0};
                        utext_replace(dest[i], 0, utext_nativeLength(dest[i]), emptyString, 0, &status);
                    }
                }
                break;
            
            } 
        }
        else
        {
            
            
            if (UTEXT_FULL_TEXT_IN_CHUNK(input, fInputLength)) {
                if (dest[i]) {
                    utext_replace(dest[i], 0, utext_nativeLength(dest[i]), 
                                  input->chunkContents+nextOutputStringStart, 
                                  (int32_t)(fActiveLimit-nextOutputStringStart), &status);
                } else {
                    UText remainingText = UTEXT_INITIALIZER;
                    utext_openUChars(&remainingText, input->chunkContents+nextOutputStringStart, 
                                     fActiveLimit-nextOutputStringStart, &status);
                    dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                    utext_close(&remainingText);
                }
            } else {
                UErrorCode lengthStatus = U_ZERO_ERROR;
                int32_t remaining16Length = utext_extract(input, nextOutputStringStart, fActiveLimit, NULL, 0, &lengthStatus);
                UChar *remainingChars = (UChar *)uprv_malloc(sizeof(UChar)*(remaining16Length+1));
                if (remainingChars == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    break;
                }
                
                utext_extract(input, nextOutputStringStart, fActiveLimit, remainingChars, remaining16Length+1, &status);
                if (dest[i]) {
                    utext_replace(dest[i], 0, utext_nativeLength(dest[i]), remainingChars, remaining16Length, &status);
                } else {
                    UText remainingText = UTEXT_INITIALIZER;
                    utext_openUChars(&remainingText, remainingChars, remaining16Length, &status);
                    dest[i] = utext_clone(NULL, &remainingText, TRUE, FALSE, &status);
                    utext_close(&remainingText);
                }
                
                uprv_free(remainingChars);
            }
            break;
        }
        if (U_FAILURE(status)) {
            break;
        }
    }   
    return i+1;
}







int32_t RegexMatcher::start(UErrorCode &status) const {
    return start(0, status);
}

int64_t RegexMatcher::start64(UErrorCode &status) const {
    return start64(0, status);
}







int64_t RegexMatcher::start64(int32_t group, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return -1;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return -1;
    }
    if (fMatch == FALSE) {
        status = U_REGEX_INVALID_STATE;
        return -1;
    }
    if (group < 0 || group > fPattern->fGroupMap->size()) {
        status = U_INDEX_OUTOFBOUNDS_ERROR;
        return -1;
    }
    int64_t s;
    if (group == 0) {
        s = fMatchStart; 
    } else {
        int32_t groupOffset = fPattern->fGroupMap->elementAti(group-1);
        U_ASSERT(groupOffset < fPattern->fFrameSize);
        U_ASSERT(groupOffset >= 0);
        s = fFrame->fExtra[groupOffset];
    }
    
    return s;
}


int32_t RegexMatcher::start(int32_t group, UErrorCode &status) const {
    return (int32_t)start64(group, status);
}






RegexMatcher &RegexMatcher::useAnchoringBounds(UBool b) {
    fAnchoringBounds = b;
    fAnchorStart = (fAnchoringBounds ? fRegionStart : 0);
    fAnchorLimit = (fAnchoringBounds ? fRegionLimit : fInputLength);
    return *this;
}







RegexMatcher &RegexMatcher::useTransparentBounds(UBool b) {
    fTransparentBounds = b;
    fLookStart = (fTransparentBounds ? 0 : fRegionStart);
    fLookLimit = (fTransparentBounds ? fInputLength : fRegionLimit);
    return *this;
}






void RegexMatcher::setTimeLimit(int32_t limit, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return;
    }
    if (limit < 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    fTimeLimit = limit;
}







int32_t RegexMatcher::getTimeLimit() const {
    return fTimeLimit;
}







void RegexMatcher::setStackLimit(int32_t limit, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (U_FAILURE(fDeferredStatus)) {
        status = fDeferredStatus;
        return;
    }
    if (limit < 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    
    
    
    reset();
    
    if (limit == 0) {
        
        fStack->setMaxCapacity(0);
    } else {
        
        
        
        int32_t adjustedLimit = limit / sizeof(int32_t);
        if (adjustedLimit < fPattern->fFrameSize) {
            adjustedLimit = fPattern->fFrameSize;
        }
        fStack->setMaxCapacity(adjustedLimit);
    }
    fStackLimit = limit;
}







int32_t RegexMatcher::getStackLimit() const {
    return fStackLimit;
}







void RegexMatcher::setMatchCallback(URegexMatchCallback     *callback,
                                    const void              *context,
                                    UErrorCode              &status) {
    if (U_FAILURE(status)) {
        return;
    }
    fCallbackFn = callback;
    fCallbackContext = context;
}







void RegexMatcher::getMatchCallback(URegexMatchCallback   *&callback,
                                  const void              *&context,
                                  UErrorCode              &status) {
    if (U_FAILURE(status)) {
       return;
    }
    callback = fCallbackFn;
    context  = fCallbackContext;
}







void RegexMatcher::setFindProgressCallback(URegexFindProgressCallback      *callback,
                                                const void                      *context,
                                                UErrorCode                      &status) {
    if (U_FAILURE(status)) {
        return;
    }
    fFindProgressCallbackFn = callback;
    fFindProgressCallbackContext = context;
}







void RegexMatcher::getFindProgressCallback(URegexFindProgressCallback    *&callback,
                                                const void                    *&context,
                                                UErrorCode                    &status) {
    if (U_FAILURE(status)) {
       return;
    }
    callback = fFindProgressCallbackFn;
    context  = fFindProgressCallbackContext;
}

















REStackFrame *RegexMatcher::resetStack() {
    
    
    
    fStack->removeAllElements();

    REStackFrame *iFrame = (REStackFrame *)fStack->reserveBlock(fPattern->fFrameSize, fDeferredStatus);
    int32_t i;
    for (i=0; i<fPattern->fFrameSize-RESTACKFRAME_HDRCOUNT; i++) {
        iFrame->fExtra[i] = -1;
    }
    return iFrame;
}



















UBool RegexMatcher::isWordBoundary(int64_t pos) {
    UBool isBoundary = FALSE;
    UBool cIsWord    = FALSE;
    
    if (pos >= fLookLimit) {
        fHitEnd = TRUE;
    } else {
        
        
        UTEXT_SETNATIVEINDEX(fInputText, pos);
        UChar32  c = UTEXT_CURRENT32(fInputText);
        if (u_hasBinaryProperty(c, UCHAR_GRAPHEME_EXTEND) || u_charType(c) == U_FORMAT_CHAR) {
            
            return FALSE;
        }
        cIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(c);
    }
    
    
    
    UBool prevCIsWord = FALSE;
    for (;;) {
        if (UTEXT_GETNATIVEINDEX(fInputText) <= fLookStart) {
            break;
        }
        UChar32 prevChar = UTEXT_PREVIOUS32(fInputText);
        if (!(u_hasBinaryProperty(prevChar, UCHAR_GRAPHEME_EXTEND)
              || u_charType(prevChar) == U_FORMAT_CHAR)) {
            prevCIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(prevChar);
            break;
        }
    }
    isBoundary = cIsWord ^ prevCIsWord;
    return isBoundary;
}

UBool RegexMatcher::isChunkWordBoundary(int32_t pos) {
    UBool isBoundary = FALSE;
    UBool cIsWord    = FALSE;
    
    const UChar *inputBuf = fInputText->chunkContents;
    
    if (pos >= fLookLimit) {
        fHitEnd = TRUE;
    } else {
        
        
        UChar32 c;
        U16_GET(inputBuf, fLookStart, pos, fLookLimit, c);
        if (u_hasBinaryProperty(c, UCHAR_GRAPHEME_EXTEND) || u_charType(c) == U_FORMAT_CHAR) {
            
            return FALSE;
        }
        cIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(c);
    }
    
    
    
    UBool prevCIsWord = FALSE;
    for (;;) {
        if (pos <= fLookStart) {
            break;
        }
        UChar32 prevChar;
        U16_PREV(inputBuf, fLookStart, pos, prevChar);
        if (!(u_hasBinaryProperty(prevChar, UCHAR_GRAPHEME_EXTEND)
              || u_charType(prevChar) == U_FORMAT_CHAR)) {
            prevCIsWord = fPattern->fStaticSets[URX_ISWORD_SET]->contains(prevChar);
            break;
        }
    }
    isBoundary = cIsWord ^ prevCIsWord;
    return isBoundary;
}










UBool RegexMatcher::isUWordBoundary(int64_t pos) {
    UBool       returnVal = FALSE;
#if UCONFIG_NO_BREAK_ITERATION==0
    
    
    if (fWordBreakItr == NULL) {
        fWordBreakItr = 
            (RuleBasedBreakIterator *)BreakIterator::createWordInstance(Locale::getEnglish(), fDeferredStatus);
        if (U_FAILURE(fDeferredStatus)) {
            return FALSE;
        }
        fWordBreakItr->setText(fInputText, fDeferredStatus);
    }

    if (pos >= fLookLimit) {
        fHitEnd = TRUE;
        returnVal = TRUE;   
                            
                            
    } else {
        if (!UTEXT_USES_U16(fInputText)) {
            
            UErrorCode status = U_ZERO_ERROR;
            pos = utext_extract(fInputText, 0, pos, NULL, 0, &status);
        }
        returnVal = fWordBreakItr->isBoundary((int32_t)pos);
    }
#endif
    return   returnVal;
}












void RegexMatcher::IncrementTime(UErrorCode &status) {
    fTickCounter = TIMER_INITIAL_VALUE;
    fTime++;
    if (fCallbackFn != NULL) {
        if ((*fCallbackFn)(fCallbackContext, fTime) == FALSE) {
            status = U_REGEX_STOPPED_BY_CALLER;
            return;
        }
    }
    if (fTimeLimit > 0 && fTime >= fTimeLimit) {
        status = U_REGEX_TIME_OUT;
    }
}














UBool RegexMatcher::ReportFindProgress(int64_t matchIndex, UErrorCode &status) {
    if (fFindProgressCallbackFn != NULL) {
        if ((*fFindProgressCallbackFn)(fFindProgressCallbackContext, matchIndex) == FALSE) {
            status = U_ZERO_ERROR ;
            return FALSE;
        }
    }
    return TRUE;
}






















inline REStackFrame *RegexMatcher::StateSave(REStackFrame *fp, int64_t savePatIdx, UErrorCode &status) {
    
    int64_t *newFP = fStack->reserveBlock(fFrameSize, status);
    if (newFP == NULL) {
        
        
        
        status = U_REGEX_STACK_OVERFLOW;
        
        
        
        
        return fp;
    }
    fp = (REStackFrame *)(newFP - fFrameSize);  
    
    
    int64_t *source = (int64_t *)fp;
    int64_t *dest   = newFP;
    for (;;) {
        *dest++ = *source++;
        if (source == newFP) {
            break;
        }
    }
    
    fTickCounter--;
    if (fTickCounter <= 0) {
       IncrementTime(status);    
    }
    fp->fPatIdx = savePatIdx;
    return (REStackFrame *)newFP;
}










void RegexMatcher::MatchAt(int64_t startIdx, UBool toEnd, UErrorCode &status) {
    UBool       isMatch  = FALSE;      
    
    int64_t     backSearchIndex = U_INT64_MAX; 

    int32_t     op;                    
    int32_t     opType;                
    int32_t     opValue;               
        
    #ifdef REGEX_RUN_DEBUG
    if (fTraceDebug)
    {
        printf("MatchAt(startIdx=%ld)\n", startIdx);
        printf("Original Pattern: ");
        UChar32 c = utext_next32From(fPattern->fPattern, 0);
        while (c != U_SENTINEL) {
            if (c<32 || c>256) {
                c = '.';
            }
            REGEX_DUMP_DEBUG_PRINTF(("%c", c));
            
            c = UTEXT_NEXT32(fPattern->fPattern);
        }
        printf("\n");
        printf("Input String: ");
        c = utext_next32From(fInputText, 0);
        while (c != U_SENTINEL) {
            if (c<32 || c>256) {
                c = '.';
            }
            printf("%c", c);
            
            c = UTEXT_NEXT32(fInputText);
        }
        printf("\n");
        printf("\n");
    }
    #endif

    if (U_FAILURE(status)) {
        return;
    }

    
    
    int64_t             *pat           = fPattern->fCompiledPat->getBuffer();

    const UChar         *litText       = fPattern->fLiteralText.getBuffer();
    UVector             *sets          = fPattern->fSets;

    fFrameSize = fPattern->fFrameSize;
    REStackFrame        *fp            = resetStack();

    fp->fPatIdx   = 0;
    fp->fInputIdx = startIdx;

    
    int32_t i;
    for (i = 0; i<fPattern->fDataSize; i++) {
        fData[i] = 0;
    }

    
    
    
    
    for (;;) {
#if 0
        if (_heapchk() != _HEAPOK) {
            fprintf(stderr, "Heap Trouble\n");
        }
#endif
        
        op      = (int32_t)pat[fp->fPatIdx];
        opType  = URX_TYPE(op);
        opValue = URX_VAL(op);
        #ifdef REGEX_RUN_DEBUG
        if (fTraceDebug) {
            UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
            printf("inputIdx=%d   inputChar=%x   sp=%3d   activeLimit=%d  ", fp->fInputIdx,
                UTEXT_CURRENT32(fInputText), (int64_t *)fp-fStack->getBuffer(), fActiveLimit);
            fPattern->dumpOp(fp->fPatIdx);
        }
        #endif
        fp->fPatIdx++;
        
        switch (opType) {


        case URX_NOP:
            break;


        case URX_BACKTRACK:
            
            
            
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;


        case URX_ONECHAR:
            if (fp->fInputIdx < fActiveLimit) {
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (c == opValue) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    break;
                }
            } else {
                fHitEnd = TRUE;
            }
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;


        case URX_STRING:
            {
                
                
                

                int32_t   stringStartIdx = opValue;
                op      = (int32_t)pat[fp->fPatIdx];     
                fp->fPatIdx++;
                opType    = URX_TYPE(op);
                int32_t stringLen = URX_VAL(op);
                U_ASSERT(opType == URX_STRING_LEN);
                U_ASSERT(stringLen >= 2);
                                
                const UChar *patternString = litText+stringStartIdx;
                int32_t patternStringIndex = 0;
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 inputChar;
                UChar32 patternChar;
                UBool success = TRUE;
                while (patternStringIndex < stringLen) {
                    if (UTEXT_GETNATIVEINDEX(fInputText) >= fActiveLimit) {
                        success = FALSE;
                        fHitEnd = TRUE;
                        break;
                    }
                    inputChar = UTEXT_NEXT32(fInputText);
                    U16_NEXT(patternString, patternStringIndex, stringLen, patternChar);
                    if (patternChar != inputChar) {
                        success = FALSE;
                        break;
                    }
                }
                
                if (success) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;


        case URX_STATE_SAVE:
            fp = StateSave(fp, opValue, status);
            break;


        case URX_END:
            
            
            if (toEnd && fp->fInputIdx != fActiveLimit) {
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            }
            isMatch = TRUE;
            goto  breakFromLoop;

        
            
            
            
            
        case URX_START_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < fFrameSize-3);
            fp->fExtra[opValue+2] = fp->fInputIdx;
            break;


        case URX_END_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < fFrameSize-3);
            U_ASSERT(fp->fExtra[opValue+2] >= 0);            
            fp->fExtra[opValue]   = fp->fExtra[opValue+2];   
            fp->fExtra[opValue+1] = fp->fInputIdx;           
            U_ASSERT(fp->fExtra[opValue] <= fp->fExtra[opValue+1]);
            break;


        case URX_DOLLAR:                   
                                           
            {
                if (fp->fInputIdx >= fAnchorLimit) {
                    
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;
                }
                
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                
                
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (UTEXT_GETNATIVEINDEX(fInputText) >= fAnchorLimit) {
                    if ((c>=0x0a && c<=0x0d) || c==0x85 || c==0x2028 || c==0x2029) {
                        
                      if ( !(c==0x0a && fp->fInputIdx>fAnchorStart && ((void)UTEXT_PREVIOUS32(fInputText), UTEXT_PREVIOUS32(fInputText))==0x0d)) {
                            
                            fHitEnd = TRUE;
                            fRequireEnd = TRUE;
                            
                            break;
                        }
                    }
                } else {
                    UChar32 nextC = UTEXT_NEXT32(fInputText);
                    if (c == 0x0d && nextC == 0x0a && UTEXT_GETNATIVEINDEX(fInputText) >= fAnchorLimit) {
                        fHitEnd = TRUE;
                        fRequireEnd = TRUE;
                        break;                         
                    }
                }

                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


         case URX_DOLLAR_D:                   
            if (fp->fInputIdx >= fAnchorLimit) {
                
                fHitEnd = TRUE;
                fRequireEnd = TRUE;
                break;
            } else {
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 c = UTEXT_NEXT32(fInputText);
                
                if (c == 0x0a && UTEXT_GETNATIVEINDEX(fInputText) == fAnchorLimit) {
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;
                }
            }

            
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;


         case URX_DOLLAR_M:                
             {
                 if (fp->fInputIdx >= fAnchorLimit) {
                     
                     fHitEnd = TRUE;
                     fRequireEnd = TRUE;
                     break;
                 }
                 
                 
                 UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                 UChar32 c = UTEXT_CURRENT32(fInputText);
                 if ((c>=0x0a && c<=0x0d) || c==0x85 ||c==0x2028 || c==0x2029) {
                     
                     
                     
                     if ( !(c==0x0a && fp->fInputIdx>fAnchorStart && UTEXT_PREVIOUS32(fInputText)==0x0d)) {
                        break;
                     }
                 }
                 
                 fp = (REStackFrame *)fStack->popFrame(fFrameSize);
             }
             break;


         case URX_DOLLAR_MD:                
             {
                 if (fp->fInputIdx >= fAnchorLimit) {
                     
                     fHitEnd = TRUE;
                     fRequireEnd = TRUE;  
                     break;               
                 }
                 
                 
                 UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                 if (UTEXT_CURRENT32(fInputText) != 0x0a) {
                     fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                 }
             }
             break;


       case URX_CARET:                    
            if (fp->fInputIdx != fAnchorStart) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


       case URX_CARET_M:                   
           {
               if (fp->fInputIdx == fAnchorStart) {
                   
                   break;
               }
               
               
               UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
               UChar32  c = UTEXT_PREVIOUS32(fInputText); 
               if ((fp->fInputIdx < fAnchorLimit) && 
                   ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                   
                   
                   break;
               }
               
               fp = (REStackFrame *)fStack->popFrame(fFrameSize);
           }
           break;


       case URX_CARET_M_UNIX:       
           {
               U_ASSERT(fp->fInputIdx >= fAnchorStart);
               if (fp->fInputIdx <= fAnchorStart) {
                   
                   break;
               }
               
               U_ASSERT(fp->fInputIdx <= fAnchorLimit);
               UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
               UChar32  c = UTEXT_PREVIOUS32(fInputText);
               if (c != 0x0a) {
                   
                   fp = (REStackFrame *)fStack->popFrame(fFrameSize);
               }
           }
           break;

        case URX_BACKSLASH_B:          
            {
                UBool success = isWordBoundary(fp->fInputIdx);
                success ^= (UBool)(opValue != 0);     
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;


        case URX_BACKSLASH_BU:          
            {
                UBool success = isUWordBoundary(fp->fInputIdx);
                success ^= (UBool)(opValue != 0);     
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;


        case URX_BACKSLASH_D:            
            {
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);

                UChar32 c = UTEXT_NEXT32(fInputText);
                int8_t ctype = u_charType(c);     
                UBool success = (ctype == U_DECIMAL_DIGIT_NUMBER);
                success ^= (UBool)(opValue != 0);        
                if (success) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;


        case URX_BACKSLASH_G:          
            if (!((fMatch && fp->fInputIdx==fMatchEnd) || (fMatch==FALSE && fp->fInputIdx==fActiveStart))) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


        case URX_BACKSLASH_X:     
            
            
            
            {

                
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);

                
                
                UChar32  c;
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                UnicodeSet **sets = fPattern->fStaticSets;
                if (sets[URX_GC_NORMAL]->contains(c))  goto GC_Extend;
                if (sets[URX_GC_CONTROL]->contains(c)) goto GC_Control;
                if (sets[URX_GC_L]->contains(c))       goto GC_L;
                if (sets[URX_GC_LV]->contains(c))      goto GC_V;
                if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
                if (sets[URX_GC_V]->contains(c))       goto GC_V;
                if (sets[URX_GC_T]->contains(c))       goto GC_T;
                goto GC_Extend;



GC_L:
                if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                if (sets[URX_GC_L]->contains(c))       goto GC_L;
                if (sets[URX_GC_LV]->contains(c))      goto GC_V;
                if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
                if (sets[URX_GC_V]->contains(c))       goto GC_V;
                (void)UTEXT_PREVIOUS32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                goto GC_Extend;

GC_V:
                if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                if (sets[URX_GC_V]->contains(c))       goto GC_V;
                if (sets[URX_GC_T]->contains(c))       goto GC_T;
                (void)UTEXT_PREVIOUS32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                goto GC_Extend;

GC_T:
                if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                if (sets[URX_GC_T]->contains(c))       goto GC_T;
                (void)UTEXT_PREVIOUS32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                goto GC_Extend;

GC_Extend:
                
                for (;;) {
                    if (fp->fInputIdx >= fActiveLimit) {
                        break;
                    }
                    c = UTEXT_CURRENT32(fInputText);
                    if (sets[URX_GC_EXTEND]->contains(c) == FALSE) {
                        break;
                    }
                    (void)UTEXT_NEXT32(fInputText);
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                }
                goto GC_Done;

GC_Control:
                
                
                if (c == 0x0d && fp->fInputIdx < fActiveLimit && UTEXT_CURRENT32(fInputText) == 0x0a) {
                    c = UTEXT_NEXT32(fInputText);
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                }

GC_Done:
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                }
                break;
            }
            



        case URX_BACKSLASH_Z:          
            if (fp->fInputIdx < fAnchorLimit) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            } else {
                fHitEnd = TRUE;
                fRequireEnd = TRUE;
            }
            break;



        case URX_STATIC_SETREF:
            {
                
                
                
                
                
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                UBool success = ((opValue & URX_NEG_SET) == URX_NEG_SET);  
                opValue &= ~URX_NEG_SET;
                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);

                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c)) {
                        success = !success;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c)) {
                        success = !success;
                    }
                }
                if (success) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                } else {
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            

        case URX_STAT_SETREF_N:
            {
                
                
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);

                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c) == FALSE) {
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        break;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c) == FALSE) {
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        break;
                    }
                }
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            

        case URX_SETREF:
            if (fp->fInputIdx >= fActiveLimit) {
                fHitEnd = TRUE;
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            } else {
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                    
                
                UChar32 c = UTEXT_NEXT32(fInputText);
                U_ASSERT(opValue > 0 && opValue < sets->size());
                if (c<256) {
                    Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                    if (s8->contains(c)) {
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        break;
                    }
                } else {
                    UnicodeSet *s = (UnicodeSet *)sets->elementAt(opValue);
                    if (s->contains(c)) {
                        
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                        break;
                    }
                }
                
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


        case URX_DOTANY:
            {
                
                if (fp->fInputIdx >= fActiveLimit) {
                    
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (((c & 0x7f) <= 0x29) &&     
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                    
                        fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
            }
            break;


        case URX_DOTANY_ALL:
            {
                
                if (fp->fInputIdx >= fActiveLimit) {
                    
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                
                
                UChar32 c; 
                c = UTEXT_NEXT32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                if (c==0x0d && fp->fInputIdx < fActiveLimit) {
                    
                    UChar32 nextc = UTEXT_CURRENT32(fInputText);
                    if (nextc == 0x0a) {
                        (void)UTEXT_NEXT32(fInputText);
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }
            }
            break;


        case URX_DOTANY_UNIX:
            {
                
                
                if (fp->fInputIdx >= fActiveLimit) {
                    
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                
                
                UChar32 c = UTEXT_NEXT32(fInputText);
                if (c == 0x0a) {
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                } else {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                }
            }
            break;


        case URX_JMP:
            fp->fPatIdx = opValue;
            break;

        case URX_FAIL:
            isMatch = FALSE;
            goto breakFromLoop;

        case URX_JMP_SAV:
            U_ASSERT(opValue < fPattern->fCompiledPat->size());
            fp = StateSave(fp, fp->fPatIdx, status);       
            fp->fPatIdx = opValue;                         
            break;

        case URX_JMP_SAV_X:
            
            
            
            
            {
                U_ASSERT(opValue > 0 && opValue < fPattern->fCompiledPat->size());
                int32_t  stoOp = (int32_t)pat[opValue-1];
                U_ASSERT(URX_TYPE(stoOp) == URX_STO_INP_LOC);
                int32_t  frameLoc = URX_VAL(stoOp);
                U_ASSERT(frameLoc >= 0 && frameLoc < fFrameSize);
                int64_t prevInputIdx = fp->fExtra[frameLoc];
                U_ASSERT(prevInputIdx <= fp->fInputIdx);
                if (prevInputIdx < fp->fInputIdx) {
                    
                    fp = StateSave(fp, fp->fPatIdx, status);  
                    fp->fPatIdx = opValue;
                    fp->fExtra[frameLoc] = fp->fInputIdx;
                } 
                
                
            }
            break;

        case URX_CTR_INIT:
            {
                U_ASSERT(opValue >= 0 && opValue < fFrameSize-2);
                fp->fExtra[opValue] = 0;       

                
                
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = (int32_t)pat[instrOperandLoc+1];
                int32_t maxCount = (int32_t)pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);

                if (minCount == 0) {
                    fp = StateSave(fp, loopLoc+1, status);
                }
                if (maxCount == 0) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;

        case URX_CTR_LOOP:
            {
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = (int32_t)pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT);
                int64_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = (int32_t)pat[opValue+2];
                int32_t maxCount  = (int32_t)pat[opValue+3];
                
                
                
                
                
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);
                if ((uint64_t)*pCounter >= (uint32_t)maxCount) {
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }
                if (*pCounter >= minCount) {
                    fp = StateSave(fp, fp->fPatIdx, status);
                }
                fp->fPatIdx = opValue + 4;    
            }
            break;

        case URX_CTR_INIT_NG:
            {
                
                U_ASSERT(opValue >= 0 && opValue < fFrameSize-2);
                fp->fExtra[opValue] = 0;       

                
                
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = (int32_t)pat[instrOperandLoc+1];
                int32_t maxCount = (int32_t)pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);

                if (minCount == 0) {
                    if (maxCount != 0) {
                        fp = StateSave(fp, fp->fPatIdx, status);
                    }
                    fp->fPatIdx = loopLoc+1;   
                } 
            }
            break;

        case URX_CTR_LOOP_NG:
            {
                
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = (int32_t)pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT_NG);
                int64_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = (int32_t)pat[opValue+2];
                int32_t maxCount  = (int32_t)pat[opValue+3];
                
                
                
                
                
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);

                if ((uint64_t)*pCounter >= (uint32_t)maxCount) {
                    
                    
                    
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }

                if (*pCounter < minCount) {
                    
                    
                    fp->fPatIdx = opValue + 4;    
                } else {
                    
                    
                    
                    
                    fp = StateSave(fp, opValue + 4, status);
                }
            }
            break;

        case URX_STO_SP:
            U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
            fData[opValue] = fStack->size();
            break;

        case URX_LD_SP:
            {
                U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(newStackSize <= fStack->size());
                int64_t *newFP = fStack->getBuffer() + newStackSize - fFrameSize;
                if (newFP == (int64_t *)fp) {
                    break;
                }
                int32_t i;
                for (i=0; i<fFrameSize; i++) {
                    newFP[i] = ((int64_t *)fp)[i];
                }
                fp = (REStackFrame *)newFP;
                fStack->setSize(newStackSize);
            }
            break;

        case URX_BACKREF:
            {
                U_ASSERT(opValue < fFrameSize);
                int64_t groupStartIdx = fp->fExtra[opValue];
                int64_t groupEndIdx   = fp->fExtra[opValue+1];
                U_ASSERT(groupStartIdx <= groupEndIdx);
                if (groupStartIdx < 0) {
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   
                    break;
                }
                UTEXT_SETNATIVEINDEX(fAltInputText, groupStartIdx);
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);

                
                
                
                
                UBool success = TRUE;
                for (;;) {
                    if (utext_getNativeIndex(fAltInputText) >= groupEndIdx) {
                        success = TRUE;
                        break;
                    }
                    if (utext_getNativeIndex(fInputText) >= fActiveLimit) {
                        success = FALSE;
                        fHitEnd = TRUE;
                        break;
                    }
                    UChar32 captureGroupChar = utext_next32(fAltInputText);
                    UChar32 inputChar = utext_next32(fInputText);
                    if (inputChar != captureGroupChar) {
                        success = FALSE;
                        break;
                    }
                }

                if (success) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;



        case URX_BACKREF_I:
            {
                U_ASSERT(opValue < fFrameSize);
                int64_t groupStartIdx = fp->fExtra[opValue];
                int64_t groupEndIdx   = fp->fExtra[opValue+1];
                U_ASSERT(groupStartIdx <= groupEndIdx);
                if (groupStartIdx < 0) {
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   
                    break;
                }
                utext_setNativeIndex(fAltInputText, groupStartIdx);
                utext_setNativeIndex(fInputText, fp->fInputIdx);
                CaseFoldingUTextIterator captureGroupItr(*fAltInputText);
                CaseFoldingUTextIterator inputItr(*fInputText);

                
                
                
                
                UBool success = TRUE;
                for (;;) {
                    if (!captureGroupItr.inExpansion() && utext_getNativeIndex(fAltInputText) >= groupEndIdx) {
                        success = TRUE;
                        break;
                    }
                    if (!inputItr.inExpansion() && utext_getNativeIndex(fInputText) >= fActiveLimit) {
                        success = FALSE;
                        fHitEnd = TRUE;
                        break;
                    }
                    UChar32 captureGroupChar = captureGroupItr.next();
                    UChar32 inputChar = inputItr.next();
                    if (inputChar != captureGroupChar) {
                        success = FALSE;
                        break;
                    }
                }

                if (success && inputItr.inExpansion()) {
                    
                    
                    
                    success = FALSE;
                }

                if (success) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
 
            }
            break;
                
        case URX_STO_INP_LOC:
            {
                U_ASSERT(opValue >= 0 && opValue < fFrameSize);
                fp->fExtra[opValue] = fp->fInputIdx;
            }
            break;

        case URX_JMPX:
            {
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 1;
                int32_t dataLoc  = URX_VAL(pat[instrOperandLoc]);
                U_ASSERT(dataLoc >= 0 && dataLoc < fFrameSize);
                int64_t savedInputIdx = fp->fExtra[dataLoc];
                U_ASSERT(savedInputIdx <= fp->fInputIdx);
                if (savedInputIdx < fp->fInputIdx) {
                    fp->fPatIdx = opValue;                               
                } else {
                     fp = (REStackFrame *)fStack->popFrame(fFrameSize);   
                }
            }
            break;

        case URX_LA_START:
            {
                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
                fActiveStart     = fLookStart;          
                fActiveLimit     = fLookLimit;          
            }
            break;

        case URX_LA_END:
            {
                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t stackSize = fStack->size();
                int32_t newStackSize =(int32_t)fData[opValue];
                U_ASSERT(stackSize >= newStackSize);
                if (stackSize > newStackSize) {
                    
                    
                    
                    int64_t *newFP = fStack->getBuffer() + newStackSize - fFrameSize;
                    int32_t i;
                    for (i=0; i<fFrameSize; i++) {
                        newFP[i] = ((int64_t *)fp)[i];
                    }
                    fp = (REStackFrame *)newFP;
                    fStack->setSize(newStackSize);
                }
                fp->fInputIdx = fData[opValue+1];

                
                
                fActiveStart = fRegionStart;
                fActiveLimit = fRegionLimit;
            }
            break;

        case URX_ONECHAR_I:
            
            
            
            if (fp->fInputIdx < fActiveLimit) {
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);

                UChar32 c = UTEXT_NEXT32(fInputText);
                if (u_foldCase(c, U_FOLD_CASE_DEFAULT) == opValue) {
                    fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    break;
                }
            } else {
                fHitEnd = TRUE;
            }
            
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;

        case URX_STRING_I:
            {
                
                
                
                
                {
                    const UChar *patternString = litText + opValue;
                    int32_t      patternStringIdx  = 0;

                    op      = (int32_t)pat[fp->fPatIdx];
                    fp->fPatIdx++;
                    opType  = URX_TYPE(op);
                    opValue = URX_VAL(op);
                    U_ASSERT(opType == URX_STRING_LEN);
                    int32_t patternStringLen = opValue;  
                
                    
                    UChar32   cPattern;
                    UChar32   cText;
                    UBool     success = TRUE;

                    UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                    CaseFoldingUTextIterator inputIterator(*fInputText);
                    while (patternStringIdx < patternStringLen) {
                        if (!inputIterator.inExpansion() && UTEXT_GETNATIVEINDEX(fInputText) >= fActiveLimit) {
                            success = FALSE;
                            fHitEnd = TRUE;
                            break;
                        }
                        U16_NEXT(patternString, patternStringIdx, patternStringLen, cPattern);
                        cText = inputIterator.next();
                        if (cText != cPattern) {
                            success = FALSE;
                            break;
                        }
                    }
                    if (inputIterator.inExpansion()) {
                        success = FALSE;
                    }

                    if (success) {
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    } else {
                        fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    }
                }
            }
            break;

        case URX_LB_START:
            {
                
                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
                
                fData[opValue+2] = -1;
                
                
                fData[opValue+3] = fActiveLimit;
                fActiveLimit     = fp->fInputIdx;
            }
            break;


        case URX_LB_CONT:
            {
                
                

                
                
                int32_t minML = (int32_t)pat[fp->fPatIdx++];
                int32_t maxML = (int32_t)pat[fp->fPatIdx++];
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);

                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int64_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    
                    
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;
                    } else {
                        UTEXT_SETNATIVEINDEX(fInputText, *lbStartIdx);
                        (void)UTEXT_PREVIOUS32(fInputText);
                        *lbStartIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }

                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    
                    
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    int64_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= fActiveLimit);
                    U_ASSERT(restoreInputLen <= fInputLength);
                    fActiveLimit = restoreInputLen;
                    break;
                }

                
                
                fp = StateSave(fp, fp->fPatIdx-3, status);
                fp->fInputIdx = *lbStartIdx;
            }
            break;

        case URX_LB_END:
            
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != fActiveLimit) {
                    
                    
                    
                    
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                
                
                
                int64_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= fActiveLimit);
                U_ASSERT(originalInputLen <= fInputLength);
                fActiveLimit = originalInputLen;
            }
            break;


        case URX_LBN_CONT:
            {
                
                

                
                int32_t minML       = (int32_t)pat[fp->fPatIdx++];
                int32_t maxML       = (int32_t)pat[fp->fPatIdx++];
                int32_t continueLoc = (int32_t)pat[fp->fPatIdx++];
                        continueLoc = URX_VAL(continueLoc);
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);
                U_ASSERT(continueLoc > fp->fPatIdx);

                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int64_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    
                    
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;
                    } else {
                        UTEXT_SETNATIVEINDEX(fInputText, *lbStartIdx);
                        (void)UTEXT_PREVIOUS32(fInputText);
                        *lbStartIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }

                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    
                    
                    
                    int64_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= fActiveLimit);
                    U_ASSERT(restoreInputLen <= fInputLength);
                    fActiveLimit = restoreInputLen;
                    fp->fPatIdx = continueLoc;
                    break;
                }

                
                
                fp = StateSave(fp, fp->fPatIdx-4, status);
                fp->fInputIdx = *lbStartIdx;
            }
            break;

        case URX_LBN_END:
            
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != fActiveLimit) {
                    
                    
                    
                    
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }

                
                
                
                
                
                
                int64_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= fActiveLimit);
                U_ASSERT(originalInputLen <= fInputLength);
                fActiveLimit = originalInputLen;

                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(fStack->size() > newStackSize);
                fStack->setSize(newStackSize);
                
                
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;


        case URX_LOOP_SR_I:
            
            
            
            
            {
                U_ASSERT(opValue > 0 && opValue < sets->size());
                Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                UnicodeSet   *s  = (UnicodeSet *)sets->elementAt(opValue);

                
                
                int64_t ix = fp->fInputIdx;
                UTEXT_SETNATIVEINDEX(fInputText, ix);
                for (;;) {
                    if (ix >= fActiveLimit) {
                        fHitEnd = TRUE;
                        break;
                    }
                    UChar32 c = UTEXT_NEXT32(fInputText);
                    if (c<256) {
                        if (s8->contains(c) == FALSE) {
                            break;
                        }
                    } else {
                        if (s->contains(c) == FALSE) {
                            break;
                        }
                    }
                    ix = UTEXT_GETNATIVEINDEX(fInputText);
                }

                
                
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   
                    break;
                }

                
                
                
                int32_t loopcOp = (int32_t)pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < fFrameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                fp->fInputIdx = ix;

                
                
                
                fp = StateSave(fp, fp->fPatIdx, status);
                fp->fPatIdx++;
            }
            break;


        case URX_LOOP_DOT_I:
            
            
            
            {
                
                
                int64_t ix;
                if ((opValue & 1) == 1) {
                    
                    ix = fActiveLimit;
                    fHitEnd = TRUE;
                } else {
                    
                    
                    ix = fp->fInputIdx;
                    UTEXT_SETNATIVEINDEX(fInputText, ix);
                    for (;;) {
                        if (ix >= fActiveLimit) {
                            fHitEnd = TRUE;
                            break;
                        }
                        UChar32 c = UTEXT_NEXT32(fInputText);
                        if ((c & 0x7f) <= 0x29) {          
                            if ((c == 0x0a) ||             
                               (((opValue & 2) == 0) &&    
                                    (c<=0x0d && c>=0x0a)) || c==0x85 ||c==0x2028 || c==0x2029) {
                                
                                break;
                            }
                        }
                        ix = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }

                
                
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   
                    break;
                }

                
                
                
                int32_t loopcOp = (int32_t)pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < fFrameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                fp->fInputIdx = ix;

                
                
                
                fp = StateSave(fp, fp->fPatIdx, status);
                fp->fPatIdx++;
            }
            break;


        case URX_LOOP_C:
            {
                U_ASSERT(opValue>=0 && opValue<fFrameSize);
                backSearchIndex = fp->fExtra[opValue];
                U_ASSERT(backSearchIndex <= fp->fInputIdx);
                if (backSearchIndex == fp->fInputIdx) {
                    
                    
                    
                    break;
                }
                
                
                
                
                
                U_ASSERT(fp->fInputIdx > 0);
                UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
                UChar32 prevC = UTEXT_PREVIOUS32(fInputText);
                fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                
                UChar32 twoPrevC = UTEXT_PREVIOUS32(fInputText);
                if (prevC == 0x0a && 
                    fp->fInputIdx > backSearchIndex &&
                    twoPrevC == 0x0d) {
                    int32_t prevOp = (int32_t)pat[fp->fPatIdx-2];
                    if (URX_TYPE(prevOp) == URX_LOOP_DOT_I) {
                        
                        fp->fInputIdx = UTEXT_GETNATIVEINDEX(fInputText);
                    }
                }


                fp = StateSave(fp, fp->fPatIdx-1, status);
            }
            break;



        default:
            
            
            U_ASSERT(FALSE);
        }

        if (U_FAILURE(status)) {
            isMatch = FALSE;
            break;
        }
    }
    
breakFromLoop:
    fMatch = isMatch;
    if (isMatch) {
        fLastMatchEnd = fMatchEnd;
        fMatchStart   = startIdx;
        fMatchEnd     = fp->fInputIdx;
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("Match.  start=%d   end=%d\n\n", fMatchStart, fMatchEnd));
        }
    }
    else
    {
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("No match\n\n"));
        }
    }

    fFrame = fp;                
                                
                                
    return;
}














void RegexMatcher::MatchChunkAt(int32_t startIdx, UBool toEnd, UErrorCode &status) {
    UBool       isMatch  = FALSE;      
    
    int32_t     backSearchIndex = INT32_MAX; 

    int32_t     op;                    
    int32_t     opType;                
    int32_t     opValue;               
    
#ifdef REGEX_RUN_DEBUG
    if (fTraceDebug)
    {
        printf("MatchAt(startIdx=%ld)\n", startIdx);
        printf("Original Pattern: ");
        UChar32 c = utext_next32From(fPattern->fPattern, 0);
        while (c != U_SENTINEL) {
            if (c<32 || c>256) {
                c = '.';
            }
            REGEX_DUMP_DEBUG_PRINTF(("%c", c));
            
            c = UTEXT_NEXT32(fPattern->fPattern);
        }
        printf("\n");
        printf("Input String: ");
        c = utext_next32From(fInputText, 0);
        while (c != U_SENTINEL) {
            if (c<32 || c>256) {
                c = '.';
            }
            printf("%c", c);
            
            c = UTEXT_NEXT32(fInputText);
        }
        printf("\n");
        printf("\n");
    }
#endif
    
    if (U_FAILURE(status)) {
        return;
    }
    
    
    
    int64_t             *pat           = fPattern->fCompiledPat->getBuffer();
    
    const UChar         *litText       = fPattern->fLiteralText.getBuffer();
    UVector             *sets          = fPattern->fSets;
    
    const UChar         *inputBuf      = fInputText->chunkContents;
    
    fFrameSize = fPattern->fFrameSize;
    REStackFrame        *fp            = resetStack();
    
    fp->fPatIdx   = 0;
    fp->fInputIdx = startIdx;
    
    
    int32_t i;
    for (i = 0; i<fPattern->fDataSize; i++) {
        fData[i] = 0;
    }
    
    
    
    
    
    for (;;) {
#if 0
        if (_heapchk() != _HEAPOK) {
            fprintf(stderr, "Heap Trouble\n");
        }
#endif
        
        op      = (int32_t)pat[fp->fPatIdx];
        opType  = URX_TYPE(op);
        opValue = URX_VAL(op);
#ifdef REGEX_RUN_DEBUG
        if (fTraceDebug) {
            UTEXT_SETNATIVEINDEX(fInputText, fp->fInputIdx);
            printf("inputIdx=%d   inputChar=%x   sp=%3d   activeLimit=%d  ", fp->fInputIdx,
                   UTEXT_CURRENT32(fInputText), (int64_t *)fp-fStack->getBuffer(), fActiveLimit);
            fPattern->dumpOp(fp->fPatIdx);
        }
#endif
        fp->fPatIdx++;
        
        switch (opType) {
                
                
        case URX_NOP:
            break;
            
            
        case URX_BACKTRACK:
            
            
            
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;
            
            
        case URX_ONECHAR:
            if (fp->fInputIdx < fActiveLimit) {
                UChar32 c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c == opValue) {
                    break;
                }
            } else {
                fHitEnd = TRUE;
            }
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;
            
            
        case URX_STRING:
            {
                
                
                
                int32_t   stringStartIdx = opValue;
                int32_t   stringLen;
                
                op      = (int32_t)pat[fp->fPatIdx];     
                fp->fPatIdx++;
                opType    = URX_TYPE(op);
                stringLen = URX_VAL(op);
                U_ASSERT(opType == URX_STRING_LEN);
                U_ASSERT(stringLen >= 2);
                
                const UChar * pInp = inputBuf + fp->fInputIdx;
                const UChar * pInpLimit = inputBuf + fActiveLimit;
                const UChar * pPat = litText+stringStartIdx;
                const UChar * pEnd = pInp + stringLen;
                UBool success = TRUE;
                while (pInp < pEnd) {
                    if (pInp >= pInpLimit) {
                        fHitEnd = TRUE;
                        success = FALSE;
                        break;
                    }
                    if (*pInp++ != *pPat++) {
                        success = FALSE;
                        break;
                    }
                }
                
                if (success) {
                    fp->fInputIdx += stringLen;
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_STATE_SAVE:
            fp = StateSave(fp, opValue, status);
            break;
            
            
        case URX_END:
            
            
            if (toEnd && fp->fInputIdx != fActiveLimit) {
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            }
            isMatch = TRUE;
            goto  breakFromLoop;
            
            
            
            
            
            
        case URX_START_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < fFrameSize-3);
            fp->fExtra[opValue+2] = fp->fInputIdx;
            break;
            
            
        case URX_END_CAPTURE:
            U_ASSERT(opValue >= 0 && opValue < fFrameSize-3);
            U_ASSERT(fp->fExtra[opValue+2] >= 0);            
            fp->fExtra[opValue]   = fp->fExtra[opValue+2];   
            fp->fExtra[opValue+1] = fp->fInputIdx;           
            U_ASSERT(fp->fExtra[opValue] <= fp->fExtra[opValue+1]);
            break;
            
            
        case URX_DOLLAR:                   
            
            if (fp->fInputIdx < fAnchorLimit-2) {
                
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            }
            if (fp->fInputIdx >= fAnchorLimit) {
                
                fHitEnd = TRUE;
                fRequireEnd = TRUE;
                break;
            }
            
            
            
            if (fp->fInputIdx == fAnchorLimit-1) {
                UChar32 c;
                U16_GET(inputBuf, fAnchorStart, fp->fInputIdx, fAnchorLimit, c);
                
                if ((c>=0x0a && c<=0x0d) || c==0x85 || c==0x2028 || c==0x2029) {
                    if ( !(c==0x0a && fp->fInputIdx>fAnchorStart && inputBuf[fp->fInputIdx-1]==0x0d)) {
                        
                        fHitEnd = TRUE;
                        fRequireEnd = TRUE;
                        break;
                    }
                }
            } else if (fp->fInputIdx == fAnchorLimit-2 &&
                inputBuf[fp->fInputIdx]==0x0d && inputBuf[fp->fInputIdx+1]==0x0a) {
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;                         
            }
            
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            
            break;
            
            
        case URX_DOLLAR_D:                   
            if (fp->fInputIdx >= fAnchorLimit-1) {
                
                if (fp->fInputIdx == fAnchorLimit-1) {
                    
                    if (inputBuf[fp->fInputIdx] == 0x0a) {
                        fHitEnd = TRUE;
                        fRequireEnd = TRUE;
                        break;
                    }
                } else {
                    
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;
                }
            }
            
            
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;
            
            
        case URX_DOLLAR_M:                
            {
                if (fp->fInputIdx >= fAnchorLimit) {
                    
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;
                    break;
                }
                
                
                UChar32 c = inputBuf[fp->fInputIdx];
                if ((c>=0x0a && c<=0x0d) || c==0x85 ||c==0x2028 || c==0x2029) {
                    
                    
                    
                    if ( !(c==0x0a && fp->fInputIdx>fAnchorStart && inputBuf[fp->fInputIdx-1]==0x0d)) {
                        break;
                    }
                }
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_DOLLAR_MD:                
            {
                if (fp->fInputIdx >= fAnchorLimit) {
                    
                    fHitEnd = TRUE;
                    fRequireEnd = TRUE;  
                    break;               
                }
                
                
                if (inputBuf[fp->fInputIdx] != 0x0a) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_CARET:                    
            if (fp->fInputIdx != fAnchorStart) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_CARET_M:                   
            {
                if (fp->fInputIdx == fAnchorStart) {
                    
                    break;
                }
                
                
                UChar  c = inputBuf[fp->fInputIdx - 1]; 
                if ((fp->fInputIdx < fAnchorLimit) && 
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                    
                    
                    break;
                }
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_CARET_M_UNIX:       
            {
                U_ASSERT(fp->fInputIdx >= fAnchorStart);
                if (fp->fInputIdx <= fAnchorStart) {
                    
                    break;
                }
                
                U_ASSERT(fp->fInputIdx <= fAnchorLimit);
                UChar  c = inputBuf[fp->fInputIdx - 1]; 
                if (c != 0x0a) {
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
        case URX_BACKSLASH_B:          
            {
                UBool success = isChunkWordBoundary((int32_t)fp->fInputIdx);
                success ^= (UBool)(opValue != 0);     
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_BACKSLASH_BU:          
            {
                UBool success = isUWordBoundary(fp->fInputIdx);
                success ^= (UBool)(opValue != 0);     
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_BACKSLASH_D:            
            {
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UChar32 c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                int8_t ctype = u_charType(c);     
                UBool success = (ctype == U_DECIMAL_DIGIT_NUMBER);
                success ^= (UBool)(opValue != 0);        
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_BACKSLASH_G:          
            if (!((fMatch && fp->fInputIdx==fMatchEnd) || (fMatch==FALSE && fp->fInputIdx==fActiveStart))) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_BACKSLASH_X:     
        
        
        
        {

            
            if (fp->fInputIdx >= fActiveLimit) {
                fHitEnd = TRUE;
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                break;
            }

            
            
            UChar32  c;
            U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
            UnicodeSet **sets = fPattern->fStaticSets;
            if (sets[URX_GC_NORMAL]->contains(c))  goto GC_Extend;
            if (sets[URX_GC_CONTROL]->contains(c)) goto GC_Control;
            if (sets[URX_GC_L]->contains(c))       goto GC_L;
            if (sets[URX_GC_LV]->contains(c))      goto GC_V;
            if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
            if (sets[URX_GC_V]->contains(c))       goto GC_V;
            if (sets[URX_GC_T]->contains(c))       goto GC_T;
            goto GC_Extend;



GC_L:
            if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
            U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
            if (sets[URX_GC_L]->contains(c))       goto GC_L;
            if (sets[URX_GC_LV]->contains(c))      goto GC_V;
            if (sets[URX_GC_LVT]->contains(c))     goto GC_T;
            if (sets[URX_GC_V]->contains(c))       goto GC_V;
            U16_PREV(inputBuf, 0, fp->fInputIdx, c);
            goto GC_Extend;

GC_V:
            if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
            U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
            if (sets[URX_GC_V]->contains(c))       goto GC_V;
            if (sets[URX_GC_T]->contains(c))       goto GC_T;
            U16_PREV(inputBuf, 0, fp->fInputIdx, c);
            goto GC_Extend;

GC_T:
            if (fp->fInputIdx >= fActiveLimit)         goto GC_Done;
            U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
            if (sets[URX_GC_T]->contains(c))       goto GC_T;
            U16_PREV(inputBuf, 0, fp->fInputIdx, c);
            goto GC_Extend;

GC_Extend:
            
            for (;;) {
                if (fp->fInputIdx >= fActiveLimit) {
                    break;
                }
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (sets[URX_GC_EXTEND]->contains(c) == FALSE) {
                    U16_BACK_1(inputBuf, 0, fp->fInputIdx);
                    break;
                }
            }
            goto GC_Done;

GC_Control:
            
            
            if (c == 0x0d && fp->fInputIdx < fActiveLimit && inputBuf[fp->fInputIdx] == 0x0a) {
                fp->fInputIdx++;
            }

GC_Done:
            if (fp->fInputIdx >= fActiveLimit) {
                fHitEnd = TRUE;
            }
            break;
        }
            
            
            
            
        case URX_BACKSLASH_Z:          
            if (fp->fInputIdx < fAnchorLimit) {
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            } else {
                fHitEnd = TRUE;
                fRequireEnd = TRUE;
            }
            break;
            
            
            
        case URX_STATIC_SETREF:
            {
                
                
                
                
                
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                UBool success = ((opValue & URX_NEG_SET) == URX_NEG_SET);  
                opValue &= ~URX_NEG_SET;
                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);
                
                UChar32 c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c)) {
                        success = !success;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c)) {
                        success = !success;
                    }
                }
                if (!success) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_STAT_SETREF_N:
            {
                
                
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                U_ASSERT(opValue > 0 && opValue < URX_LAST_SET);
                
                UChar32  c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c < 256) {
                    Regex8BitSet *s8 = &fPattern->fStaticSets8[opValue];
                    if (s8->contains(c) == FALSE) {
                        break;
                    }
                } else {
                    const UnicodeSet *s = fPattern->fStaticSets[opValue];
                    if (s->contains(c) == FALSE) {
                        break;
                    }
                }
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_SETREF:
            {
                if (fp->fInputIdx >= fActiveLimit) {
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                U_ASSERT(opValue > 0 && opValue < sets->size());

                
                UChar32  c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c<256) {
                    Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                    if (s8->contains(c)) {
                        
                        break;
                    }
                } else {
                    UnicodeSet *s = (UnicodeSet *)sets->elementAt(opValue);
                    if (s->contains(c)) {
                        
                        break;
                    }
                }
                
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_DOTANY:
            {
                
                if (fp->fInputIdx >= fActiveLimit) {
                    
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                
                UChar32  c;
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (((c & 0x7f) <= 0x29) &&     
                    ((c<=0x0d && c>=0x0a) || c==0x85 ||c==0x2028 || c==0x2029)) {
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
            }
            break;
            
            
        case URX_DOTANY_ALL:
            {
                
                if (fp->fInputIdx >= fActiveLimit) {
                    
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                
                
                UChar32 c; 
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c==0x0d && fp->fInputIdx < fActiveLimit) {
                    
                    if (inputBuf[fp->fInputIdx] == 0x0a) {
                        U16_FWD_1(inputBuf, fp->fInputIdx, fActiveLimit);
                    }
                }
            }
            break;
            
            
        case URX_DOTANY_UNIX:
            {
                
                
                if (fp->fInputIdx >= fActiveLimit) {
                    
                    fHitEnd = TRUE;
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                
                UChar32 c; 
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (c == 0x0a) {
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
            
        case URX_JMP:
            fp->fPatIdx = opValue;
            break;
            
        case URX_FAIL:
            isMatch = FALSE;
            goto breakFromLoop;
            
        case URX_JMP_SAV:
            U_ASSERT(opValue < fPattern->fCompiledPat->size());
            fp = StateSave(fp, fp->fPatIdx, status);       
            fp->fPatIdx = opValue;                         
            break;
            
        case URX_JMP_SAV_X:
            
            
            
            
            {
                U_ASSERT(opValue > 0 && opValue < fPattern->fCompiledPat->size());
                int32_t  stoOp = (int32_t)pat[opValue-1];
                U_ASSERT(URX_TYPE(stoOp) == URX_STO_INP_LOC);
                int32_t  frameLoc = URX_VAL(stoOp);
                U_ASSERT(frameLoc >= 0 && frameLoc < fFrameSize);
                int32_t prevInputIdx = (int32_t)fp->fExtra[frameLoc];
                U_ASSERT(prevInputIdx <= fp->fInputIdx);
                if (prevInputIdx < fp->fInputIdx) {
                    
                    fp = StateSave(fp, fp->fPatIdx, status);  
                    fp->fPatIdx = opValue;
                    fp->fExtra[frameLoc] = fp->fInputIdx;
                } 
                
                
            }
            break;
            
        case URX_CTR_INIT:
            {
                U_ASSERT(opValue >= 0 && opValue < fFrameSize-2);
                fp->fExtra[opValue] = 0;       
                
                
                
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = (int32_t)pat[instrOperandLoc+1];
                int32_t maxCount = (int32_t)pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);
                
                if (minCount == 0) {
                    fp = StateSave(fp, loopLoc+1, status);
                }
                if (maxCount == 0) {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
        case URX_CTR_LOOP:
            {
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = (int32_t)pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT);
                int64_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = (int32_t)pat[opValue+2];
                int32_t maxCount  = (int32_t)pat[opValue+3];
                
                
                
                
                
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);
                if ((uint64_t)*pCounter >= (uint32_t)maxCount) {
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }
                if (*pCounter >= minCount) {
                    fp = StateSave(fp, fp->fPatIdx, status);
                }
                fp->fPatIdx = opValue + 4;    
            }
            break;
            
        case URX_CTR_INIT_NG:
            {
                
                U_ASSERT(opValue >= 0 && opValue < fFrameSize-2);
                fp->fExtra[opValue] = 0;       
                
                
                
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 3;
                int32_t loopLoc  = URX_VAL(pat[instrOperandLoc]);
                int32_t minCount = (int32_t)pat[instrOperandLoc+1];
                int32_t maxCount = (int32_t)pat[instrOperandLoc+2];
                U_ASSERT(minCount>=0);
                U_ASSERT(maxCount>=minCount || maxCount==-1);
                U_ASSERT(loopLoc>fp->fPatIdx);
                
                if (minCount == 0) {
                    if (maxCount != 0) {
                        fp = StateSave(fp, fp->fPatIdx, status);
                    }
                    fp->fPatIdx = loopLoc+1;   
                } 
            }
            break;
            
        case URX_CTR_LOOP_NG:
            {
                
                U_ASSERT(opValue>0 && opValue < fp->fPatIdx-2);
                int32_t initOp = (int32_t)pat[opValue];
                U_ASSERT(URX_TYPE(initOp) == URX_CTR_INIT_NG);
                int64_t *pCounter = &fp->fExtra[URX_VAL(initOp)];
                int32_t minCount  = (int32_t)pat[opValue+2];
                int32_t maxCount  = (int32_t)pat[opValue+3];
                
                
                
                
                
                (*pCounter)++;
                U_ASSERT(*pCounter > 0);
                
                if ((uint64_t)*pCounter >= (uint32_t)maxCount) {
                    
                    
                    
                    U_ASSERT(*pCounter == maxCount || maxCount == -1);
                    break;
                }
                
                if (*pCounter < minCount) {
                    
                    
                    fp->fPatIdx = opValue + 4;    
                } else {
                    
                    
                    
                    
                    fp = StateSave(fp, opValue + 4, status);
                }
            }
            break;
            
        case URX_STO_SP:
            U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
            fData[opValue] = fStack->size();
            break;
            
        case URX_LD_SP:
            {
                U_ASSERT(opValue >= 0 && opValue < fPattern->fDataSize);
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(newStackSize <= fStack->size());
                int64_t *newFP = fStack->getBuffer() + newStackSize - fFrameSize;
                if (newFP == (int64_t *)fp) {
                    break;
                }
                int32_t i;
                for (i=0; i<fFrameSize; i++) {
                    newFP[i] = ((int64_t *)fp)[i];
                }
                fp = (REStackFrame *)newFP;
                fStack->setSize(newStackSize);
            }
            break;
            
        case URX_BACKREF:
            {
                U_ASSERT(opValue < fFrameSize);
                int64_t groupStartIdx = fp->fExtra[opValue];
                int64_t groupEndIdx   = fp->fExtra[opValue+1];
                U_ASSERT(groupStartIdx <= groupEndIdx);
                int64_t inputIndex = fp->fInputIdx;
                if (groupStartIdx < 0) {
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   
                    break;
                }
                UBool success = TRUE;
                for (int64_t groupIndex = groupStartIdx; groupIndex < groupEndIdx; ++groupIndex,++inputIndex) {
                    if (inputIndex >= fActiveLimit) {
                        success = FALSE;
                        fHitEnd = TRUE;
                        break;
                    }
                    if (inputBuf[groupIndex] != inputBuf[inputIndex]) {
                        success = FALSE;
                        break;
                    }
                }
                if (success) {
                    fp->fInputIdx = inputIndex;
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;
            
        case URX_BACKREF_I:
            {
                U_ASSERT(opValue < fFrameSize);
                int64_t groupStartIdx = fp->fExtra[opValue];
                int64_t groupEndIdx   = fp->fExtra[opValue+1];
                U_ASSERT(groupStartIdx <= groupEndIdx);
                if (groupStartIdx < 0) {
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   
                    break;
                }
                CaseFoldingUCharIterator captureGroupItr(inputBuf, groupStartIdx, groupEndIdx);
                CaseFoldingUCharIterator inputItr(inputBuf, fp->fInputIdx, fActiveLimit);

                
                
                
                
                UBool success = TRUE;
                for (;;) {
                    UChar32 captureGroupChar = captureGroupItr.next();
                    if (captureGroupChar == U_SENTINEL) {
                        success = TRUE;
                        break;
                    }
                    UChar32 inputChar = inputItr.next();
                    if (inputChar == U_SENTINEL) {
                        success = FALSE;
                        fHitEnd = TRUE;
                        break;
                    }
                    if (inputChar != captureGroupChar) {
                        success = FALSE;
                        break;
                    }
                }

                if (success && inputItr.inExpansion()) {
                    
                    
                    
                    success = FALSE;
                }

                if (success) {
                    fp->fInputIdx = inputItr.getIndex();
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;

        case URX_STO_INP_LOC:
            {
                U_ASSERT(opValue >= 0 && opValue < fFrameSize);
                fp->fExtra[opValue] = fp->fInputIdx;
            }
            break;
            
        case URX_JMPX:
            {
                int32_t instrOperandLoc = (int32_t)fp->fPatIdx;
                fp->fPatIdx += 1;
                int32_t dataLoc  = URX_VAL(pat[instrOperandLoc]);
                U_ASSERT(dataLoc >= 0 && dataLoc < fFrameSize);
                int32_t savedInputIdx = (int32_t)fp->fExtra[dataLoc];
                U_ASSERT(savedInputIdx <= fp->fInputIdx);
                if (savedInputIdx < fp->fInputIdx) {
                    fp->fPatIdx = opValue;                               
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);   
                }
            }
            break;
            
        case URX_LA_START:
            {
                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
                fActiveStart     = fLookStart;          
                fActiveLimit     = fLookLimit;          
            }
            break;
            
        case URX_LA_END:
            {
                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t stackSize = fStack->size();
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(stackSize >= newStackSize);
                if (stackSize > newStackSize) {
                    
                    
                    
                    int64_t *newFP = fStack->getBuffer() + newStackSize - fFrameSize;
                    int32_t i;
                    for (i=0; i<fFrameSize; i++) {
                        newFP[i] = ((int64_t *)fp)[i];
                    }
                    fp = (REStackFrame *)newFP;
                    fStack->setSize(newStackSize);
                }
                fp->fInputIdx = fData[opValue+1];
                
                
                
                fActiveStart = fRegionStart;
                fActiveLimit = fRegionLimit;
            }
            break;
            
        case URX_ONECHAR_I:
            if (fp->fInputIdx < fActiveLimit) {
                UChar32 c; 
                U16_NEXT(inputBuf, fp->fInputIdx, fActiveLimit, c);
                if (u_foldCase(c, U_FOLD_CASE_DEFAULT) == opValue) {
                    break;
                }
            } else {
                fHitEnd = TRUE;
            }
            fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            break;
            
        case URX_STRING_I:
            
            
            
            
            {
                const UChar *patternString = litText + opValue;

                op      = (int32_t)pat[fp->fPatIdx];
                fp->fPatIdx++;
                opType  = URX_TYPE(op);
                opValue = URX_VAL(op);
                U_ASSERT(opType == URX_STRING_LEN);
                int32_t patternStringLen = opValue;  
            
                UChar32      cText;
                UChar32      cPattern;
                UBool        success = TRUE;
                int32_t      patternStringIdx  = 0;
                CaseFoldingUCharIterator inputIterator(inputBuf, fp->fInputIdx, fActiveLimit);
                while (patternStringIdx < patternStringLen) {
                    U16_NEXT(patternString, patternStringIdx, patternStringLen, cPattern);
                    cText = inputIterator.next();
                    if (cText != cPattern) {
                        success = FALSE;
                        if (cText == U_SENTINEL) {
                            fHitEnd = TRUE;
                        }
                        break;
                    }
                }
                if (inputIterator.inExpansion()) {
                    success = FALSE;
                }

                if (success) {
                    fp->fInputIdx = inputIterator.getIndex();
                } else {
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                }
            }
            break;

        case URX_LB_START:
            {
                
                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                fData[opValue]   = fStack->size();
                fData[opValue+1] = fp->fInputIdx;
                
                fData[opValue+2] = -1;
                
                
                fData[opValue+3] = fActiveLimit;
                fActiveLimit     = fp->fInputIdx;
            }
            break;
            
            
        case URX_LB_CONT:
            {
                
                
                
                
                
                int32_t minML = (int32_t)pat[fp->fPatIdx++];
                int32_t maxML = (int32_t)pat[fp->fPatIdx++];
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);
                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int64_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    
                    
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;
                    } else {
                        U16_BACK_1(inputBuf, 0, *lbStartIdx);
                    }
                }
                
                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    
                    
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    int64_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= fActiveLimit);
                    U_ASSERT(restoreInputLen <= fInputLength);
                    fActiveLimit = restoreInputLen;
                    break;
                }
                
                
                
                fp = StateSave(fp, fp->fPatIdx-3, status);
                fp->fInputIdx =  *lbStartIdx;
            }
            break;
            
        case URX_LB_END:
            
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != fActiveLimit) {
                    
                    
                    
                    
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                
                
                
                int64_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= fActiveLimit);
                U_ASSERT(originalInputLen <= fInputLength);
                fActiveLimit = originalInputLen;
            }
            break;
            
            
        case URX_LBN_CONT:
            {
                
                
                
                
                int32_t minML       = (int32_t)pat[fp->fPatIdx++];
                int32_t maxML       = (int32_t)pat[fp->fPatIdx++];
                int32_t continueLoc = (int32_t)pat[fp->fPatIdx++];
                continueLoc = URX_VAL(continueLoc);
                U_ASSERT(minML <= maxML);
                U_ASSERT(minML >= 0);
                U_ASSERT(continueLoc > fp->fPatIdx);
                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int64_t  *lbStartIdx = &fData[opValue+2];
                if (*lbStartIdx < 0) {
                    
                    *lbStartIdx = fp->fInputIdx - minML;
                } else {
                    
                    
                    if (*lbStartIdx == 0) {
                        (*lbStartIdx)--;   
                    } else {
                        U16_BACK_1(inputBuf, 0, *lbStartIdx);
                    }
                }
                
                if (*lbStartIdx < 0 || *lbStartIdx < fp->fInputIdx - maxML) {
                    
                    
                    
                    int64_t restoreInputLen = fData[opValue+3];
                    U_ASSERT(restoreInputLen >= fActiveLimit);
                    U_ASSERT(restoreInputLen <= fInputLength);
                    fActiveLimit = restoreInputLen;
                    fp->fPatIdx = continueLoc;
                    break;
                }
                
                
                
                fp = StateSave(fp, fp->fPatIdx-4, status);
                fp->fInputIdx =  *lbStartIdx;
            }
            break;
            
        case URX_LBN_END:
            
            {
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                if (fp->fInputIdx != fActiveLimit) {
                    
                    
                    
                    
                    
                    fp = (REStackFrame *)fStack->popFrame(fFrameSize);
                    break;
                }
                
                
                
                
                
                
                
                int64_t originalInputLen = fData[opValue+3];
                U_ASSERT(originalInputLen >= fActiveLimit);
                U_ASSERT(originalInputLen <= fInputLength);
                fActiveLimit = originalInputLen;
                
                
                
                U_ASSERT(opValue>=0 && opValue+1<fPattern->fDataSize);
                int32_t newStackSize = (int32_t)fData[opValue];
                U_ASSERT(fStack->size() > newStackSize);
                fStack->setSize(newStackSize);
                
                
                
                fp = (REStackFrame *)fStack->popFrame(fFrameSize);
            }
            break;
            
            
        case URX_LOOP_SR_I:
            
            
            
            
            {
                U_ASSERT(opValue > 0 && opValue < sets->size());
                Regex8BitSet *s8 = &fPattern->fSets8[opValue];
                UnicodeSet   *s  = (UnicodeSet *)sets->elementAt(opValue);
                
                
                
                int32_t ix = (int32_t)fp->fInputIdx;
                for (;;) {
                    if (ix >= fActiveLimit) {
                        fHitEnd = TRUE;
                        break;
                    }
                    UChar32   c;
                    U16_NEXT(inputBuf, ix, fActiveLimit, c);
                    if (c<256) {
                        if (s8->contains(c) == FALSE) {
                            U16_BACK_1(inputBuf, 0, ix);
                            break;
                        }
                    } else {
                        if (s->contains(c) == FALSE) {
                            U16_BACK_1(inputBuf, 0, ix);
                            break;
                        }
                    }
                }
                
                
                
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   
                    break;
                }
                
                
                
                
                int32_t loopcOp = (int32_t)pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < fFrameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                fp->fInputIdx = ix;
                
                
                
                
                fp = StateSave(fp, fp->fPatIdx, status);
                fp->fPatIdx++;
            }
            break;
            
            
        case URX_LOOP_DOT_I:
            
            
            
            {
                
                
                int32_t ix;
                if ((opValue & 1) == 1) {
                    
                    ix = (int32_t)fActiveLimit;
                    fHitEnd = TRUE;
                } else {
                    
                    
                    ix = (int32_t)fp->fInputIdx;
                    for (;;) {
                        if (ix >= fActiveLimit) {
                            fHitEnd = TRUE;
                            break;
                        }
                        UChar32   c;
                        U16_NEXT(inputBuf, ix, fActiveLimit, c);   
                        if ((c & 0x7f) <= 0x29) {          
                            if ((c == 0x0a) ||             
                                (((opValue & 2) == 0) &&    
                                   ((c<=0x0d && c>=0x0a) || c==0x85 || c==0x2028 || c==0x2029))) {
                                
                                
                                U16_BACK_1(inputBuf, 0, ix);
                                break;
                            }
                        }
                    }
                }
                
                
                
                if (ix == fp->fInputIdx) {
                    fp->fPatIdx++;   
                    break;
                }
                
                
                
                
                int32_t loopcOp = (int32_t)pat[fp->fPatIdx];
                U_ASSERT(URX_TYPE(loopcOp) == URX_LOOP_C);
                int32_t stackLoc = URX_VAL(loopcOp);
                U_ASSERT(stackLoc >= 0 && stackLoc < fFrameSize);
                fp->fExtra[stackLoc] = fp->fInputIdx;
                fp->fInputIdx = ix;
                
                
                
                
                fp = StateSave(fp, fp->fPatIdx, status);
                fp->fPatIdx++;
            }
            break;
            
            
        case URX_LOOP_C:
            {
                U_ASSERT(opValue>=0 && opValue<fFrameSize);
                backSearchIndex = (int32_t)fp->fExtra[opValue];
                U_ASSERT(backSearchIndex <= fp->fInputIdx);
                if (backSearchIndex == fp->fInputIdx) {
                    
                    
                    
                    break;
                }
                
                
                
                
                
                U_ASSERT(fp->fInputIdx > 0);
                UChar32 prevC;
                U16_PREV(inputBuf, 0, fp->fInputIdx, prevC); 
                
                if (prevC == 0x0a && 
                    fp->fInputIdx > backSearchIndex &&
                    inputBuf[fp->fInputIdx-1] == 0x0d) {
                    int32_t prevOp = (int32_t)pat[fp->fPatIdx-2];
                    if (URX_TYPE(prevOp) == URX_LOOP_DOT_I) {
                        
                        U16_BACK_1(inputBuf, 0, fp->fInputIdx);
                    }
                }
                
                
                fp = StateSave(fp, fp->fPatIdx-1, status);
            }
            break;
            
            
            
        default:
            
            
            U_ASSERT(FALSE);
        }
        
        if (U_FAILURE(status)) {
            isMatch = FALSE;
            break;
        }
    }
    
breakFromLoop:
    fMatch = isMatch;
    if (isMatch) {
        fLastMatchEnd = fMatchEnd;
        fMatchStart   = startIdx;
        fMatchEnd     = fp->fInputIdx;
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("Match.  start=%d   end=%d\n\n", fMatchStart, fMatchEnd));
        }
    }
    else
    {
        if (fTraceDebug) {
            REGEX_RUN_DEBUG_PRINTF(("No match\n\n"));
        }
    }
    
    fFrame = fp;                
    
    

    return;
}


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RegexMatcher)

U_NAMESPACE_END

#endif  
