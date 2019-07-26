








#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/usearch.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/utf16.h"
#include "normalizer2impl.h"
#include "ucol_imp.h"
#include "usrchimp.h"
#include "cmemory.h"
#include "ucln_in.h"
#include "uassert.h"
#include "ustr_imp.h"

U_NAMESPACE_USE



#define BOYER_MOORE 0

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))



#define LAST_BYTE_MASK_          0xFF
#define SECOND_LAST_BYTE_SHIFT_  8
#define SUPPLEMENTARY_MIN_VALUE_ 0x10000

static const Normalizer2Impl *g_nfcImpl = NULL;









static
inline void setColEIterOffset(UCollationElements *elems,
                      int32_t             offset)
{
    collIterate *ci = &(elems->iteratordata_);
    ci->pos         = ci->string + offset;
    ci->CEpos       = ci->toReturn = ci->extendCEs ? ci->extendCEs : ci->CEs;
    if (ci->flags & UCOL_ITER_INNORMBUF) {
        ci->flags = ci->origFlags;
    }
    ci->fcdPosition = NULL;

    ci->offsetReturn = NULL;
    ci->offsetStore  = ci->offsetBuffer;
    ci->offsetRepeatCount = ci->offsetRepeatValue = 0;
}






static
inline uint32_t getMask(UCollationStrength strength)
{
    switch (strength)
    {
    case UCOL_PRIMARY:
        return UCOL_PRIMARYORDERMASK;
    case UCOL_SECONDARY:
        return UCOL_SECONDARYORDERMASK | UCOL_PRIMARYORDERMASK;
    default:
        return UCOL_TERTIARYORDERMASK | UCOL_SECONDARYORDERMASK |
               UCOL_PRIMARYORDERMASK;
    }
}






static
inline int hash(uint32_t ce)
{
    
    
    
    
    return UCOL_PRIMARYORDER(ce) % MAX_TABLE_SIZE_;
}

U_CDECL_BEGIN
static UBool U_CALLCONV
usearch_cleanup(void) {
    g_nfcImpl = NULL;
    return TRUE;
}
U_CDECL_END







static
inline void initializeFCD(UErrorCode *status)
{
    if (g_nfcImpl == NULL) {
        g_nfcImpl = Normalizer2Factory::getNFCImpl(*status);
        ucln_i18n_registerCleanup(UCLN_I18N_USEARCH, usearch_cleanup);
    }
}











static
uint16_t getFCD(const UChar   *str, int32_t *offset,
                             int32_t  strlength)
{
    const UChar *temp = str + *offset;
    uint16_t    result = g_nfcImpl->nextFCD16(temp, str + strlength);
    *offset = (int32_t)(temp - str);
    return result;
}








static
inline int32_t getCE(const UStringSearch *strsrch, uint32_t sourcece)
{
    
    
    
    sourcece &= strsrch->ceMask;

    if (strsrch->toShift) {
        
        
        
        
        
        if (strsrch->variableTop > sourcece) {
            if (strsrch->strength >= UCOL_QUATERNARY) {
                sourcece &= UCOL_PRIMARYORDERMASK;
            }
            else {
                sourcece = UCOL_IGNORABLE;
            }
        }
    } else if (strsrch->strength >= UCOL_QUATERNARY && sourcece == UCOL_IGNORABLE) {
        sourcece = 0xFFFF;
    }

    return sourcece;
}









static
inline void * allocateMemory(uint32_t size, UErrorCode *status)
{
    uint32_t *result = (uint32_t *)uprv_malloc(size);
    if (result == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

















static
inline int32_t * addTouint32_tArray(int32_t    *destination,
                                    uint32_t    offset,
                                    uint32_t   *destinationlength,
                                    uint32_t    value,
                                    uint32_t    increments,
                                    UErrorCode *status)
{
    uint32_t newlength = *destinationlength;
    if (offset + 1 == newlength) {
        newlength += increments;
        int32_t *temp = (int32_t *)allocateMemory(
                                         sizeof(int32_t) * newlength, status);
        if (U_FAILURE(*status)) {
            return NULL;
        }
        uprv_memcpy(temp, destination, sizeof(int32_t) * offset);
        *destinationlength = newlength;
        destination        = temp;
    }
    destination[offset] = value;
    return destination;
}

















static
inline int64_t * addTouint64_tArray(int64_t    *destination,
                                    uint32_t    offset,
                                    uint32_t   *destinationlength,
                                    uint64_t    value,
                                    uint32_t    increments,
                                    UErrorCode *status)
{
    uint32_t newlength = *destinationlength;
    if (offset + 1 == newlength) {
        newlength += increments;
        int64_t *temp = (int64_t *)allocateMemory(
                                         sizeof(int64_t) * newlength, status);

        if (U_FAILURE(*status)) {
            return NULL;
        }

        uprv_memcpy(temp, destination, sizeof(int64_t) * offset);
        *destinationlength = newlength;
        destination        = temp;
    }

    destination[offset] = value;

    return destination;
}













static
inline uint16_t initializePatternCETable(UStringSearch *strsrch,
                                         UErrorCode    *status)
{
    UPattern *pattern            = &(strsrch->pattern);
    uint32_t  cetablesize        = INITIAL_ARRAY_SIZE_;
    int32_t  *cetable            = pattern->CEBuffer;
    uint32_t  patternlength      = pattern->textLength;
    UCollationElements *coleiter = strsrch->utilIter;

    if (coleiter == NULL) {
        coleiter = ucol_openElements(strsrch->collator, pattern->text,
                                     patternlength, status);
        
        
        
        strsrch->utilIter = coleiter;
    }
    else {
        uprv_init_collIterate(strsrch->collator, pattern->text,
                         pattern->textLength,
                         &coleiter->iteratordata_,
                         status);
    }
    if(U_FAILURE(*status)) {
        return 0;
    }

    if (pattern->CE != cetable && pattern->CE) {
        uprv_free(pattern->CE);
    }

    uint16_t  offset      = 0;
    uint16_t  result      = 0;
    int32_t   ce;

    while ((ce = ucol_next(coleiter, status)) != UCOL_NULLORDER &&
           U_SUCCESS(*status)) {
        uint32_t newce = getCE(strsrch, ce);
        if (newce) {
            int32_t *temp = addTouint32_tArray(cetable, offset, &cetablesize,
                                  newce,
                                  patternlength - ucol_getOffset(coleiter) + 1,
                                  status);
            if (U_FAILURE(*status)) {
                return 0;
            }
            offset ++;
            if (cetable != temp && cetable != pattern->CEBuffer) {
                uprv_free(cetable);
            }
            cetable = temp;
        }
        result += (uint16_t)(ucol_getMaxExpansion(coleiter, ce) - 1);
    }

    cetable[offset]   = 0;
    pattern->CE       = cetable;
    pattern->CELength = offset;

    return result;
}













static
inline uint16_t initializePatternPCETable(UStringSearch *strsrch,
                                          UErrorCode    *status)
{
    UPattern *pattern            = &(strsrch->pattern);
    uint32_t  pcetablesize       = INITIAL_ARRAY_SIZE_;
    int64_t  *pcetable           = pattern->PCEBuffer;
    uint32_t  patternlength      = pattern->textLength;
    UCollationElements *coleiter = strsrch->utilIter;

    if (coleiter == NULL) {
        coleiter = ucol_openElements(strsrch->collator, pattern->text,
                                     patternlength, status);
        
        
        
        strsrch->utilIter = coleiter;
    } else {
        uprv_init_collIterate(strsrch->collator, pattern->text,
                              pattern->textLength,
                              &coleiter->iteratordata_,
                              status);
    }
    if(U_FAILURE(*status)) {
        return 0;
    }

    if (pattern->PCE != pcetable && pattern->PCE != NULL) {
        uprv_free(pattern->PCE);
    }

    uint16_t  offset = 0;
    uint16_t  result = 0;
    int64_t   pce;

    uprv_init_pce(coleiter);

    
    
    
    while ((pce = ucol_nextProcessed(coleiter, NULL, NULL, status)) != UCOL_PROCESSED_NULLORDER &&
           U_SUCCESS(*status)) {
        int64_t *temp = addTouint64_tArray(pcetable, offset, &pcetablesize,
                              pce,
                              patternlength - ucol_getOffset(coleiter) + 1,
                              status);

        if (U_FAILURE(*status)) {
            return 0;
        }

        offset += 1;

        if (pcetable != temp && pcetable != pattern->PCEBuffer) {
            uprv_free(pcetable);
        }

        pcetable = temp;
        
    }

    pcetable[offset]   = 0;
    pattern->PCE       = pcetable;
    pattern->PCELength = offset;

    return result;
}









static
inline int16_t initializePattern(UStringSearch *strsrch, UErrorCode *status)
{
          UPattern   *pattern     = &(strsrch->pattern);
    const UChar      *patterntext = pattern->text;
          int32_t     length      = pattern->textLength;
          int32_t index       = 0;

    
    if (strsrch->strength == UCOL_PRIMARY) {
        pattern->hasPrefixAccents = 0;
        pattern->hasSuffixAccents = 0;
    } else {
        pattern->hasPrefixAccents = getFCD(patterntext, &index, length) >>
                                                         SECOND_LAST_BYTE_SHIFT_;
        index = length;
        U16_BACK_1(patterntext, 0, index);
        pattern->hasSuffixAccents = getFCD(patterntext, &index, length) &
                                                                 LAST_BYTE_MASK_;
    }

    
    if (strsrch->pattern.PCE != NULL) {
        if (strsrch->pattern.PCE != strsrch->pattern.PCEBuffer) {
            uprv_free(strsrch->pattern.PCE);
        }

        strsrch->pattern.PCE = NULL;
    }

    
    return initializePatternCETable(strsrch, status);
}












static
inline void setShiftTable(int16_t   shift[], int16_t backshift[],
                          int32_t  *cetable, int32_t cesize,
                          int16_t   expansionsize,
                          int16_t   defaultforward,
                          int16_t   defaultbackward)
{
    
    
    
    
    int32_t count;
    for (count = 0; count < MAX_TABLE_SIZE_; count ++) {
        shift[count] = defaultforward;
    }
    cesize --; 
    for (count = 0; count < cesize; count ++) {
        
        int temp = defaultforward - count - 1;
        shift[hash(cetable[count])] = temp > 1 ? temp : 1;
    }
    shift[hash(cetable[cesize])] = 1;
    
    shift[hash(0)] = 1;

    for (count = 0; count < MAX_TABLE_SIZE_; count ++) {
        backshift[count] = defaultbackward;
    }
    for (count = cesize; count > 0; count --) {
        
        backshift[hash(cetable[count])] = count > expansionsize ?
                                          (int16_t)(count - expansionsize) : 1;
    }
    backshift[hash(cetable[0])] = 1;
    backshift[hash(0)] = 1;
}





























static
inline void initialize(UStringSearch *strsrch, UErrorCode *status)
{
    int16_t expandlength  = initializePattern(strsrch, status);
    if (U_SUCCESS(*status) && strsrch->pattern.CELength > 0) {
        UPattern *pattern = &strsrch->pattern;
        int32_t   cesize  = pattern->CELength;

        int16_t minlength = cesize > expandlength
                            ? (int16_t)cesize - expandlength : 1;
        pattern->defaultShiftSize    = minlength;
        setShiftTable(pattern->shift, pattern->backShift, pattern->CE,
                      cesize, expandlength, minlength, minlength);
        return;
    }
    strsrch->pattern.defaultShiftSize = 0;
}

#if BOYER_MOORE







static
void checkBreakBoundary(const UStringSearch *strsrch, int32_t * ,
                               int32_t *end)
{
#if !UCONFIG_NO_BREAK_ITERATION
    UBreakIterator *breakiterator = strsrch->search->internalBreakIter;
    if (breakiterator) {
        int32_t matchend = *end;
        

        if (!ubrk_isBoundary(breakiterator, matchend)) {
            *end = ubrk_following(breakiterator, matchend);
        }

        

        


    }
#endif
}









static
UBool isBreakUnit(const UStringSearch *strsrch, int32_t start,
                               int32_t    end)
{
#if !UCONFIG_NO_BREAK_ITERATION
    UBreakIterator *breakiterator = strsrch->search->breakIter;
    
    if (breakiterator) {
        int32_t startindex = ubrk_first(breakiterator);
        int32_t endindex   = ubrk_last(breakiterator);

        
        if (start < startindex || start > endindex ||
            end < startindex || end > endindex) {
            return FALSE;
        }
        
        
        
        UBool result = (start == startindex ||
                ubrk_following(breakiterator, start - 1) == start) &&
               (end == endindex ||
                ubrk_following(breakiterator, end - 1) == end);
        if (result) {
            
                  UCollationElements *coleiter  = strsrch->utilIter;
            const UChar              *text      = strsrch->search->text +
                                                                      start;
                  UErrorCode          status    = U_ZERO_ERROR;
            ucol_setText(coleiter, text, end - start, &status);
            for (int32_t count = 0; count < strsrch->pattern.CELength;
                 count ++) {
                int32_t ce = getCE(strsrch, ucol_next(coleiter, &status));
                if (ce == UCOL_IGNORABLE) {
                    count --;
                    continue;
                }
                if (U_FAILURE(status) || ce != strsrch->pattern.CE[count]) {
                    return FALSE;
                }
            }
            int32_t nextce = ucol_next(coleiter, &status);
            while (ucol_getOffset(coleiter) == (end - start)
                   && getCE(strsrch, nextce) == UCOL_IGNORABLE) {
                nextce = ucol_next(coleiter, &status);
            }
            if (ucol_getOffset(coleiter) == (end - start)
                && nextce != UCOL_NULLORDER) {
                
                return FALSE;
            }
        }
        return result;
    }
#endif
    return TRUE;
}











static
inline int32_t getNextBaseOffset(const UChar       *text,
                                           int32_t  textoffset,
                                           int32_t      textlength)
{
    if (textoffset < textlength) {
        int32_t temp = textoffset;
        if (getFCD(text, &temp, textlength) >> SECOND_LAST_BYTE_SHIFT_) {
            while (temp < textlength) {
                int32_t result = temp;
                if ((getFCD(text, &temp, textlength) >>
                     SECOND_LAST_BYTE_SHIFT_) == 0) {
                    return result;
                }
            }
            return textlength;
        }
    }
    return textoffset;
}










static
inline int32_t getNextUStringSearchBaseOffset(UStringSearch *strsrch,
                                                  int32_t    textoffset)
{
    int32_t textlength = strsrch->search->textLength;
    if (strsrch->pattern.hasSuffixAccents &&
        textoffset < textlength) {
              int32_t  temp       = textoffset;
        const UChar       *text       = strsrch->search->text;
        U16_BACK_1(text, 0, temp);
        if (getFCD(text, &temp, textlength) & LAST_BYTE_MASK_) {
            return getNextBaseOffset(text, textoffset, textlength);
        }
    }
    return textoffset;
}













static
inline int32_t shiftForward(UStringSearch *strsrch,
                                int32_t    textoffset,
                                int32_t       ce,
                                int32_t        patternceindex)
{
    UPattern *pattern = &(strsrch->pattern);
    if (ce != UCOL_NULLORDER) {
        int32_t shift = pattern->shift[hash(ce)];
        
        
        int32_t adjust = pattern->CELength - patternceindex;
        if (adjust > 1 && shift >= adjust) {
            shift -= adjust - 1;
        }
        textoffset += shift;
    }
    else {
        textoffset += pattern->defaultShiftSize;
    }

    textoffset = getNextUStringSearchBaseOffset(strsrch, textoffset);
    
    
    
    
    
    
    return textoffset;
}
#endif 





static
inline void setMatchNotFound(UStringSearch *strsrch)
{
    
    strsrch->search->matchedIndex = USEARCH_DONE;
    strsrch->search->matchedLength = 0;
    if (strsrch->search->isForwardSearching) {
        setColEIterOffset(strsrch->textIter, strsrch->search->textLength);
    }
    else {
        setColEIterOffset(strsrch->textIter, 0);
    }
}

#if BOYER_MOORE










static
inline int32_t getNextSafeOffset(const UCollator   *collator,
                                     const UChar       *text,
                                           int32_t  textoffset,
                                           int32_t      textlength)
{
    int32_t result = textoffset; 
    while (result != textlength && ucol_unsafeCP(text[result], collator)) {
        result ++;
    }
    return result;
}


























static
UBool checkExtraMatchAccents(const UStringSearch *strsrch, int32_t start,
                                   int32_t    end,
                                   UErrorCode    *status)
{
    UBool result = FALSE;
    if (strsrch->pattern.hasPrefixAccents) {
              int32_t  length = end - start;
              int32_t  offset = 0;
        const UChar       *text   = strsrch->search->text + start;

        U16_FWD_1(text, offset, length);
        
        if (unorm_quickCheck(text, offset, UNORM_NFD, status) == UNORM_NO) {
            int32_t safeoffset = getNextSafeOffset(strsrch->collator,
                                                       text, 0, length);
            if (safeoffset != length) {
                safeoffset ++;
            }
            UChar   *norm = NULL;
            UChar    buffer[INITIAL_ARRAY_SIZE_];
            int32_t  size = unorm_normalize(text, safeoffset, UNORM_NFD, 0,
                                            buffer, INITIAL_ARRAY_SIZE_,
                                            status);
            if (U_FAILURE(*status)) {
                return FALSE;
            }
            if (size >= INITIAL_ARRAY_SIZE_) {
                norm = (UChar *)allocateMemory((size + 1) * sizeof(UChar),
                                               status);
                
                
                
                size = unorm_normalize(text, safeoffset, UNORM_NFD, 0, norm,
                                       size, status);
                if (U_FAILURE(*status) && norm != NULL) {
                    uprv_free(norm);
                    return FALSE;
                }
            }
            else {
                norm = buffer;
            }

            UCollationElements *coleiter  = strsrch->utilIter;
            ucol_setText(coleiter, norm, size, status);
            uint32_t            firstce   = strsrch->pattern.CE[0];
            UBool               ignorable = TRUE;
            uint32_t            ce        = UCOL_IGNORABLE;
            while (U_SUCCESS(*status) && ce != firstce && ce != (uint32_t)UCOL_NULLORDER) {
                offset = ucol_getOffset(coleiter);
                if (ce != firstce && ce != UCOL_IGNORABLE) {
                    ignorable = FALSE;
                }
                ce = ucol_next(coleiter, status);
            }
            UChar32 codepoint;
            U16_PREV(norm, 0, offset, codepoint);
            result = !ignorable && (u_getCombiningClass(codepoint) != 0);

            if (norm != buffer) {
                uprv_free(norm);
            }
        }
    }

    return result;
}






















static
UBool hasAccentsBeforeMatch(const UStringSearch *strsrch, int32_t start,
                                  int32_t    end)
{
    if (strsrch->pattern.hasPrefixAccents) {
        UCollationElements *coleiter  = strsrch->textIter;
        UErrorCode          status    = U_ZERO_ERROR;
        
        uint32_t            ignorable = TRUE;
        int32_t             firstce   = strsrch->pattern.CE[0];

        setColEIterOffset(coleiter, start);
        int32_t ce  = getCE(strsrch, ucol_next(coleiter, &status));
        if (U_FAILURE(status)) {
            return TRUE;
        }
        while (ce != firstce) {
            if (ce != UCOL_IGNORABLE) {
                ignorable = FALSE;
            }
            ce = getCE(strsrch, ucol_next(coleiter, &status));
            if (U_FAILURE(status) || ce == UCOL_NULLORDER) {
                return TRUE;
            }
        }
        if (!ignorable && inNormBuf(coleiter)) {
            
            return TRUE;
        }

        
        int32_t temp = start;
        
        
        
        
        
        
        
        UBool accent = getFCD(strsrch->search->text, &temp,
                              strsrch->search->textLength) > 0xFF;
        if (!accent) {
            return checkExtraMatchAccents(strsrch, start, end, &status);
        }
        if (!ignorable) {
            return TRUE;
        }
        if (start > 0) {
            temp = start;
            U16_BACK_1(strsrch->search->text, 0, temp);
            if (getFCD(strsrch->search->text, &temp,
                       strsrch->search->textLength) & LAST_BYTE_MASK_) {
                setColEIterOffset(coleiter, start);
                ce = ucol_previous(coleiter, &status);
                if (U_FAILURE(status) ||
                    (ce != UCOL_NULLORDER && ce != UCOL_IGNORABLE)) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

















static
UBool hasAccentsAfterMatch(const UStringSearch *strsrch, int32_t start,
                                 int32_t    end)
{
    if (strsrch->pattern.hasSuffixAccents) {
        const UChar       *text       = strsrch->search->text;
              int32_t  temp       = end;
              int32_t      textlength = strsrch->search->textLength;
        U16_BACK_1(text, 0, temp);
        if (getFCD(text, &temp, textlength) & LAST_BYTE_MASK_) {
            int32_t             firstce  = strsrch->pattern.CE[0];
            UCollationElements *coleiter = strsrch->textIter;
            UErrorCode          status   = U_ZERO_ERROR;
            int32_t ce;
            setColEIterOffset(coleiter, start);
            while ((ce = getCE(strsrch, ucol_next(coleiter, &status))) != firstce) {
                if (U_FAILURE(status) || ce == UCOL_NULLORDER) {
                    return TRUE;
                }
            }
            int32_t count = 1;
            while (count < strsrch->pattern.CELength) {
                if (getCE(strsrch, ucol_next(coleiter, &status))
                    == UCOL_IGNORABLE) {
                    
                    count --;
                }
                if (U_FAILURE(status)) {
                    return TRUE;
                }
                count ++;
            }

            ce = ucol_next(coleiter, &status);
            if (U_FAILURE(status)) {
                return TRUE;
            }
            if (ce != UCOL_NULLORDER && ce != UCOL_IGNORABLE) {
                ce = getCE(strsrch, ce);
            }
            if (ce != UCOL_NULLORDER && ce != UCOL_IGNORABLE) {
                if (ucol_getOffset(coleiter) <= end) {
                    return TRUE;
                }
                if (getFCD(text, &end, textlength) >> SECOND_LAST_BYTE_SHIFT_) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}
#endif 







static
inline UBool isOutOfBounds(int32_t textlength, int32_t offset)
{
    return offset < 0 || offset > textlength;
}








static
inline UBool checkIdentical(const UStringSearch *strsrch, int32_t start,
                                  int32_t    end)
{
    if (strsrch->strength != UCOL_IDENTICAL) {
        return TRUE;
    }

    
    
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString t2, p2;
    strsrch->nfd->normalize(
        UnicodeString(FALSE, strsrch->search->text + start, end - start), t2, status);
    strsrch->nfd->normalize(
        UnicodeString(FALSE, strsrch->pattern.text, strsrch->pattern.textLength), p2, status);
    
    return U_SUCCESS(status) && t2 == p2;
}

#if BOYER_MOORE







static
inline UBool checkRepeatedMatch(UStringSearch *strsrch,
                                int32_t    start,
                                int32_t    end)
{
    int32_t lastmatchindex = strsrch->search->matchedIndex;
    UBool       result;
    if (lastmatchindex == USEARCH_DONE) {
        return FALSE;
    }
    if (strsrch->search->isForwardSearching) {
        result = start <= lastmatchindex;
    }
    else {
        result = start >= lastmatchindex;
    }
    if (!result && !strsrch->search->isOverlap) {
        if (strsrch->search->isForwardSearching) {
            result = start < lastmatchindex + strsrch->search->matchedLength;
        }
        else {
            result = end > lastmatchindex;
        }
    }
    return result;
}







static
inline int32_t getColElemIterOffset(const UCollationElements *coleiter,
                                              UBool               forwards)
{
    int32_t result = ucol_getOffset(coleiter);
    
    if (FALSE && !forwards && inNormBuf(coleiter) && !isFCDPointerNull(coleiter)) {
        result ++;
    }
    return result;
}
















static
UBool checkNextExactContractionMatch(UStringSearch *strsrch,
                                     int32_t   *start,
                                     int32_t   *end, UErrorCode  *status)
{
          UCollationElements *coleiter   = strsrch->textIter;
          int32_t             textlength = strsrch->search->textLength;
          int32_t             temp       = *start;
    const UCollator          *collator   = strsrch->collator;
    const UChar              *text       = strsrch->search->text;
    
    
    
    
    
    
    
    
    
    if ((*end < textlength && ucol_unsafeCP(text[*end], collator)) ||
        (*start + 1 < textlength
         && ucol_unsafeCP(text[*start + 1], collator))) {
        int32_t expansion  = getExpansionPrefix(coleiter);
        UBool   expandflag = expansion > 0;
        setColEIterOffset(coleiter, *start);
        while (expansion > 0) {
            
            
            
            
            
            
            
            ucol_next(coleiter, status);
            if (U_FAILURE(*status)) {
                return FALSE;
            }
            if (ucol_getOffset(coleiter) != temp) {
                *start = temp;
                temp  = ucol_getOffset(coleiter);
            }
            expansion --;
        }

        int32_t  *patternce       = strsrch->pattern.CE;
        int32_t   patterncelength = strsrch->pattern.CELength;
        int32_t   count           = 0;
        while (count < patterncelength) {
            int32_t ce = getCE(strsrch, ucol_next(coleiter, status));
            if (ce == UCOL_IGNORABLE) {
                continue;
            }
            if (expandflag && count == 0 && ucol_getOffset(coleiter) != temp) {
                *start = temp;
                temp   = ucol_getOffset(coleiter);
            }
            if (U_FAILURE(*status) || ce != patternce[count]) {
                (*end) ++;
                *end = getNextUStringSearchBaseOffset(strsrch, *end);
                return FALSE;
            }
            count ++;
        }
    }
    return TRUE;
}





















static
inline UBool checkNextExactMatch(UStringSearch *strsrch,
                                 int32_t   *textoffset, UErrorCode *status)
{
    UCollationElements *coleiter = strsrch->textIter;
    int32_t         start    = getColElemIterOffset(coleiter, FALSE);

    if (!checkNextExactContractionMatch(strsrch, &start, textoffset, status)) {
        return FALSE;
    }

    
    if (!isBreakUnit(strsrch, start, *textoffset) ||
        checkRepeatedMatch(strsrch, start, *textoffset) ||
        hasAccentsBeforeMatch(strsrch, start, *textoffset) ||
        !checkIdentical(strsrch, start, *textoffset) ||
        hasAccentsAfterMatch(strsrch, start, *textoffset)) {

        (*textoffset) ++;
        *textoffset = getNextUStringSearchBaseOffset(strsrch, *textoffset);
        return FALSE;
    }

    
    if (!strsrch->search->breakIter && strsrch->strength == UCOL_PRIMARY) {
        checkBreakBoundary(strsrch, &start, textoffset);
    }

    
    strsrch->search->matchedIndex  = start;
    strsrch->search->matchedLength = *textoffset - start;
    return TRUE;
}









static
inline int32_t getPreviousBaseOffset(const UChar       *text,
                                               int32_t  textoffset)
{
    if (textoffset > 0) {
        for (;;) {
            int32_t result = textoffset;
            U16_BACK_1(text, 0, textoffset);
            int32_t temp = textoffset;
            uint16_t fcd = getFCD(text, &temp, result);
            if ((fcd >> SECOND_LAST_BYTE_SHIFT_) == 0) {
                if (fcd & LAST_BYTE_MASK_) {
                    return textoffset;
                }
                return result;
            }
            if (textoffset == 0) {
                return 0;
            }
        }
    }
    return textoffset;
}







static
inline int getUnblockedAccentIndex(UChar *accents, int32_t *accentsindex)
{
    int32_t index     = 0;
    int32_t     length    = u_strlen(accents);
    UChar32     codepoint = 0;
    int         cclass    = 0;
    int         result    = 0;
    int32_t temp;
    while (index < length) {
        temp = index;
        U16_NEXT(accents, index, length, codepoint);
        if (u_getCombiningClass(codepoint) != cclass) {
            cclass        = u_getCombiningClass(codepoint);
            accentsindex[result] = temp;
            result ++;
        }
    }
    accentsindex[result] = length;
    return result;
}

















static
inline UChar * addToUCharArray(      UChar      *destination,
                                     int32_t    *destinationlength,
                               const UChar      *source1,
                               const UChar      *source2,
                                     int32_t     source2length,
                               const UChar      *source3,
                                     UErrorCode *status)
{
    int32_t source1length = source1 ? u_strlen(source1) : 0;
    int32_t source3length = source3 ? u_strlen(source3) : 0;
    if (*destinationlength < source1length + source2length + source3length +
                                                                           1)
    {
        destination = (UChar *)allocateMemory(
          (source1length + source2length + source3length + 1) * sizeof(UChar),
          status);
        
        
        if (U_FAILURE(*status)) {
            *destinationlength = 0;
            return NULL;
        }
    }
    if (source1length != 0) {
        uprv_memcpy(destination, source1, sizeof(UChar) * source1length);
    }
    if (source2length != 0) {
        uprv_memcpy(destination + source1length, source2,
                    sizeof(UChar) * source2length);
    }
    if (source3length != 0) {
        uprv_memcpy(destination + source1length + source2length, source3,
                    sizeof(UChar) * source3length);
    }
    *destinationlength = source1length + source2length + source3length;
    return destination;
}








static
inline UBool checkCollationMatch(const UStringSearch      *strsrch,
                                       UCollationElements *coleiter)
{
    int         patternceindex = strsrch->pattern.CELength;
    int32_t    *patternce      = strsrch->pattern.CE;
    UErrorCode  status = U_ZERO_ERROR;
    while (patternceindex > 0) {
        int32_t ce = getCE(strsrch, ucol_next(coleiter, &status));
        if (ce == UCOL_IGNORABLE) {
            continue;
        }
        if (U_FAILURE(status) || ce != *patternce) {
            return FALSE;
        }
        patternce ++;
        patternceindex --;
    }
    return TRUE;
}




















static
int32_t doNextCanonicalPrefixMatch(UStringSearch *strsrch,
                                       int32_t    start,
                                       int32_t    end,
                                       UErrorCode    *status)
{
    const UChar       *text       = strsrch->search->text;
          int32_t      textlength = strsrch->search->textLength;
          int32_t  tempstart  = start;

    if ((getFCD(text, &tempstart, textlength) & LAST_BYTE_MASK_) == 0) {
        
        return USEARCH_DONE;
    }

    int32_t offset = getNextBaseOffset(text, tempstart, textlength);
    start = getPreviousBaseOffset(text, tempstart);

    UChar       accents[INITIAL_ARRAY_SIZE_];
    
    unorm_normalize(text + start, offset - start, UNORM_NFD, 0, accents,
                    INITIAL_ARRAY_SIZE_, status);
    if (U_FAILURE(*status)) {
        return USEARCH_DONE;
    }

    int32_t         accentsindex[INITIAL_ARRAY_SIZE_];
    int32_t         accentsize = getUnblockedAccentIndex(accents,
                                                                 accentsindex);
    int32_t         count      = (2 << (accentsize - 1)) - 1;
    UChar               buffer[INITIAL_ARRAY_SIZE_];
    UCollationElements *coleiter   = strsrch->utilIter;
    while (U_SUCCESS(*status) && count > 0) {
        UChar *rearrange = strsrch->canonicalPrefixAccents;
        
        for (int k = 0; k < accentsindex[0]; k ++) {
            *rearrange ++ = accents[k];
        }
        
        
        for (int i = 0; i <= accentsize - 1; i ++) {
            int32_t mask = 1 << (accentsize - i - 1);
            if (count & mask) {
                for (int j = accentsindex[i]; j < accentsindex[i + 1]; j ++) {
                    *rearrange ++ = accents[j];
                }
            }
        }
        *rearrange = 0;
        int32_t  matchsize = INITIAL_ARRAY_SIZE_;
        UChar   *match     = addToUCharArray(buffer, &matchsize,
                                           strsrch->canonicalPrefixAccents,
                                           strsrch->search->text + offset,
                                           end - offset,
                                           strsrch->canonicalSuffixAccents,
                                           status);

        
        
        ucol_setText(coleiter, match, matchsize, status);
        if (U_SUCCESS(*status)) {
            if (checkCollationMatch(strsrch, coleiter)) {
                if (match != buffer) {
                    uprv_free(match);
                }
                return start;
            }
        }
        count --;
    }
    return USEARCH_DONE;
}











static
inline uint32_t getPreviousSafeOffset(const UCollator   *collator,
                                      const UChar       *text,
                                            int32_t  textoffset)
{
    int32_t result = textoffset; 
    while (result != 0 && ucol_unsafeCP(text[result - 1], collator)) {
        result --;
    }
    if (result != 0) {
        
        result --;
    }
    return result;
}








static
inline void cleanUpSafeText(const UStringSearch *strsrch, UChar *safetext,
                                  UChar         *safebuffer)
{
    if (safetext != safebuffer && safetext != strsrch->canonicalSuffixAccents)
    {
       uprv_free(safetext);
    }
}
















static
int32_t doNextCanonicalSuffixMatch(UStringSearch *strsrch,
                                       int32_t    textoffset,
                                       UErrorCode    *status)
{
    const UChar              *text           = strsrch->search->text;
    const UCollator          *collator       = strsrch->collator;
          int32_t             safelength     = 0;
          UChar              *safetext;
          int32_t             safetextlength;
          UChar               safebuffer[INITIAL_ARRAY_SIZE_];
          UCollationElements *coleiter       = strsrch->utilIter;
          int32_t         safeoffset     = textoffset;

    if (textoffset != 0 && ucol_unsafeCP(strsrch->canonicalSuffixAccents[0],
                                         collator)) {
        safeoffset     = getPreviousSafeOffset(collator, text, textoffset);
        safelength     = textoffset - safeoffset;
        safetextlength = INITIAL_ARRAY_SIZE_;
        safetext       = addToUCharArray(safebuffer, &safetextlength, NULL,
                                         text + safeoffset, safelength,
                                         strsrch->canonicalSuffixAccents,
                                         status);
    }
    else {
        safetextlength = u_strlen(strsrch->canonicalSuffixAccents);
        safetext       = strsrch->canonicalSuffixAccents;
    }

    
    ucol_setText(coleiter, safetext, safetextlength, status);
    

    int32_t  *ce        = strsrch->pattern.CE;
    int32_t   celength  = strsrch->pattern.CELength;
    int       ceindex   = celength - 1;
    UBool     isSafe    = TRUE; 

    while (ceindex >= 0) {
        int32_t textce = ucol_previous(coleiter, status);
        if (U_FAILURE(*status)) {
            if (isSafe) {
                cleanUpSafeText(strsrch, safetext, safebuffer);
            }
            return USEARCH_DONE;
        }
        if (textce == UCOL_NULLORDER) {
            
            if (coleiter == strsrch->textIter) {
                cleanUpSafeText(strsrch, safetext, safebuffer);
                return USEARCH_DONE;
            }
            cleanUpSafeText(strsrch, safetext, safebuffer);
            safetext = safebuffer;
            coleiter = strsrch->textIter;
            setColEIterOffset(coleiter, safeoffset);
            
            isSafe = FALSE;
            continue;
        }
        textce = getCE(strsrch, textce);
        if (textce != UCOL_IGNORABLE && textce != ce[ceindex]) {
            
            int32_t failedoffset = getColElemIterOffset(coleiter, FALSE);
            if (isSafe && failedoffset >= safelength) {
                
                cleanUpSafeText(strsrch, safetext, safebuffer);
                return USEARCH_DONE;
            }
            else {
                if (isSafe) {
                    failedoffset += safeoffset;
                    cleanUpSafeText(strsrch, safetext, safebuffer);
                }

                
                int32_t result = doNextCanonicalPrefixMatch(strsrch,
                                        failedoffset, textoffset, status);
                if (result != USEARCH_DONE) {
                    
                    setColEIterOffset(strsrch->textIter, result);
                }
                if (U_FAILURE(*status)) {
                    return USEARCH_DONE;
                }
                return result;
            }
        }
        if (textce == ce[ceindex]) {
            ceindex --;
        }
    }
    
    if (isSafe) {
        int32_t result     = getColElemIterOffset(coleiter, FALSE);
        
        int32_t    leftoverces = getExpansionPrefix(coleiter);
        cleanUpSafeText(strsrch, safetext, safebuffer);
        if (result >= safelength) {
            result = textoffset;
        }
        else {
            result += safeoffset;
        }
        setColEIterOffset(strsrch->textIter, result);
        strsrch->textIter->iteratordata_.toReturn =
                       setExpansionPrefix(strsrch->textIter, leftoverces);
        return result;
    }

    return ucol_getOffset(coleiter);
}





















static
UBool doNextCanonicalMatch(UStringSearch *strsrch,
                           int32_t    textoffset,
                           UErrorCode    *status)
{
    const UChar       *text = strsrch->search->text;
          int32_t  temp = textoffset;
    U16_BACK_1(text, 0, temp);
    if ((getFCD(text, &temp, textoffset) & LAST_BYTE_MASK_) == 0) {
        UCollationElements *coleiter = strsrch->textIter;
        int32_t         offset   = getColElemIterOffset(coleiter, FALSE);
        if (strsrch->pattern.hasPrefixAccents) {
            offset = doNextCanonicalPrefixMatch(strsrch, offset, textoffset,
                                                status);
            if (U_SUCCESS(*status) && offset != USEARCH_DONE) {
                setColEIterOffset(coleiter, offset);
                return TRUE;
            }
        }
        return FALSE;
    }

    if (!strsrch->pattern.hasSuffixAccents) {
        return FALSE;
    }

    UChar       accents[INITIAL_ARRAY_SIZE_];
    
    int32_t baseoffset = getPreviousBaseOffset(text, textoffset);
    
    unorm_normalize(text + baseoffset, textoffset - baseoffset, UNORM_NFD,
                               0, accents, INITIAL_ARRAY_SIZE_, status);
    

    int32_t accentsindex[INITIAL_ARRAY_SIZE_];
    int32_t size = getUnblockedAccentIndex(accents, accentsindex);

    
    int32_t  count = (2 << (size - 1)) - 1;
    while (U_SUCCESS(*status) && count > 0) {
        UChar *rearrange = strsrch->canonicalSuffixAccents;
        
        for (int k = 0; k < accentsindex[0]; k ++) {
            *rearrange ++ = accents[k];
        }
        
        
        for (int i = 0; i <= size - 1; i ++) {
            int32_t mask = 1 << (size - i - 1);
            if (count & mask) {
                for (int j = accentsindex[i]; j < accentsindex[i + 1]; j ++) {
                    *rearrange ++ = accents[j];
                }
            }
        }
        *rearrange = 0;
        int32_t offset = doNextCanonicalSuffixMatch(strsrch, baseoffset,
                                                        status);
        if (offset != USEARCH_DONE) {
            return TRUE; 
        }
        count --;
    }
    return FALSE;
}









static
inline int32_t getPreviousUStringSearchBaseOffset(UStringSearch *strsrch,
                                                      int32_t textoffset)
{
    if (strsrch->pattern.hasPrefixAccents && textoffset > 0) {
        const UChar       *text = strsrch->search->text;
              int32_t  offset = textoffset;
        if (getFCD(text, &offset, strsrch->search->textLength) >>
                                                   SECOND_LAST_BYTE_SHIFT_) {
            return getPreviousBaseOffset(text, textoffset);
        }
    }
    return textoffset;
}














static
UBool checkNextCanonicalContractionMatch(UStringSearch *strsrch,
                                         int32_t   *start,
                                         int32_t   *end,
                                         UErrorCode    *status)
{
          UCollationElements *coleiter   = strsrch->textIter;
          int32_t             textlength = strsrch->search->textLength;
          int32_t         temp       = *start;
    const UCollator          *collator   = strsrch->collator;
    const UChar              *text       = strsrch->search->text;
    
    
    if ((*end < textlength && ucol_unsafeCP(text[*end], collator)) ||
        (*start + 1 < textlength
         && ucol_unsafeCP(text[*start + 1], collator))) {
        int32_t expansion  = getExpansionPrefix(coleiter);
        UBool   expandflag = expansion > 0;
        setColEIterOffset(coleiter, *start);
        while (expansion > 0) {
            
            
            
            
            
            
            
            ucol_next(coleiter, status);
            if (U_FAILURE(*status)) {
                return FALSE;
            }
            if (ucol_getOffset(coleiter) != temp) {
                *start = temp;
                temp  = ucol_getOffset(coleiter);
            }
            expansion --;
        }

        int32_t  *patternce       = strsrch->pattern.CE;
        int32_t   patterncelength = strsrch->pattern.CELength;
        int32_t   count           = 0;
        int32_t   textlength      = strsrch->search->textLength;
        while (count < patterncelength) {
            int32_t ce = getCE(strsrch, ucol_next(coleiter, status));
            
            
            if (ce == UCOL_IGNORABLE) {
                continue;
            }
            if (expandflag && count == 0 && ucol_getOffset(coleiter) != temp) {
                *start = temp;
                temp   = ucol_getOffset(coleiter);
            }

            if (count == 0 && ce != patternce[0]) {
                
                
                
                int32_t expected = patternce[0];
                if (getFCD(text, start, textlength) & LAST_BYTE_MASK_) {
                    ce = getCE(strsrch, ucol_next(coleiter, status));
                    while (U_SUCCESS(*status) && ce != expected &&
                           ce != UCOL_NULLORDER &&
                           ucol_getOffset(coleiter) <= *end) {
                        ce = getCE(strsrch, ucol_next(coleiter, status));
                    }
                }
            }
            if (U_FAILURE(*status) || ce != patternce[count]) {
                (*end) ++;
                *end = getNextUStringSearchBaseOffset(strsrch, *end);
                return FALSE;
            }
            count ++;
        }
    }
    return TRUE;
}




















static
inline UBool checkNextCanonicalMatch(UStringSearch *strsrch,
                                     int32_t   *textoffset,
                                     UErrorCode    *status)
{
    
    UCollationElements *coleiter = strsrch->textIter;
    
    if ((strsrch->pattern.hasSuffixAccents &&
        strsrch->canonicalSuffixAccents[0]) ||
        (strsrch->pattern.hasPrefixAccents &&
        strsrch->canonicalPrefixAccents[0])) {
        strsrch->search->matchedIndex  = getPreviousUStringSearchBaseOffset(
                                                    strsrch,
                                                    ucol_getOffset(coleiter));
        strsrch->search->matchedLength = *textoffset -
                                                strsrch->search->matchedIndex;
        return TRUE;
    }

    int32_t start = getColElemIterOffset(coleiter, FALSE);
    if (!checkNextCanonicalContractionMatch(strsrch, &start, textoffset,
                                            status) || U_FAILURE(*status)) {
        return FALSE;
    }

    start = getPreviousUStringSearchBaseOffset(strsrch, start);
    
    if (checkRepeatedMatch(strsrch, start, *textoffset) ||
        !isBreakUnit(strsrch, start, *textoffset) ||
        !checkIdentical(strsrch, start, *textoffset)) {
        (*textoffset) ++;
        *textoffset = getNextBaseOffset(strsrch->search->text, *textoffset,
                                        strsrch->search->textLength);
        return FALSE;
    }

    strsrch->search->matchedIndex  = start;
    strsrch->search->matchedLength = *textoffset - start;
    return TRUE;
}














static
inline int32_t reverseShift(UStringSearch *strsrch,
                                int32_t    textoffset,
                                int32_t       ce,
                                int32_t        patternceindex)
{
    if (strsrch->search->isOverlap) {
        if (textoffset != strsrch->search->textLength) {
            textoffset --;
        }
        else {
            textoffset -= strsrch->pattern.defaultShiftSize;
        }
    }
    else {
        if (ce != UCOL_NULLORDER) {
            int32_t shift = strsrch->pattern.backShift[hash(ce)];

            
            
            int32_t adjust = patternceindex;
            if (adjust > 1 && shift > adjust) {
                shift -= adjust - 1;
            }
            textoffset -= shift;
        }
        else {
            textoffset -= strsrch->pattern.defaultShiftSize;
        }
    }
    textoffset = getPreviousUStringSearchBaseOffset(strsrch, textoffset);
    return textoffset;
}












static
UBool checkPreviousExactContractionMatch(UStringSearch *strsrch,
                                     int32_t   *start,
                                     int32_t   *end, UErrorCode  *status)
{
          UCollationElements *coleiter   = strsrch->textIter;
          int32_t             textlength = strsrch->search->textLength;
          int32_t             temp       = *end;
    const UCollator          *collator   = strsrch->collator;
    const UChar              *text       = strsrch->search->text;
    
    
    
    
    
    if (*start < textlength && ucol_unsafeCP(text[*start], collator)) {
        int32_t expansion  = getExpansionSuffix(coleiter);
        UBool   expandflag = expansion > 0;
        setColEIterOffset(coleiter, *end);
        while (U_SUCCESS(*status) && expansion > 0) {
            
            
            
            
            
            
            
            ucol_previous(coleiter, status);
            if (U_FAILURE(*status)) {
                return FALSE;
            }
            if (ucol_getOffset(coleiter) != temp) {
                *end = temp;
                temp  = ucol_getOffset(coleiter);
            }
            expansion --;
        }

        int32_t  *patternce       = strsrch->pattern.CE;
        int32_t   patterncelength = strsrch->pattern.CELength;
        int32_t   count           = patterncelength;
        while (count > 0) {
            int32_t ce = getCE(strsrch, ucol_previous(coleiter, status));
            
            
            if (ce == UCOL_IGNORABLE) {
                continue;
            }
            if (expandflag && count == 0 &&
                getColElemIterOffset(coleiter, FALSE) != temp) {
                *end = temp;
                temp  = ucol_getOffset(coleiter);
            }
            if (U_FAILURE(*status) || ce != patternce[count - 1]) {
                (*start) --;
                *start = getPreviousBaseOffset(text, *start);
                return FALSE;
            }
            count --;
        }
    }
    return TRUE;
}























static
inline UBool checkPreviousExactMatch(UStringSearch *strsrch,
                                     int32_t   *textoffset,
                                     UErrorCode    *status)
{
    
    int32_t end = ucol_getOffset(strsrch->textIter);
    if (!checkPreviousExactContractionMatch(strsrch, textoffset, &end, status)
        || U_FAILURE(*status)) {
            return FALSE;
    }

    
    
    if (checkRepeatedMatch(strsrch, *textoffset, end) ||
        !isBreakUnit(strsrch, *textoffset, end) ||
        hasAccentsBeforeMatch(strsrch, *textoffset, end) ||
        !checkIdentical(strsrch, *textoffset, end) ||
        hasAccentsAfterMatch(strsrch, *textoffset, end)) {
        (*textoffset) --;
        *textoffset = getPreviousBaseOffset(strsrch->search->text,
                                            *textoffset);
        return FALSE;
    }

    
    if (!strsrch->search->breakIter && strsrch->strength == UCOL_PRIMARY) {
        checkBreakBoundary(strsrch, textoffset, &end);
    }

    strsrch->search->matchedIndex = *textoffset;
    strsrch->search->matchedLength = end - *textoffset;
    return TRUE;
}




















static
int32_t doPreviousCanonicalSuffixMatch(UStringSearch *strsrch,
                                           int32_t    start,
                                           int32_t    end,
                                           UErrorCode    *status)
{
    const UChar       *text       = strsrch->search->text;
          int32_t  tempend    = end;

    U16_BACK_1(text, 0, tempend);
    if (!(getFCD(text, &tempend, strsrch->search->textLength) &
                                                           LAST_BYTE_MASK_)) {
        
        return USEARCH_DONE;
    }
    end = getNextBaseOffset(text, end, strsrch->search->textLength);

    if (U_SUCCESS(*status)) {
        UChar       accents[INITIAL_ARRAY_SIZE_];
        int32_t offset = getPreviousBaseOffset(text, end);
        
        unorm_normalize(text + offset, end - offset, UNORM_NFD, 0, accents,
                        INITIAL_ARRAY_SIZE_, status);

        int32_t         accentsindex[INITIAL_ARRAY_SIZE_];
        int32_t         accentsize = getUnblockedAccentIndex(accents,
                                                         accentsindex);
        int32_t         count      = (2 << (accentsize - 1)) - 1;
        UChar               buffer[INITIAL_ARRAY_SIZE_];
        UCollationElements *coleiter = strsrch->utilIter;
        while (U_SUCCESS(*status) && count > 0) {
            UChar *rearrange = strsrch->canonicalSuffixAccents;
            
            for (int k = 0; k < accentsindex[0]; k ++) {
                *rearrange ++ = accents[k];
            }
            
            
            for (int i = 0; i <= accentsize - 1; i ++) {
                int32_t mask = 1 << (accentsize - i - 1);
                if (count & mask) {
                    for (int j = accentsindex[i]; j < accentsindex[i + 1]; j ++) {
                        *rearrange ++ = accents[j];
                    }
                }
            }
            *rearrange = 0;
            int32_t  matchsize = INITIAL_ARRAY_SIZE_;
            UChar   *match     = addToUCharArray(buffer, &matchsize,
                                           strsrch->canonicalPrefixAccents,
                                           strsrch->search->text + start,
                                           offset - start,
                                           strsrch->canonicalSuffixAccents,
                                           status);

            
            
            ucol_setText(coleiter, match, matchsize, status);
            if (U_SUCCESS(*status)) {
                if (checkCollationMatch(strsrch, coleiter)) {
                    if (match != buffer) {
                        uprv_free(match);
                    }
                    return end;
                }
            }
            count --;
        }
    }
    return USEARCH_DONE;
}
















static
int32_t doPreviousCanonicalPrefixMatch(UStringSearch *strsrch,
                                           int32_t    textoffset,
                                           UErrorCode    *status)
{
    const UChar       *text       = strsrch->search->text;
    const UCollator   *collator   = strsrch->collator;
          int32_t      safelength = 0;
          UChar       *safetext;
          int32_t      safetextlength;
          UChar        safebuffer[INITIAL_ARRAY_SIZE_];
          int32_t  safeoffset = textoffset;

    if (textoffset &&
        ucol_unsafeCP(strsrch->canonicalPrefixAccents[
                                 u_strlen(strsrch->canonicalPrefixAccents) - 1
                                         ], collator)) {
        safeoffset     = getNextSafeOffset(collator, text, textoffset,
                                           strsrch->search->textLength);
        safelength     = safeoffset - textoffset;
        safetextlength = INITIAL_ARRAY_SIZE_;
        safetext       = addToUCharArray(safebuffer, &safetextlength,
                                         strsrch->canonicalPrefixAccents,
                                         text + textoffset, safelength,
                                         NULL, status);
    }
    else {
        safetextlength = u_strlen(strsrch->canonicalPrefixAccents);
        safetext       = strsrch->canonicalPrefixAccents;
    }

    UCollationElements *coleiter = strsrch->utilIter;
     
    ucol_setText(coleiter, safetext, safetextlength, status);
    

    int32_t  *ce           = strsrch->pattern.CE;
    int32_t   celength     = strsrch->pattern.CELength;
    int       ceindex      = 0;
    UBool     isSafe       = TRUE; 
    int32_t   prefixlength = u_strlen(strsrch->canonicalPrefixAccents);

    while (ceindex < celength) {
        int32_t textce = ucol_next(coleiter, status);
        if (U_FAILURE(*status)) {
            if (isSafe) {
                cleanUpSafeText(strsrch, safetext, safebuffer);
            }
            return USEARCH_DONE;
        }
        if (textce == UCOL_NULLORDER) {
            
            if (coleiter == strsrch->textIter) {
                cleanUpSafeText(strsrch, safetext, safebuffer);
                return USEARCH_DONE;
            }
            cleanUpSafeText(strsrch, safetext, safebuffer);
            safetext = safebuffer;
            coleiter = strsrch->textIter;
            setColEIterOffset(coleiter, safeoffset);
            
            isSafe = FALSE;
            continue;
        }
        textce = getCE(strsrch, textce);
        if (textce != UCOL_IGNORABLE && textce != ce[ceindex]) {
            
            int32_t failedoffset = ucol_getOffset(coleiter);
            if (isSafe && failedoffset <= prefixlength) {
                
                cleanUpSafeText(strsrch, safetext, safebuffer);
                return USEARCH_DONE;
            }
            else {
                if (isSafe) {
                    failedoffset = safeoffset - failedoffset;
                    cleanUpSafeText(strsrch, safetext, safebuffer);
                }

                
                int32_t result = doPreviousCanonicalSuffixMatch(strsrch,
                                        textoffset, failedoffset, status);
                if (result != USEARCH_DONE) {
                    
                    setColEIterOffset(strsrch->textIter, result);
                }
                if (U_FAILURE(*status)) {
                    return USEARCH_DONE;
                }
                return result;
            }
        }
        if (textce == ce[ceindex]) {
            ceindex ++;
        }
    }
    
    if (isSafe) {
        int32_t result      = ucol_getOffset(coleiter);
        
        int32_t     leftoverces = getExpansionSuffix(coleiter);
        cleanUpSafeText(strsrch, safetext, safebuffer);
        if (result <= prefixlength) {
            result = textoffset;
        }
        else {
            result = textoffset + (safeoffset - result);
        }
        setColEIterOffset(strsrch->textIter, result);
        setExpansionSuffix(strsrch->textIter, leftoverces);
        return result;
    }

    return ucol_getOffset(coleiter);
}





















static
UBool doPreviousCanonicalMatch(UStringSearch *strsrch,
                               int32_t    textoffset,
                               UErrorCode    *status)
{
    const UChar       *text       = strsrch->search->text;
          int32_t  temp       = textoffset;
          int32_t      textlength = strsrch->search->textLength;
    if ((getFCD(text, &temp, textlength) >> SECOND_LAST_BYTE_SHIFT_) == 0) {
        UCollationElements *coleiter = strsrch->textIter;
        int32_t         offset   = ucol_getOffset(coleiter);
        if (strsrch->pattern.hasSuffixAccents) {
            offset = doPreviousCanonicalSuffixMatch(strsrch, textoffset,
                                                    offset, status);
            if (U_SUCCESS(*status) && offset != USEARCH_DONE) {
                setColEIterOffset(coleiter, offset);
                return TRUE;
            }
        }
        return FALSE;
    }

    if (!strsrch->pattern.hasPrefixAccents) {
        return FALSE;
    }

    UChar       accents[INITIAL_ARRAY_SIZE_];
    
    int32_t baseoffset = getNextBaseOffset(text, textoffset, textlength);
    
    unorm_normalize(text + textoffset, baseoffset - textoffset, UNORM_NFD,
                               0, accents, INITIAL_ARRAY_SIZE_, status);
    

    int32_t accentsindex[INITIAL_ARRAY_SIZE_];
    int32_t size = getUnblockedAccentIndex(accents, accentsindex);

    
    int32_t  count = (2 << (size - 1)) - 1;
    while (U_SUCCESS(*status) && count > 0) {
        UChar *rearrange = strsrch->canonicalPrefixAccents;
        
        for (int k = 0; k < accentsindex[0]; k ++) {
            *rearrange ++ = accents[k];
        }
        
        
        for (int i = 0; i <= size - 1; i ++) {
            int32_t mask = 1 << (size - i - 1);
            if (count & mask) {
                for (int j = accentsindex[i]; j < accentsindex[i + 1]; j ++) {
                    *rearrange ++ = accents[j];
                }
            }
        }
        *rearrange = 0;
        int32_t offset = doPreviousCanonicalPrefixMatch(strsrch,
                                                          baseoffset, status);
        if (offset != USEARCH_DONE) {
            return TRUE; 
        }
        count --;
    }
    return FALSE;
}












static
UBool checkPreviousCanonicalContractionMatch(UStringSearch *strsrch,
                                     int32_t   *start,
                                     int32_t   *end, UErrorCode  *status)
{
          UCollationElements *coleiter   = strsrch->textIter;
          int32_t             textlength = strsrch->search->textLength;
          int32_t         temp       = *end;
    const UCollator          *collator   = strsrch->collator;
    const UChar              *text       = strsrch->search->text;
    
    
    
    
    
    if (*start < textlength && ucol_unsafeCP(text[*start], collator)) {
        int32_t expansion  = getExpansionSuffix(coleiter);
        UBool   expandflag = expansion > 0;
        setColEIterOffset(coleiter, *end);
        while (expansion > 0) {
            
            
            
            
            
            
            
            ucol_previous(coleiter, status);
            if (U_FAILURE(*status)) {
                return FALSE;
            }
            if (ucol_getOffset(coleiter) != temp) {
                *end = temp;
                temp  = ucol_getOffset(coleiter);
            }
            expansion --;
        }

        int32_t  *patternce       = strsrch->pattern.CE;
        int32_t   patterncelength = strsrch->pattern.CELength;
        int32_t   count           = patterncelength;
        while (count > 0) {
            int32_t ce = getCE(strsrch, ucol_previous(coleiter, status));
            
            
            if (ce == UCOL_IGNORABLE) {
                continue;
            }
            if (expandflag && count == 0 &&
                getColElemIterOffset(coleiter, FALSE) != temp) {
                *end = temp;
                temp  = ucol_getOffset(coleiter);
            }
            if (count == patterncelength &&
                ce != patternce[patterncelength - 1]) {
                
                
                int32_t    expected = patternce[patterncelength - 1];
                U16_BACK_1(text, 0, *end);
                if (getFCD(text, end, textlength) & LAST_BYTE_MASK_) {
                    ce = getCE(strsrch, ucol_previous(coleiter, status));
                    while (U_SUCCESS(*status) && ce != expected &&
                           ce != UCOL_NULLORDER &&
                           ucol_getOffset(coleiter) <= *start) {
                        ce = getCE(strsrch, ucol_previous(coleiter, status));
                    }
                }
            }
            if (U_FAILURE(*status) || ce != patternce[count - 1]) {
                (*start) --;
                *start = getPreviousBaseOffset(text, *start);
                return FALSE;
            }
            count --;
        }
    }
    return TRUE;
}




















static
inline UBool checkPreviousCanonicalMatch(UStringSearch *strsrch,
                                         int32_t   *textoffset,
                                         UErrorCode    *status)
{
    
    UCollationElements *coleiter = strsrch->textIter;
    
    if ((strsrch->pattern.hasSuffixAccents &&
        strsrch->canonicalSuffixAccents[0]) ||
        (strsrch->pattern.hasPrefixAccents &&
        strsrch->canonicalPrefixAccents[0])) {
        strsrch->search->matchedIndex  = *textoffset;
        strsrch->search->matchedLength =
            getNextUStringSearchBaseOffset(strsrch,
                                      getColElemIterOffset(coleiter, FALSE))
            - *textoffset;
        return TRUE;
    }

    int32_t end = ucol_getOffset(coleiter);
    if (!checkPreviousCanonicalContractionMatch(strsrch, textoffset, &end,
                                                status) ||
         U_FAILURE(*status)) {
        return FALSE;
    }

    end = getNextUStringSearchBaseOffset(strsrch, end);
    
    if (checkRepeatedMatch(strsrch, *textoffset, end) ||
        !isBreakUnit(strsrch, *textoffset, end) ||
        !checkIdentical(strsrch, *textoffset, end)) {
        (*textoffset) --;
        *textoffset = getPreviousBaseOffset(strsrch->search->text,
                                            *textoffset);
        return FALSE;
    }

    strsrch->search->matchedIndex  = *textoffset;
    strsrch->search->matchedLength = end - *textoffset;
    return TRUE;
}
#endif 



U_CAPI UStringSearch * U_EXPORT2 usearch_open(const UChar *pattern,
                                          int32_t         patternlength,
                                    const UChar          *text,
                                          int32_t         textlength,
                                    const char           *locale,
                                          UBreakIterator *breakiter,
                                          UErrorCode     *status)
{
    if (U_FAILURE(*status)) {
        return NULL;
    }
#if UCONFIG_NO_BREAK_ITERATION
    if (breakiter != NULL) {
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    }
#endif
    if (locale) {
        
        UCollator     *collator = ucol_open(locale, status);
        
        UStringSearch *result   = usearch_openFromCollator(pattern,
                                              patternlength, text, textlength,
                                              collator, breakiter, status);

        if (result == NULL || U_FAILURE(*status)) {
            if (collator) {
                ucol_close(collator);
            }
            return NULL;
        }
        else {
            result->ownCollator = TRUE;
        }
        return result;
    }
    *status = U_ILLEGAL_ARGUMENT_ERROR;
    return NULL;
}

U_CAPI UStringSearch * U_EXPORT2 usearch_openFromCollator(
                                  const UChar          *pattern,
                                        int32_t         patternlength,
                                  const UChar          *text,
                                        int32_t         textlength,
                                  const UCollator      *collator,
                                        UBreakIterator *breakiter,
                                        UErrorCode     *status)
{
    if (U_FAILURE(*status)) {
        return NULL;
    }
#if UCONFIG_NO_BREAK_ITERATION
    if (breakiter != NULL) {
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    }
#endif
    if (pattern == NULL || text == NULL || collator == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    
    if(ucol_getAttribute(collator, UCOL_NUMERIC_COLLATION, status) == UCOL_ON) {
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    }

    if (U_SUCCESS(*status)) {
        initializeFCD(status);
        if (U_FAILURE(*status)) {
            return NULL;
        }

        UStringSearch *result;
        if (textlength == -1) {
            textlength = u_strlen(text);
        }
        if (patternlength == -1) {
            patternlength = u_strlen(pattern);
        }
        if (textlength <= 0 || patternlength <= 0) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return NULL;
        }

        result = (UStringSearch *)uprv_malloc(sizeof(UStringSearch));
        if (result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }

        result->collator    = collator;
        result->strength    = ucol_getStrength(collator);
        result->ceMask      = getMask(result->strength);
        result->toShift     =
             ucol_getAttribute(collator, UCOL_ALTERNATE_HANDLING, status) ==
                                                            UCOL_SHIFTED;
        result->variableTop = ucol_getVariableTop(collator, status);

        result->nfd         = Normalizer2Factory::getNFDInstance(*status);

        if (U_FAILURE(*status)) {
            uprv_free(result);
            return NULL;
        }

        result->search             = (USearch *)uprv_malloc(sizeof(USearch));
        if (result->search == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(result);
            return NULL;
        }

        result->search->text       = text;
        result->search->textLength = textlength;

        result->pattern.text       = pattern;
        result->pattern.textLength = patternlength;
        result->pattern.CE         = NULL;
        result->pattern.PCE        = NULL;

        result->search->breakIter  = breakiter;
#if !UCONFIG_NO_BREAK_ITERATION
        result->search->internalBreakIter = ubrk_open(UBRK_CHARACTER, ucol_getLocaleByType(result->collator, ULOC_VALID_LOCALE, status), text, textlength, status);
        if (breakiter) {
            ubrk_setText(breakiter, text, textlength, status);
        }
#endif

        result->ownCollator           = FALSE;
        result->search->matchedLength = 0;
        result->search->matchedIndex  = USEARCH_DONE;
        result->utilIter              = NULL;
        result->textIter              = ucol_openElements(collator, text,
                                                          textlength, status);
        if (U_FAILURE(*status)) {
            usearch_close(result);
            return NULL;
        }

        result->search->isOverlap          = FALSE;
        result->search->isCanonicalMatch   = FALSE;
        result->search->elementComparisonType = 0;
        result->search->isForwardSearching = TRUE;
        result->search->reset              = TRUE;

        initialize(result, status);

        if (U_FAILURE(*status)) {
            usearch_close(result);
            return NULL;
        }

        return result;
    }
    return NULL;
}

U_CAPI void U_EXPORT2 usearch_close(UStringSearch *strsrch)
{
    if (strsrch) {
        if (strsrch->pattern.CE != strsrch->pattern.CEBuffer &&
            strsrch->pattern.CE) {
            uprv_free(strsrch->pattern.CE);
        }

        if (strsrch->pattern.PCE != NULL &&
            strsrch->pattern.PCE != strsrch->pattern.PCEBuffer) {
            uprv_free(strsrch->pattern.PCE);
        }

        ucol_closeElements(strsrch->textIter);
        ucol_closeElements(strsrch->utilIter);

        if (strsrch->ownCollator && strsrch->collator) {
            ucol_close((UCollator *)strsrch->collator);
        }

#if !UCONFIG_NO_BREAK_ITERATION
        if (strsrch->search->internalBreakIter) {
            ubrk_close(strsrch->search->internalBreakIter);
        }
#endif

        uprv_free(strsrch->search);
        uprv_free(strsrch);
    }
}



U_CAPI void U_EXPORT2 usearch_setOffset(UStringSearch *strsrch,
                                        int32_t    position,
                                        UErrorCode    *status)
{
    if (U_SUCCESS(*status) && strsrch) {
        if (isOutOfBounds(strsrch->search->textLength, position)) {
            *status = U_INDEX_OUTOFBOUNDS_ERROR;
        }
        else {
            setColEIterOffset(strsrch->textIter, position);
        }
        strsrch->search->matchedIndex  = USEARCH_DONE;
        strsrch->search->matchedLength = 0;
        strsrch->search->reset         = FALSE;
    }
}

U_CAPI int32_t U_EXPORT2 usearch_getOffset(const UStringSearch *strsrch)
{
    if (strsrch) {
        int32_t result = ucol_getOffset(strsrch->textIter);
        if (isOutOfBounds(strsrch->search->textLength, result)) {
            return USEARCH_DONE;
        }
        return result;
    }
    return USEARCH_DONE;
}

U_CAPI void U_EXPORT2 usearch_setAttribute(UStringSearch *strsrch,
                                 USearchAttribute attribute,
                                 USearchAttributeValue value,
                                 UErrorCode *status)
{
    if (U_SUCCESS(*status) && strsrch) {
        switch (attribute)
        {
        case USEARCH_OVERLAP :
            strsrch->search->isOverlap = (value == USEARCH_ON ? TRUE : FALSE);
            break;
        case USEARCH_CANONICAL_MATCH :
            strsrch->search->isCanonicalMatch = (value == USEARCH_ON ? TRUE :
                                                                      FALSE);
            break;
        case USEARCH_ELEMENT_COMPARISON :
            if (value == USEARCH_PATTERN_BASE_WEIGHT_IS_WILDCARD || value == USEARCH_ANY_BASE_WEIGHT_IS_WILDCARD) {
                strsrch->search->elementComparisonType = (int16_t)value;
            } else {
                strsrch->search->elementComparisonType = 0;
            }
            break;
        case USEARCH_ATTRIBUTE_COUNT :
        default:
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
    }
    if (value == USEARCH_ATTRIBUTE_VALUE_COUNT) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
    }
}

U_CAPI USearchAttributeValue U_EXPORT2 usearch_getAttribute(
                                                const UStringSearch *strsrch,
                                                USearchAttribute attribute)
{
    if (strsrch) {
        switch (attribute) {
        case USEARCH_OVERLAP :
            return (strsrch->search->isOverlap == TRUE ? USEARCH_ON :
                                                        USEARCH_OFF);
        case USEARCH_CANONICAL_MATCH :
            return (strsrch->search->isCanonicalMatch == TRUE ? USEARCH_ON :
                                                               USEARCH_OFF);
        case USEARCH_ELEMENT_COMPARISON :
            {
                int16_t value = strsrch->search->elementComparisonType;
                if (value == USEARCH_PATTERN_BASE_WEIGHT_IS_WILDCARD || value == USEARCH_ANY_BASE_WEIGHT_IS_WILDCARD) {
                    return (USearchAttributeValue)value;
                } else {
                    return USEARCH_STANDARD_ELEMENT_COMPARISON;
                }
            }
        case USEARCH_ATTRIBUTE_COUNT :
            return USEARCH_DEFAULT;
        }
    }
    return USEARCH_DEFAULT;
}

U_CAPI int32_t U_EXPORT2 usearch_getMatchedStart(
                                                const UStringSearch *strsrch)
{
    if (strsrch == NULL) {
        return USEARCH_DONE;
    }
    return strsrch->search->matchedIndex;
}


U_CAPI int32_t U_EXPORT2 usearch_getMatchedText(const UStringSearch *strsrch,
                                            UChar         *result,
                                            int32_t        resultCapacity,
                                            UErrorCode    *status)
{
    if (U_FAILURE(*status)) {
        return USEARCH_DONE;
    }
    if (strsrch == NULL || resultCapacity < 0 || (resultCapacity > 0 &&
        result == NULL)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return USEARCH_DONE;
    }

    int32_t     copylength = strsrch->search->matchedLength;
    int32_t copyindex  = strsrch->search->matchedIndex;
    if (copyindex == USEARCH_DONE) {
        u_terminateUChars(result, resultCapacity, 0, status);
        return USEARCH_DONE;
    }

    if (resultCapacity < copylength) {
        copylength = resultCapacity;
    }
    if (copylength > 0) {
        uprv_memcpy(result, strsrch->search->text + copyindex,
                    copylength * sizeof(UChar));
    }
    return u_terminateUChars(result, resultCapacity,
                             strsrch->search->matchedLength, status);
}

U_CAPI int32_t U_EXPORT2 usearch_getMatchedLength(
                                              const UStringSearch *strsrch)
{
    if (strsrch) {
        return strsrch->search->matchedLength;
    }
    return USEARCH_DONE;
}

#if !UCONFIG_NO_BREAK_ITERATION

U_CAPI void U_EXPORT2 usearch_setBreakIterator(UStringSearch  *strsrch,
                                               UBreakIterator *breakiter,
                                               UErrorCode     *status)
{
    if (U_SUCCESS(*status) && strsrch) {
        strsrch->search->breakIter = breakiter;
        if (breakiter) {
            ubrk_setText(breakiter, strsrch->search->text,
                         strsrch->search->textLength, status);
        }
    }
}

U_CAPI const UBreakIterator* U_EXPORT2
usearch_getBreakIterator(const UStringSearch *strsrch)
{
    if (strsrch) {
        return strsrch->search->breakIter;
    }
    return NULL;
}

#endif

U_CAPI void U_EXPORT2 usearch_setText(      UStringSearch *strsrch,
                                      const UChar         *text,
                                            int32_t        textlength,
                                            UErrorCode    *status)
{
    if (U_SUCCESS(*status)) {
        if (strsrch == NULL || text == NULL || textlength < -1 ||
            textlength == 0) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        else {
            if (textlength == -1) {
                textlength = u_strlen(text);
            }
            strsrch->search->text       = text;
            strsrch->search->textLength = textlength;
            ucol_setText(strsrch->textIter, text, textlength, status);
            strsrch->search->matchedIndex  = USEARCH_DONE;
            strsrch->search->matchedLength = 0;
            strsrch->search->reset         = TRUE;
#if !UCONFIG_NO_BREAK_ITERATION
            if (strsrch->search->breakIter != NULL) {
                ubrk_setText(strsrch->search->breakIter, text,
                             textlength, status);
            }
            ubrk_setText(strsrch->search->internalBreakIter, text, textlength, status);
#endif
        }
    }
}

U_CAPI const UChar * U_EXPORT2 usearch_getText(const UStringSearch *strsrch,
                                                     int32_t       *length)
{
    if (strsrch) {
        *length = strsrch->search->textLength;
        return strsrch->search->text;
    }
    return NULL;
}

U_CAPI void U_EXPORT2 usearch_setCollator(      UStringSearch *strsrch,
                                          const UCollator     *collator,
                                                UErrorCode    *status)
{
    if (U_SUCCESS(*status)) {
        if (collator == NULL) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }

        if (strsrch) {
            if (strsrch->ownCollator && (strsrch->collator != collator)) {
                ucol_close((UCollator *)strsrch->collator);
                strsrch->ownCollator = FALSE;
            }
            strsrch->collator    = collator;
            strsrch->strength    = ucol_getStrength(collator);
            strsrch->ceMask      = getMask(strsrch->strength);
#if !UCONFIG_NO_BREAK_ITERATION
            ubrk_close(strsrch->search->internalBreakIter);
            strsrch->search->internalBreakIter = ubrk_open(UBRK_CHARACTER, ucol_getLocaleByType(collator, ULOC_VALID_LOCALE, status),
                                                     strsrch->search->text, strsrch->search->textLength, status);
#endif
            
            strsrch->toShift     =
               ucol_getAttribute(collator, UCOL_ALTERNATE_HANDLING, status) ==
                                                                UCOL_SHIFTED;
            
            strsrch->variableTop = ucol_getVariableTop(collator, status);
            if (U_SUCCESS(*status)) {
                initialize(strsrch, status);
                if (U_SUCCESS(*status)) {
                    
                    ucol_freeOffsetBuffer(&(strsrch->textIter->iteratordata_));
                    uprv_init_collIterate(collator, strsrch->search->text,
                                          strsrch->search->textLength,
                                          &(strsrch->textIter->iteratordata_),
                                          status);
                    strsrch->utilIter->iteratordata_.coll = collator;
                }
            }
        }

        
        
        
#if 0
        uprv_init_pce(strsrch->textIter);
        uprv_init_pce(strsrch->utilIter);
#endif
    }
}

U_CAPI UCollator * U_EXPORT2 usearch_getCollator(const UStringSearch *strsrch)
{
    if (strsrch) {
        return (UCollator *)strsrch->collator;
    }
    return NULL;
}

U_CAPI void U_EXPORT2 usearch_setPattern(      UStringSearch *strsrch,
                                         const UChar         *pattern,
                                               int32_t        patternlength,
                                               UErrorCode    *status)
{
    if (U_SUCCESS(*status)) {
        if (strsrch == NULL || pattern == NULL) {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        else {
            if (patternlength == -1) {
                patternlength = u_strlen(pattern);
            }
            if (patternlength == 0) {
                *status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            strsrch->pattern.text       = pattern;
            strsrch->pattern.textLength = patternlength;
            initialize(strsrch, status);
        }
    }
}

U_CAPI const UChar* U_EXPORT2
usearch_getPattern(const UStringSearch *strsrch,
                   int32_t       *length)
{
    if (strsrch) {
        *length = strsrch->pattern.textLength;
        return strsrch->pattern.text;
    }
    return NULL;
}



U_CAPI int32_t U_EXPORT2 usearch_first(UStringSearch *strsrch,
                                           UErrorCode    *status)
{
    if (strsrch && U_SUCCESS(*status)) {
        strsrch->search->isForwardSearching = TRUE;
        usearch_setOffset(strsrch, 0, status);
        if (U_SUCCESS(*status)) {
            return usearch_next(strsrch, status);
        }
    }
    return USEARCH_DONE;
}

U_CAPI int32_t U_EXPORT2 usearch_following(UStringSearch *strsrch,
                                               int32_t    position,
                                               UErrorCode    *status)
{
    if (strsrch && U_SUCCESS(*status)) {
        strsrch->search->isForwardSearching = TRUE;
        
        usearch_setOffset(strsrch, position, status);
        if (U_SUCCESS(*status)) {
            return usearch_next(strsrch, status);
        }
    }
    return USEARCH_DONE;
}

U_CAPI int32_t U_EXPORT2 usearch_last(UStringSearch *strsrch,
                                          UErrorCode    *status)
{
    if (strsrch && U_SUCCESS(*status)) {
        strsrch->search->isForwardSearching = FALSE;
        usearch_setOffset(strsrch, strsrch->search->textLength, status);
        if (U_SUCCESS(*status)) {
            return usearch_previous(strsrch, status);
        }
    }
    return USEARCH_DONE;
}

U_CAPI int32_t U_EXPORT2 usearch_preceding(UStringSearch *strsrch,
                                               int32_t    position,
                                               UErrorCode    *status)
{
    if (strsrch && U_SUCCESS(*status)) {
        strsrch->search->isForwardSearching = FALSE;
        
        usearch_setOffset(strsrch, position, status);
        if (U_SUCCESS(*status)) {
            return usearch_previous(strsrch, status);
        }
    }
    return USEARCH_DONE;
}























U_CAPI int32_t U_EXPORT2 usearch_next(UStringSearch *strsrch,
                                          UErrorCode    *status)
{
    if (U_SUCCESS(*status) && strsrch) {
        
        
        int32_t      offset       = usearch_getOffset(strsrch);
        USearch     *search       = strsrch->search;
        search->reset             = FALSE;
        int32_t      textlength   = search->textLength;
        if (search->isForwardSearching) {
#if BOYER_MOORE
            if (offset == textlength
                || (!search->isOverlap &&
                    (offset + strsrch->pattern.defaultShiftSize > textlength ||
                    (search->matchedIndex != USEARCH_DONE &&
                     offset + search->matchedLength >= textlength)))) {
                
                setMatchNotFound(strsrch);
                return USEARCH_DONE;
            }
#else
            if (offset == textlength ||
                (! search->isOverlap &&
                (search->matchedIndex != USEARCH_DONE &&
                offset + search->matchedLength > textlength))) {
                    
                    setMatchNotFound(strsrch);
                    return USEARCH_DONE;
            }
#endif
        }
        else {
            
            
            
            
            
            search->isForwardSearching = TRUE;
            if (search->matchedIndex != USEARCH_DONE) {
                
                
                return search->matchedIndex;
            }
        }

        if (U_SUCCESS(*status)) {
            if (strsrch->pattern.CELength == 0) {
                if (search->matchedIndex == USEARCH_DONE) {
                    search->matchedIndex = offset;
                }
                else { 
                    U16_FWD_1(search->text, search->matchedIndex, textlength);
                }

                search->matchedLength = 0;
                setColEIterOffset(strsrch->textIter, search->matchedIndex);
                
                if (search->matchedIndex == textlength) {
                    search->matchedIndex = USEARCH_DONE;
                }
            }
            else {
                if (search->matchedLength > 0) {
                    
                    if (search->isOverlap) {
                        ucol_setOffset(strsrch->textIter, offset + 1, status);
                    }
                    else {
                        ucol_setOffset(strsrch->textIter,
                                       offset + search->matchedLength, status);
                    }
                }
                else {
                    
                    
                    
                    
                    search->matchedIndex = offset - 1;
                }

                if (search->isCanonicalMatch) {
                    
                    usearch_handleNextCanonical(strsrch, status);
                }
                else {
                    usearch_handleNextExact(strsrch, status);
                }
            }

            if (U_FAILURE(*status)) {
                return USEARCH_DONE;
            }

#if !BOYER_MOORE
            if (search->matchedIndex == USEARCH_DONE) {
                ucol_setOffset(strsrch->textIter, search->textLength, status);
            } else {
                ucol_setOffset(strsrch->textIter, search->matchedIndex, status);
            }
#endif

            return search->matchedIndex;
        }
    }
    return USEARCH_DONE;
}

U_CAPI int32_t U_EXPORT2 usearch_previous(UStringSearch *strsrch,
                                              UErrorCode *status)
{
    if (U_SUCCESS(*status) && strsrch) {
        int32_t offset;
        USearch *search = strsrch->search;
        if (search->reset) {
            offset                     = search->textLength;
            search->isForwardSearching = FALSE;
            search->reset              = FALSE;
            setColEIterOffset(strsrch->textIter, offset);
        }
        else {
            offset = usearch_getOffset(strsrch);
        }

        int32_t matchedindex = search->matchedIndex;
        if (search->isForwardSearching == TRUE) {
            
            
            
            
            
            search->isForwardSearching = FALSE;
            if (matchedindex != USEARCH_DONE) {
                return matchedindex;
            }
        }
        else {
#if BOYER_MOORE
            if (offset == 0 || matchedindex == 0 ||
                (!search->isOverlap &&
                    (offset < strsrch->pattern.defaultShiftSize ||
                    (matchedindex != USEARCH_DONE &&
                    matchedindex < strsrch->pattern.defaultShiftSize)))) {
                
                setMatchNotFound(strsrch);
                return USEARCH_DONE;
            }
#else
            
            
            if (offset == 0 || matchedindex == 0) {
                setMatchNotFound(strsrch);
                return USEARCH_DONE;
            }
#endif
        }

        if (U_SUCCESS(*status)) {
            if (strsrch->pattern.CELength == 0) {
                search->matchedIndex =
                      (matchedindex == USEARCH_DONE ? offset : matchedindex);
                if (search->matchedIndex == 0) {
                    setMatchNotFound(strsrch);
                    
                }
                else { 
                    U16_BACK_1(search->text, 0, search->matchedIndex);
                    setColEIterOffset(strsrch->textIter, search->matchedIndex);
                    
                    search->matchedLength = 0;
                }
            }
            else {
                if (strsrch->search->isCanonicalMatch) {
                    
                    usearch_handlePreviousCanonical(strsrch, status);
                    
                }
                else {
                    usearch_handlePreviousExact(strsrch, status);
                    
                }
            }

            if (U_FAILURE(*status)) {
                return USEARCH_DONE;
            }

            return search->matchedIndex;
        }
    }
    return USEARCH_DONE;
}



U_CAPI void U_EXPORT2 usearch_reset(UStringSearch *strsrch)
{
    




    if (strsrch) {
        UErrorCode status            = U_ZERO_ERROR;
        UBool      sameCollAttribute = TRUE;
        uint32_t   ceMask;
        UBool      shift;
        uint32_t   varTop;

        
        UCollationStrength newStrength = ucol_getStrength(strsrch->collator);
        if ((strsrch->strength < UCOL_QUATERNARY && newStrength >= UCOL_QUATERNARY) ||
            (strsrch->strength >= UCOL_QUATERNARY && newStrength < UCOL_QUATERNARY)) {
                sameCollAttribute = FALSE;
        }

        strsrch->strength    = ucol_getStrength(strsrch->collator);
        ceMask = getMask(strsrch->strength);
        if (strsrch->ceMask != ceMask) {
            strsrch->ceMask = ceMask;
            sameCollAttribute = FALSE;
        }

        
        shift = ucol_getAttribute(strsrch->collator, UCOL_ALTERNATE_HANDLING,
                                  &status) == UCOL_SHIFTED;
        if (strsrch->toShift != shift) {
            strsrch->toShift  = shift;
            sameCollAttribute = FALSE;
        }

        
        varTop = ucol_getVariableTop(strsrch->collator, &status);
        if (strsrch->variableTop != varTop) {
            strsrch->variableTop = varTop;
            sameCollAttribute    = FALSE;
        }
        if (!sameCollAttribute) {
            initialize(strsrch, &status);
        }
        
        ucol_freeOffsetBuffer(&(strsrch->textIter->iteratordata_));
        uprv_init_collIterate(strsrch->collator, strsrch->search->text,
                              strsrch->search->textLength,
                              &(strsrch->textIter->iteratordata_),
                              &status);
        strsrch->search->matchedLength      = 0;
        strsrch->search->matchedIndex       = USEARCH_DONE;
        strsrch->search->isOverlap          = FALSE;
        strsrch->search->isCanonicalMatch   = FALSE;
        strsrch->search->elementComparisonType = 0;
        strsrch->search->isForwardSearching = TRUE;
        strsrch->search->reset              = TRUE;
    }
}





struct  CEI {
    int64_t ce;
    int32_t lowIndex;
    int32_t highIndex;
};

U_NAMESPACE_BEGIN





#define   DEFAULT_CEBUFFER_SIZE 96
#define   CEBUFFER_EXTRA 32


#define   MAX_TARGET_IGNORABLES_PER_PAT_JAMO_L 8
#define   MAX_TARGET_IGNORABLES_PER_PAT_OTHER 3
#define   MIGHT_BE_JAMO_L(c) ((c >= 0x1100 && c <= 0x115E) || (c >= 0x3131 && c <= 0x314E) || (c >= 0x3165 && c <= 0x3186))
struct CEBuffer {
    CEI                  defBuf[DEFAULT_CEBUFFER_SIZE];
    CEI                 *buf;
    int32_t              bufSize;
    int32_t              firstIx;
    int32_t              limitIx;
    UCollationElements  *ceIter;
    UStringSearch       *strSearch;



               CEBuffer(UStringSearch *ss, UErrorCode *status);
               ~CEBuffer();
   const CEI   *get(int32_t index);
   const CEI   *getPrevious(int32_t index);
};


CEBuffer::CEBuffer(UStringSearch *ss, UErrorCode *status) {
    buf = defBuf;
    strSearch = ss;
    bufSize = ss->pattern.PCELength + CEBUFFER_EXTRA;
    if (ss->search->elementComparisonType != 0) {
        const UChar * patText = ss->pattern.text;
        if (patText) {
            const UChar * patTextLimit = patText + ss->pattern.textLength;
            while ( patText < patTextLimit ) {
                UChar c = *patText++;
                if (MIGHT_BE_JAMO_L(c)) {
                    bufSize += MAX_TARGET_IGNORABLES_PER_PAT_JAMO_L;
                } else {
                    
                    bufSize += MAX_TARGET_IGNORABLES_PER_PAT_OTHER;
                }
            }
        }
    }
    ceIter    = ss->textIter;
    firstIx = 0;
    limitIx = 0;

    uprv_init_pce(ceIter);

    if (bufSize>DEFAULT_CEBUFFER_SIZE) {
        buf = (CEI *)uprv_malloc(bufSize * sizeof(CEI));
        if (buf == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
}




CEBuffer::~CEBuffer() {
    if (buf != defBuf) {
        uprv_free(buf);
    }
}








const CEI *CEBuffer::get(int32_t index) {
    int i = index % bufSize;

    if (index>=firstIx && index<limitIx) {
        
        
        return &buf[i];
    }

    
    
    
    if (index != limitIx) {
        U_ASSERT(FALSE);

        return NULL;
    }

    
    limitIx++;

    if (limitIx - firstIx >= bufSize) {
        
        firstIx++;
    }

    UErrorCode status = U_ZERO_ERROR;

    buf[i].ce = ucol_nextProcessed(ceIter, &buf[i].lowIndex, &buf[i].highIndex, &status);

    return &buf[i];
}







const CEI *CEBuffer::getPrevious(int32_t index) {
    int i = index % bufSize;

    if (index>=firstIx && index<limitIx) {
        
        
        return &buf[i];
    }

    
    
    
    if (index != limitIx) {
        U_ASSERT(FALSE);

        return NULL;
    }

    
    limitIx++;

    if (limitIx - firstIx >= bufSize) {
        
        firstIx++;
    }

    UErrorCode status = U_ZERO_ERROR;

    buf[i].ce = ucol_previousProcessed(ceIter, &buf[i].lowIndex, &buf[i].highIndex, &status);

    return &buf[i];
}

U_NAMESPACE_END




#ifdef USEARCH_DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif






static int32_t nextBoundaryAfter(UStringSearch *strsrch, int32_t startIndex) {
#if 0
    const UChar *text = strsrch->search->text;
    int32_t textLen   = strsrch->search->textLength;

    U_ASSERT(startIndex>=0);
    U_ASSERT(startIndex<=textLen);

    if (startIndex >= textLen) {
        return startIndex;
    }

    UChar32  c;
    int32_t  i = startIndex;
    U16_NEXT(text, i, textLen, c);

    
    
    int32_t gcProperty = u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK);
    if (gcProperty==U_GCB_CONTROL || gcProperty==U_GCB_LF || gcProperty==U_GCB_CR) {
        return i;
    }

    
    
    int32_t  indexOfLastCharChecked;
    for (;;) {
        indexOfLastCharChecked = i;
        if (i>=textLen) {
            break;
        }
        U16_NEXT(text, i, textLen, c);
        gcProperty = u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK);
        if (gcProperty != U_GCB_EXTEND && gcProperty != U_GCB_SPACING_MARK) {
            break;
        }
    }
    return indexOfLastCharChecked;
#elif !UCONFIG_NO_BREAK_ITERATION
    UBreakIterator *breakiterator = strsrch->search->breakIter;

    if (breakiterator == NULL) {
        breakiterator = strsrch->search->internalBreakIter;
    }

    if (breakiterator != NULL) {
        return ubrk_following(breakiterator, startIndex);
    }

    return startIndex;
#else
    
    return startIndex;
#endif

}






static UBool isBreakBoundary(UStringSearch *strsrch, int32_t index) {
#if 0
    const UChar *text = strsrch->search->text;
    int32_t textLen   = strsrch->search->textLength;

    U_ASSERT(index>=0);
    U_ASSERT(index<=textLen);

    if (index>=textLen || index<=0) {
        return TRUE;
    }

    
    
    UChar32  c;
    U16_GET(text, 0, index, textLen, c);
    int32_t gcProperty = u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK);
    if (gcProperty != U_GCB_EXTEND && gcProperty != U_GCB_SPACING_MARK) {
        return TRUE;
    }

    
    
    U16_PREV(text, 0, index, c);
    gcProperty = u_getIntPropertyValue(c, UCHAR_GRAPHEME_CLUSTER_BREAK);
    UBool combining =  !(gcProperty==U_GCB_CONTROL || gcProperty==U_GCB_LF || gcProperty==U_GCB_CR);
    return !combining;
#elif !UCONFIG_NO_BREAK_ITERATION
    UBreakIterator *breakiterator = strsrch->search->breakIter;

    if (breakiterator == NULL) {
        breakiterator = strsrch->search->internalBreakIter;
    }

    return (breakiterator != NULL && ubrk_isBoundary(breakiterator, index));
#else
    
    return TRUE;
#endif
}

#if 0
static UBool onBreakBoundaries(const UStringSearch *strsrch, int32_t start, int32_t end)
{
#if !UCONFIG_NO_BREAK_ITERATION
    UBreakIterator *breakiterator = strsrch->search->breakIter;

    if (breakiterator != NULL) {
        int32_t startindex = ubrk_first(breakiterator);
        int32_t endindex   = ubrk_last(breakiterator);

        
        if (start < startindex || start > endindex ||
            end < startindex || end > endindex) {
            return FALSE;
        }

        return ubrk_isBoundary(breakiterator, start) &&
               ubrk_isBoundary(breakiterator, end);
    }
#endif

    return TRUE;
}
#endif

typedef enum {
    U_CE_MATCH = -1,
    U_CE_NO_MATCH = 0,
    U_CE_SKIP_TARG,
    U_CE_SKIP_PATN
} UCompareCEsResult;
#define U_CE_LEVEL2_BASE 0x00000005
#define U_CE_LEVEL3_BASE 0x00050000

static UCompareCEsResult compareCE64s(int64_t targCE, int64_t patCE, int16_t compareType) {
    if (targCE == patCE) {
        return U_CE_MATCH;
    }
    if (compareType == 0) {
        return U_CE_NO_MATCH;
    }
    
    int64_t targCEshifted = targCE >> 32;
    int64_t patCEshifted = patCE >> 32;
    int64_t mask;

    mask = 0xFFFF0000;
    int32_t targLev1 = (int32_t)(targCEshifted & mask);
    int32_t patLev1 = (int32_t)(patCEshifted & mask);
    if ( targLev1 != patLev1 ) {
        if ( targLev1 == 0 ) {
            return U_CE_SKIP_TARG;
        }
        if ( patLev1 == 0 && compareType == USEARCH_ANY_BASE_WEIGHT_IS_WILDCARD ) {
            return U_CE_SKIP_PATN;
        }
        return U_CE_NO_MATCH;
    }

    mask = 0x0000FFFF;
    int32_t targLev2 = (int32_t)(targCEshifted & mask);
    int32_t patLev2 = (int32_t)(patCEshifted & mask);
    if ( targLev2 != patLev2 ) {
        if ( targLev2 == 0 ) {
            return U_CE_SKIP_TARG;
        }
        if ( patLev2 == 0 && compareType == USEARCH_ANY_BASE_WEIGHT_IS_WILDCARD ) {
            return U_CE_SKIP_PATN;
        }
        return (patLev2 == U_CE_LEVEL2_BASE || (compareType == USEARCH_ANY_BASE_WEIGHT_IS_WILDCARD && targLev2 == U_CE_LEVEL2_BASE) )?
            U_CE_MATCH: U_CE_NO_MATCH;
    }
    
    mask = 0xFFFF0000;
    int32_t targLev3 = (int32_t)(targCE & mask);
    int32_t patLev3 = (int32_t)(patCE & mask);
    if ( targLev3 != patLev3 ) {
        return (patLev3 == U_CE_LEVEL3_BASE || (compareType == USEARCH_ANY_BASE_WEIGHT_IS_WILDCARD && targLev3 == U_CE_LEVEL3_BASE) )?
            U_CE_MATCH: U_CE_NO_MATCH;
   }

    return U_CE_MATCH;
}

#if BOYER_MOORE

#endif

U_CAPI UBool U_EXPORT2 usearch_search(UStringSearch  *strsrch,
                                       int32_t        startIdx,
                                       int32_t        *matchStart,
                                       int32_t        *matchLimit,
                                       UErrorCode     *status)
{
    if (U_FAILURE(*status)) {
        return FALSE;
    }

    

#ifdef USEARCH_DEBUG
    if (getenv("USEARCH_DEBUG") != NULL) {
        printf("Pattern CEs\n");
        for (int ii=0; ii<strsrch->pattern.CELength; ii++) {
            printf(" %8x", strsrch->pattern.CE[ii]);
        }
        printf("\n");
    }

#endif
    
    
    
    if(strsrch->pattern.CELength == 0         ||
       startIdx < 0                           ||
       startIdx > strsrch->search->textLength ||
       strsrch->pattern.CE == NULL) {
           *status = U_ILLEGAL_ARGUMENT_ERROR;
           return FALSE;
    }

    if (strsrch->pattern.PCE == NULL) {
        initializePatternPCETable(strsrch, status);
    }

    ucol_setOffset(strsrch->textIter, startIdx, status);
    CEBuffer ceb(strsrch, status);


    int32_t    targetIx = 0;
    const CEI *targetCEI = NULL;
    int32_t    patIx;
    UBool      found;

    int32_t  mStart = -1;
    int32_t  mLimit = -1;
    int32_t  minLimit;
    int32_t  maxLimit;



    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for(targetIx=0; ; targetIx++)
    {
        found = TRUE;
        
        
        int32_t targetIxOffset = 0;
        int64_t patCE = 0;
        
        
        
        const CEI *firstCEI = ceb.get(targetIx);
        if (firstCEI == NULL) {
            *status = U_INTERNAL_PROGRAM_ERROR;
            found = FALSE;
            break;
        }
        
        for (patIx=0; patIx<strsrch->pattern.PCELength; patIx++) {
            patCE = strsrch->pattern.PCE[patIx];
            targetCEI = ceb.get(targetIx+patIx+targetIxOffset);
            
            
            
            UCompareCEsResult ceMatch = compareCE64s(targetCEI->ce, patCE, strsrch->search->elementComparisonType);
            if ( ceMatch == U_CE_NO_MATCH ) {
                found = FALSE;
                break;
            } else if ( ceMatch > U_CE_NO_MATCH ) {
                if ( ceMatch == U_CE_SKIP_TARG ) {
                    
                    patIx--;
                    targetIxOffset++;
                } else { 
                    
                    targetIxOffset--;
                }
            }
        }
        targetIxOffset += strsrch->pattern.PCELength; 

        if (!found && ((targetCEI == NULL) || (targetCEI->ce != UCOL_PROCESSED_NULLORDER))) {
            
            continue;
        }

        if (!found) {
            
            break;
        }


        
        
        
        
        
        const CEI *lastCEI  = ceb.get(targetIx + targetIxOffset - 1);

        mStart   = firstCEI->lowIndex;
        minLimit = lastCEI->lowIndex;

        
        

        
        
        
        
        
        const CEI *nextCEI = 0;
        if (strsrch->search->elementComparisonType == 0) {
            nextCEI  = ceb.get(targetIx + targetIxOffset);
            maxLimit = nextCEI->lowIndex;
            if (nextCEI->lowIndex == nextCEI->highIndex && nextCEI->ce != UCOL_PROCESSED_NULLORDER) {
                found = FALSE;
            }
        } else {
            for ( ; ; ++targetIxOffset ) {
                nextCEI = ceb.get(targetIx + targetIxOffset);
                maxLimit = nextCEI->lowIndex;
                
                if (  nextCEI->ce == UCOL_PROCESSED_NULLORDER ) {
                    break;
                }
                
                
                
                if ( (((nextCEI->ce) >> 32) & 0xFFFF0000UL) == 0 ) {
                    UCompareCEsResult ceMatch = compareCE64s(nextCEI->ce, patCE, strsrch->search->elementComparisonType);
                    if ( ceMatch == U_CE_NO_MATCH || ceMatch == U_CE_SKIP_PATN ) {
                        found = FALSE;
                        break;
                    }
                
                
                } else if ( nextCEI->lowIndex == nextCEI->highIndex ) {
                    found = false;
                    break;
                
                } else {
                    break;
                }
            }
        }


        
        
        
        
        
        
        if (!isBreakBoundary(strsrch, mStart)) {
            found = FALSE;
        }

        
        
        
        
        
        int32_t secondIx = firstCEI->highIndex;
        if (mStart == secondIx) {
            found = FALSE;
        }

        
        
        mLimit = maxLimit;
        if (minLimit < maxLimit) {
            
            
            
            
            
            
            if (minLimit == lastCEI->highIndex && isBreakBoundary(strsrch, minLimit)) {
                mLimit = minLimit;
            } else {
                int32_t nba = nextBoundaryAfter(strsrch, minLimit);
                if (nba >= lastCEI->highIndex) {
                    mLimit = nba;
                }
            }
        }

    #ifdef USEARCH_DEBUG
        if (getenv("USEARCH_DEBUG") != NULL) {
            printf("minLimit, maxLimit, mLimit = %d, %d, %d\n", minLimit, maxLimit, mLimit);
        }
    #endif

        
        
        if (mLimit > maxLimit) {
            found = FALSE;
        }

        if (!isBreakBoundary(strsrch, mLimit)) {
            found = FALSE;
        }

        if (! checkIdentical(strsrch, mStart, mLimit)) {
            found = FALSE;
        }

        if (found) {
            break;
        }
    }

    #ifdef USEARCH_DEBUG
    if (getenv("USEARCH_DEBUG") != NULL) {
        printf("Target CEs [%d .. %d]\n", ceb.firstIx, ceb.limitIx);
        int32_t  lastToPrint = ceb.limitIx+2;
        for (int ii=ceb.firstIx; ii<lastToPrint; ii++) {
            printf("%8x@%d ", ceb.get(ii)->ce, ceb.get(ii)->srcIndex);
        }
        printf("\n%s\n", found? "match found" : "no match");
    }
    #endif

    
    
    if (found==FALSE) {
        mLimit = -1;
        mStart = -1;
    }

    if (matchStart != NULL) {
        *matchStart= mStart;
    }

    if (matchLimit != NULL) {
        *matchLimit = mLimit;
    }

    return found;
}

U_CAPI UBool U_EXPORT2 usearch_searchBackwards(UStringSearch  *strsrch,
                                                int32_t        startIdx,
                                                int32_t        *matchStart,
                                                int32_t        *matchLimit,
                                                UErrorCode     *status)
{
    if (U_FAILURE(*status)) {
        return FALSE;
    }

    

#ifdef USEARCH_DEBUG
    if (getenv("USEARCH_DEBUG") != NULL) {
        printf("Pattern CEs\n");
        for (int ii=0; ii<strsrch->pattern.CELength; ii++) {
            printf(" %8x", strsrch->pattern.CE[ii]);
        }
        printf("\n");
    }

#endif
    
    
    
    if(strsrch->pattern.CELength == 0         ||
       startIdx < 0                           ||
       startIdx > strsrch->search->textLength ||
       strsrch->pattern.CE == NULL) {
           *status = U_ILLEGAL_ARGUMENT_ERROR;
           return FALSE;
    }

    if (strsrch->pattern.PCE == NULL) {
        initializePatternPCETable(strsrch, status);
    }

    CEBuffer ceb(strsrch, status);
    int32_t    targetIx = 0;

    








    if (startIdx < strsrch->search->textLength) {
        UBreakIterator *bi = strsrch->search->internalBreakIter;
        int32_t next = ubrk_following(bi, startIdx);

        ucol_setOffset(strsrch->textIter, next, status);

        for (targetIx = 0; ; targetIx += 1) {
            if (ceb.getPrevious(targetIx)->lowIndex < startIdx) {
                break;
            }
        }
    } else {
        ucol_setOffset(strsrch->textIter, startIdx, status);
    }


    const CEI *targetCEI = NULL;
    int32_t    patIx;
    UBool      found;

    int32_t  limitIx = targetIx;
    int32_t  mStart = -1;
    int32_t  mLimit = -1;
    int32_t  minLimit;
    int32_t  maxLimit;



    
    
    
    
    
    
    for(targetIx = limitIx; ; targetIx += 1)
    {
        found = TRUE;
        
        
        
        const CEI *lastCEI  = ceb.getPrevious(targetIx);
        if (lastCEI == NULL) {
            *status = U_INTERNAL_PROGRAM_ERROR;
            found = FALSE;
             break;
        }
        
        
        int32_t targetIxOffset = 0;
        for (patIx = strsrch->pattern.PCELength - 1; patIx >= 0; patIx -= 1) {
            int64_t patCE = strsrch->pattern.PCE[patIx];

            targetCEI = ceb.getPrevious(targetIx + strsrch->pattern.PCELength - 1 - patIx + targetIxOffset);
            
            
            
            UCompareCEsResult ceMatch = compareCE64s(targetCEI->ce, patCE, strsrch->search->elementComparisonType);
            if ( ceMatch == U_CE_NO_MATCH ) {
                found = FALSE;
                break;
            } else if ( ceMatch > U_CE_NO_MATCH ) {
                if ( ceMatch == U_CE_SKIP_TARG ) {
                    
                    patIx++;
                    targetIxOffset++;
                } else { 
                    
                    targetIxOffset--;
                }
            }
        }

        if (!found && ((targetCEI == NULL) || (targetCEI->ce != UCOL_PROCESSED_NULLORDER))) {
            
            continue;
        }

        if (!found) {
            
            break;
        }


        
        
        
        
        
        const CEI *firstCEI = ceb.getPrevious(targetIx + strsrch->pattern.PCELength - 1 + targetIxOffset);
        mStart   = firstCEI->lowIndex;

        
        
        
        
        
        
        if (!isBreakBoundary(strsrch, mStart)) {
            found = FALSE;
        }

        
        
        if (mStart == firstCEI->highIndex) {
            found = FALSE;
        }


        minLimit = lastCEI->lowIndex;

        if (targetIx > 0) {
            
            

            
            
            
            
            
            const CEI *nextCEI  = ceb.getPrevious(targetIx - 1);

            if (nextCEI->lowIndex == nextCEI->highIndex && nextCEI->ce != UCOL_PROCESSED_NULLORDER) {
                found = FALSE;
            }

            mLimit = maxLimit = nextCEI->lowIndex;

            
            
            if (minLimit < maxLimit) {
                int32_t nba = nextBoundaryAfter(strsrch, minLimit);

                if (nba >= lastCEI->highIndex) {
                    mLimit = nba;
                }
            }

            
            
            if (mLimit > maxLimit) {
                found = FALSE;
            }

            
            if (!isBreakBoundary(strsrch, mLimit)) {
                found = FALSE;
            }

        } else {
            
            
            
            
            int32_t nba = nextBoundaryAfter(strsrch, minLimit);
            mLimit = maxLimit = (nba > 0) && (startIdx > nba) ? nba : startIdx;
        }

    #ifdef USEARCH_DEBUG
        if (getenv("USEARCH_DEBUG") != NULL) {
            printf("minLimit, maxLimit, mLimit = %d, %d, %d\n", minLimit, maxLimit, mLimit);
        }
    #endif


        if (! checkIdentical(strsrch, mStart, mLimit)) {
            found = FALSE;
        }

        if (found) {
            break;
        }
    }

    #ifdef USEARCH_DEBUG
    if (getenv("USEARCH_DEBUG") != NULL) {
        printf("Target CEs [%d .. %d]\n", ceb.firstIx, ceb.limitIx);
        int32_t  lastToPrint = ceb.limitIx+2;
        for (int ii=ceb.firstIx; ii<lastToPrint; ii++) {
            printf("%8x@%d ", ceb.get(ii)->ce, ceb.get(ii)->srcIndex);
        }
        printf("\n%s\n", found? "match found" : "no match");
    }
    #endif

    
    
    if (found==FALSE) {
        mLimit = -1;
        mStart = -1;
    }

    if (matchStart != NULL) {
        *matchStart= mStart;
    }

    if (matchLimit != NULL) {
        *matchLimit = mLimit;
    }

    return found;
}



UBool usearch_handleNextExact(UStringSearch *strsrch, UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        setMatchNotFound(strsrch);
        return FALSE;
    }

#if BOYER_MOORE
    UCollationElements *coleiter        = strsrch->textIter;
    int32_t             textlength      = strsrch->search->textLength;
    int32_t            *patternce       = strsrch->pattern.CE;
    int32_t             patterncelength = strsrch->pattern.CELength;
    int32_t             textoffset      = ucol_getOffset(coleiter);

    
    
    
    textoffset = shiftForward(strsrch, textoffset, UCOL_NULLORDER,
                              patterncelength);
    while (textoffset <= textlength)
    {
        uint32_t    patternceindex = patterncelength - 1;
        int32_t     targetce;
        UBool       found          = FALSE;
        int32_t    lastce          = UCOL_NULLORDER;

        setColEIterOffset(coleiter, textoffset);

        for (;;) {
            
            
            
            targetce = ucol_previous(coleiter, status);
            if (U_FAILURE(*status) || targetce == UCOL_NULLORDER) {
                found = FALSE;
                break;
            }
            targetce = getCE(strsrch, targetce);
            if (targetce == UCOL_IGNORABLE && inNormBuf(coleiter)) {
                
                
                continue;
            }
            if (lastce == UCOL_NULLORDER || lastce == UCOL_IGNORABLE) {
                lastce = targetce;
            }
            
            if (targetce == patternce[patternceindex]) {
                
                found = TRUE;
                break;
            }
            if (!hasExpansion(coleiter)) {
                found = FALSE;
                break;
            }
        }

        

        while (found && patternceindex > 0) {
            lastce = targetce;
            targetce    = ucol_previous(coleiter, status);
            if (U_FAILURE(*status) || targetce == UCOL_NULLORDER) {
                found = FALSE;
                break;
            }
            targetce    = getCE(strsrch, targetce);
            if (targetce == UCOL_IGNORABLE) {
                continue;
            }

            patternceindex --;
            
            found = found && targetce == patternce[patternceindex];
        }

        targetce = lastce;

        if (!found) {
            if (U_FAILURE(*status)) {
                break;
            }
            textoffset = shiftForward(strsrch, textoffset, lastce,
                                      patternceindex);
            
            patternceindex = patterncelength;
            continue;
        }

        if (checkNextExactMatch(strsrch, &textoffset, status)) {
            
            setColEIterOffset(coleiter, strsrch->search->matchedIndex);
            return TRUE;
        }
    }
    setMatchNotFound(strsrch);
    return FALSE;
#else
    int32_t textOffset = ucol_getOffset(strsrch->textIter);
    int32_t start = -1;
    int32_t end = -1;

    if (usearch_search(strsrch, textOffset, &start, &end, status)) {
        strsrch->search->matchedIndex  = start;
        strsrch->search->matchedLength = end - start;
        return TRUE;
    } else {
        setMatchNotFound(strsrch);
        return FALSE;
    }
#endif
}

UBool usearch_handleNextCanonical(UStringSearch *strsrch, UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        setMatchNotFound(strsrch);
        return FALSE;
    }

#if BOYER_MOORE
    UCollationElements *coleiter        = strsrch->textIter;
    int32_t             textlength      = strsrch->search->textLength;
    int32_t            *patternce       = strsrch->pattern.CE;
    int32_t             patterncelength = strsrch->pattern.CELength;
    int32_t             textoffset      = ucol_getOffset(coleiter);
    UBool               hasPatternAccents =
       strsrch->pattern.hasSuffixAccents || strsrch->pattern.hasPrefixAccents;

    textoffset = shiftForward(strsrch, textoffset, UCOL_NULLORDER,
                              patterncelength);
    strsrch->canonicalPrefixAccents[0] = 0;
    strsrch->canonicalSuffixAccents[0] = 0;

    while (textoffset <= textlength)
    {
        int32_t     patternceindex = patterncelength - 1;
        int32_t     targetce;
        UBool       found          = FALSE;
        int32_t     lastce         = UCOL_NULLORDER;

        setColEIterOffset(coleiter, textoffset);

        for (;;) {
            
            
            
            targetce = ucol_previous(coleiter, status);
            if (U_FAILURE(*status) || targetce == UCOL_NULLORDER) {
                found = FALSE;
                break;
            }
            targetce = getCE(strsrch, targetce);
            if (lastce == UCOL_NULLORDER || lastce == UCOL_IGNORABLE) {
                lastce = targetce;
            }
            
            if (targetce == patternce[patternceindex]) {
                
                found = TRUE;
                break;
            }
            if (!hasExpansion(coleiter)) {
                found = FALSE;
                break;
            }
        }

        while (found && patternceindex > 0) {
            targetce    = ucol_previous(coleiter, status);
            if (U_FAILURE(*status) || targetce == UCOL_NULLORDER) {
                found = FALSE;
                break;
            }
            targetce    = getCE(strsrch, targetce);
            if (targetce == UCOL_IGNORABLE) {
                continue;
            }

            patternceindex --;
            
            found = found && targetce == patternce[patternceindex];
        }

        
        if (hasPatternAccents && !found) {
            strsrch->canonicalPrefixAccents[0] = 0;
            strsrch->canonicalSuffixAccents[0] = 0;
            if (U_FAILURE(*status)) {
                break;
            }
            found = doNextCanonicalMatch(strsrch, textoffset, status);
        }

        if (!found) {
            if (U_FAILURE(*status)) {
                break;
            }
            textoffset = shiftForward(strsrch, textoffset, lastce,
                                      patternceindex);
            
            patternceindex = patterncelength;
            continue;
        }

        if (checkNextCanonicalMatch(strsrch, &textoffset, status)) {
            setColEIterOffset(coleiter, strsrch->search->matchedIndex);
            return TRUE;
        }
    }
    setMatchNotFound(strsrch);
    return FALSE;
#else
    int32_t textOffset = ucol_getOffset(strsrch->textIter);
    int32_t start = -1;
    int32_t end = -1;

    if (usearch_search(strsrch, textOffset, &start, &end, status)) {
        strsrch->search->matchedIndex  = start;
        strsrch->search->matchedLength = end - start;
        return TRUE;
    } else {
        setMatchNotFound(strsrch);
        return FALSE;
    }
#endif
}

UBool usearch_handlePreviousExact(UStringSearch *strsrch, UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        setMatchNotFound(strsrch);
        return FALSE;
    }

#if BOYER_MOORE
    UCollationElements *coleiter        = strsrch->textIter;
    int32_t            *patternce       = strsrch->pattern.CE;
    int32_t             patterncelength = strsrch->pattern.CELength;
    int32_t             textoffset      = ucol_getOffset(coleiter);

    
    
    
    if (strsrch->search->matchedIndex != USEARCH_DONE) {
        textoffset = strsrch->search->matchedIndex;
    }

    textoffset = reverseShift(strsrch, textoffset, UCOL_NULLORDER,
                              patterncelength);

    while (textoffset >= 0)
    {
        int32_t     patternceindex = 1;
        int32_t     targetce;
        UBool       found          = FALSE;
        int32_t     firstce        = UCOL_NULLORDER;

        
        setColEIterOffset(coleiter, textoffset);

        for (;;) {
            
            
            
            
            targetce = ucol_next(coleiter, status);
            if (U_FAILURE(*status) || targetce == UCOL_NULLORDER) {
                found = FALSE;
                break;
            }
            targetce = getCE(strsrch, targetce);
            if (firstce == UCOL_NULLORDER || firstce == UCOL_IGNORABLE) {
                firstce = targetce;
            }
            if (targetce == UCOL_IGNORABLE && strsrch->strength != UCOL_PRIMARY) {
                continue;
            }
            
            if (targetce == patternce[0]) {
                found = TRUE;
                break;
            }
            if (!hasExpansion(coleiter)) {
                
                found = FALSE;
                break;
            }
        }

        

        while (found && (patternceindex < patterncelength)) {
            firstce = targetce;
            targetce    = ucol_next(coleiter, status);
            if (U_FAILURE(*status) || targetce == UCOL_NULLORDER) {
                found = FALSE;
                break;
            }
            targetce    = getCE(strsrch, targetce);
            if (targetce == UCOL_IGNORABLE) {
                continue;
            }

            
            found = found && targetce == patternce[patternceindex];
            patternceindex ++;
        }

        targetce = firstce;

        if (!found) {
            if (U_FAILURE(*status)) {
                break;
            }

            textoffset = reverseShift(strsrch, textoffset, targetce,
                                      patternceindex);
            patternceindex = 0;
            continue;
        }

        if (checkPreviousExactMatch(strsrch, &textoffset, status)) {
            setColEIterOffset(coleiter, textoffset);
            return TRUE;
        }
    }
    setMatchNotFound(strsrch);
    return FALSE;
#else
    int32_t textOffset;

    if (strsrch->search->isOverlap) {
        if (strsrch->search->matchedIndex != USEARCH_DONE) {
            textOffset = strsrch->search->matchedIndex + strsrch->search->matchedLength - 1;
        } else {
            
            initializePatternPCETable(strsrch, status);
            for (int32_t nPCEs = 0; nPCEs < strsrch->pattern.PCELength - 1; nPCEs++) {
                int64_t pce = ucol_nextProcessed(strsrch->textIter, NULL, NULL, status);
                if (pce == UCOL_PROCESSED_NULLORDER) {
                    
                    break;
                }
            }
            if (U_FAILURE(*status)) {
                setMatchNotFound(strsrch);
                return FALSE;
            }
            textOffset = ucol_getOffset(strsrch->textIter);
        }
    } else {
        textOffset = ucol_getOffset(strsrch->textIter);
    }

    int32_t start = -1;
    int32_t end = -1;

    if (usearch_searchBackwards(strsrch, textOffset, &start, &end, status)) {
        strsrch->search->matchedIndex = start;
        strsrch->search->matchedLength = end - start;
        return TRUE;
    } else {
        setMatchNotFound(strsrch);
        return FALSE;
    }
#endif
}

UBool usearch_handlePreviousCanonical(UStringSearch *strsrch,
                                      UErrorCode    *status)
{
    if (U_FAILURE(*status)) {
        setMatchNotFound(strsrch);
        return FALSE;
    }

#if BOYER_MOORE
    UCollationElements *coleiter        = strsrch->textIter;
    int32_t            *patternce       = strsrch->pattern.CE;
    int32_t             patterncelength = strsrch->pattern.CELength;
    int32_t             textoffset      = ucol_getOffset(coleiter);
    UBool               hasPatternAccents =
       strsrch->pattern.hasSuffixAccents || strsrch->pattern.hasPrefixAccents;

    
    
    
    if (strsrch->search->matchedIndex != USEARCH_DONE) {
        textoffset = strsrch->search->matchedIndex;
    }

    textoffset = reverseShift(strsrch, textoffset, UCOL_NULLORDER,
                              patterncelength);
    strsrch->canonicalPrefixAccents[0] = 0;
    strsrch->canonicalSuffixAccents[0] = 0;

    while (textoffset >= 0)
    {
        int32_t     patternceindex = 1;
        int32_t     targetce;
        UBool       found          = FALSE;
        int32_t     firstce        = UCOL_NULLORDER;

        setColEIterOffset(coleiter, textoffset);
        for (;;) {
            
            
            
            
            targetce = ucol_next(coleiter, status);
            if (U_FAILURE(*status) || targetce == UCOL_NULLORDER) {
                found = FALSE;
                break;
            }
            targetce = getCE(strsrch, targetce);
            if (firstce == UCOL_NULLORDER || firstce == UCOL_IGNORABLE) {
                firstce = targetce;
            }

            
            if (targetce == patternce[0]) {
                
                found = TRUE;
                break;
            }
            if (!hasExpansion(coleiter)) {
                
                found = FALSE;
                break;
            }
        }

        targetce = firstce;

        while (found && patternceindex < patterncelength) {
            targetce    = ucol_next(coleiter, status);
            if (U_FAILURE(*status) || targetce == UCOL_NULLORDER) {
                found = FALSE;
                break;
            }
            targetce = getCE(strsrch, targetce);
            if (targetce == UCOL_IGNORABLE) {
                continue;
            }

            
            found = found && targetce == patternce[patternceindex];
            patternceindex ++;
        }

        
        if (hasPatternAccents && !found) {
            strsrch->canonicalPrefixAccents[0] = 0;
            strsrch->canonicalSuffixAccents[0] = 0;
            if (U_FAILURE(*status)) {
                break;
            }
            found = doPreviousCanonicalMatch(strsrch, textoffset, status);
        }

        if (!found) {
            if (U_FAILURE(*status)) {
                break;
            }
            textoffset = reverseShift(strsrch, textoffset, targetce,
                                      patternceindex);
            patternceindex = 0;
            continue;
        }

        if (checkPreviousCanonicalMatch(strsrch, &textoffset, status)) {
            setColEIterOffset(coleiter, textoffset);
            return TRUE;
        }
    }
    setMatchNotFound(strsrch);
    return FALSE;
#else
    int32_t textOffset;

    if (strsrch->search->isOverlap) {
        if (strsrch->search->matchedIndex != USEARCH_DONE) {
            textOffset = strsrch->search->matchedIndex + strsrch->search->matchedLength - 1;
        } else {
            
            initializePatternPCETable(strsrch, status);
            for (int32_t nPCEs = 0; nPCEs < strsrch->pattern.PCELength - 1; nPCEs++) {
                int64_t pce = ucol_nextProcessed(strsrch->textIter, NULL, NULL, status);
                if (pce == UCOL_PROCESSED_NULLORDER) {
                    
                    break;
                }
            }
            if (U_FAILURE(*status)) {
                setMatchNotFound(strsrch);
                return FALSE;
            }
            textOffset = ucol_getOffset(strsrch->textIter);
        }
    } else {
        textOffset = ucol_getOffset(strsrch->textIter);
    }

    int32_t start = -1;
    int32_t end = -1;

    if (usearch_searchBackwards(strsrch, textOffset, &start, &end, status)) {
        strsrch->search->matchedIndex = start;
        strsrch->search->matchedLength = end - start;
        return TRUE;
    } else {
        setMatchNotFound(strsrch);
        return FALSE;
    }
#endif
}

#endif 
