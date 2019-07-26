







#ifndef USEARCH_H
#define USEARCH_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/localpointer.h"
#include "unicode/ucol.h"
#include "unicode/ucoleitr.h"
#include "unicode/ubrk.h"
























































































































#define USEARCH_DONE -1





struct UStringSearch;




typedef struct UStringSearch UStringSearch;




typedef enum {
    
    USEARCH_OVERLAP,
    



    USEARCH_CANONICAL_MATCH,
    




    USEARCH_ELEMENT_COMPARISON,

    USEARCH_ATTRIBUTE_COUNT
} USearchAttribute;




typedef enum {
    
    USEARCH_DEFAULT = -1,
    
    USEARCH_OFF, 
    
    USEARCH_ON,
    





    USEARCH_STANDARD_ELEMENT_COMPARISON,
    












    USEARCH_PATTERN_BASE_WEIGHT_IS_WILDCARD,
    












    USEARCH_ANY_BASE_WEIGHT_IS_WILDCARD,

    USEARCH_ATTRIBUTE_VALUE_COUNT
} USearchAttributeValue;

























U_STABLE UStringSearch * U_EXPORT2 usearch_open(const UChar          *pattern, 
                                              int32_t         patternlength, 
                                        const UChar          *text, 
                                              int32_t         textlength,
                                        const char           *locale,
                                              UBreakIterator *breakiter,
                                              UErrorCode     *status);

























U_STABLE UStringSearch * U_EXPORT2 usearch_openFromCollator(
                                         const UChar *pattern, 
                                               int32_t         patternlength,
                                         const UChar          *text, 
                                               int32_t         textlength,
                                         const UCollator      *collator,
                                               UBreakIterator *breakiter,
                                               UErrorCode     *status);







U_STABLE void U_EXPORT2 usearch_close(UStringSearch *searchiter);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUStringSearchPointer, UStringSearch, usearch_close);

U_NAMESPACE_END

#endif


















U_STABLE void U_EXPORT2 usearch_setOffset(UStringSearch *strsrch, 
                                        int32_t    position,
                                        UErrorCode    *status);









U_STABLE int32_t U_EXPORT2 usearch_getOffset(const UStringSearch *strsrch);
    











U_STABLE void U_EXPORT2 usearch_setAttribute(UStringSearch         *strsrch, 
                                           USearchAttribute       attribute,
                                           USearchAttributeValue  value,
                                           UErrorCode            *status);









U_STABLE USearchAttributeValue U_EXPORT2 usearch_getAttribute(
                                         const UStringSearch    *strsrch,
                                               USearchAttribute  attribute);




















U_STABLE int32_t U_EXPORT2 usearch_getMatchedStart(
                                               const UStringSearch *strsrch);
    

















U_STABLE int32_t U_EXPORT2 usearch_getMatchedLength(
                                               const UStringSearch *strsrch);


























U_STABLE int32_t U_EXPORT2 usearch_getMatchedText(const UStringSearch *strsrch, 
                                            UChar         *result, 
                                            int32_t        resultCapacity, 
                                            UErrorCode    *status);

#if !UCONFIG_NO_BREAK_ITERATION
















U_STABLE void U_EXPORT2 usearch_setBreakIterator(UStringSearch  *strsrch, 
                                               UBreakIterator *breakiter,
                                               UErrorCode     *status);












U_STABLE const UBreakIterator * U_EXPORT2 usearch_getBreakIterator(
                                              const UStringSearch *strsrch);
    
#endif
    













U_STABLE void U_EXPORT2 usearch_setText(      UStringSearch *strsrch, 
                                      const UChar         *text,
                                            int32_t        textlength,
                                            UErrorCode    *status);









U_STABLE const UChar * U_EXPORT2 usearch_getText(const UStringSearch *strsrch, 
                                               int32_t       *length);











U_STABLE UCollator * U_EXPORT2 usearch_getCollator(
                                               const UStringSearch *strsrch);











U_STABLE void U_EXPORT2 usearch_setCollator(      UStringSearch *strsrch, 
                                          const UCollator     *collator,
                                                UErrorCode    *status);













U_STABLE void U_EXPORT2 usearch_setPattern(      UStringSearch *strsrch, 
                                         const UChar         *pattern,
                                               int32_t        patternlength,
                                               UErrorCode    *status);









U_STABLE const UChar * U_EXPORT2 usearch_getPattern(
                                               const UStringSearch *strsrch, 
                                                     int32_t       *length);


















U_STABLE int32_t U_EXPORT2 usearch_first(UStringSearch *strsrch, 
                                           UErrorCode    *status);






















U_STABLE int32_t U_EXPORT2 usearch_following(UStringSearch *strsrch, 
                                               int32_t    position, 
                                               UErrorCode    *status);
    















U_STABLE int32_t U_EXPORT2 usearch_last(UStringSearch *strsrch, 
                                          UErrorCode    *status);


























U_STABLE int32_t U_EXPORT2 usearch_preceding(UStringSearch *strsrch, 
                                               int32_t    position, 
                                               UErrorCode    *status);
    

















U_STABLE int32_t U_EXPORT2 usearch_next(UStringSearch *strsrch, 
                                          UErrorCode    *status);


















U_STABLE int32_t U_EXPORT2 usearch_previous(UStringSearch *strsrch, 
                                              UErrorCode    *status);
    










U_STABLE void U_EXPORT2 usearch_reset(UStringSearch *strsrch);

#ifndef U_HIDE_INTERNAL_API






















































U_INTERNAL UBool U_EXPORT2 usearch_search(UStringSearch *strsrch,
                                          int32_t        startIdx,
                                          int32_t        *matchStart,
                                          int32_t        *matchLimit,
                                          UErrorCode     *status);























































U_INTERNAL UBool U_EXPORT2 usearch_searchBackwards(UStringSearch *strsrch,
                                                   int32_t        startIdx,
                                                   int32_t        *matchStart,
                                                   int32_t        *matchLimit,
                                                   UErrorCode     *status);
#endif  

#endif 

#endif
