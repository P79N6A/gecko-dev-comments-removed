














#ifndef UCOLEITR_H
#define UCOLEITR_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION






#define UCOL_NULLORDER        ((int32_t)0xFFFFFFFF)

#ifndef U_HIDE_INTERNAL_API






#define UCOL_PROCESSED_NULLORDER        ((int64_t)U_INT64_MAX)
#endif  

#include "unicode/ucol.h"






typedef struct UCollationElements UCollationElements;








































































U_STABLE UCollationElements* U_EXPORT2 
ucol_openElements(const UCollator  *coll,
                  const UChar      *text,
                        int32_t    textLength,
                        UErrorCode *status);









U_STABLE int32_t U_EXPORT2 
ucol_keyHashCode(const uint8_t* key, int32_t length);







U_STABLE void U_EXPORT2 
ucol_closeElements(UCollationElements *elems);










U_STABLE void U_EXPORT2 
ucol_reset(UCollationElements *elems);

#ifndef U_HIDE_INTERNAL_API











U_INTERNAL void U_EXPORT2
ucol_forceHanImplicit(UCollationElements *elems, UErrorCode *status);
#endif  










U_STABLE int32_t U_EXPORT2 
ucol_next(UCollationElements *elems, UErrorCode *status);

















U_STABLE int32_t U_EXPORT2 
ucol_previous(UCollationElements *elems, UErrorCode *status);

#ifndef U_HIDE_INTERNAL_API













U_INTERNAL int64_t U_EXPORT2
ucol_nextProcessed(UCollationElements *elems, int32_t *ixLow, int32_t *ixHigh, UErrorCode *status);





















U_INTERNAL int64_t U_EXPORT2
ucol_previousProcessed(UCollationElements *elems, int32_t *ixLow, int32_t *ixHigh, UErrorCode *status);
#endif  












U_STABLE int32_t U_EXPORT2 
ucol_getMaxExpansion(const UCollationElements *elems, int32_t order);













U_STABLE void U_EXPORT2 
ucol_setText(      UCollationElements *elems, 
             const UChar              *text,
                   int32_t            textLength,
                   UErrorCode         *status);










U_STABLE int32_t U_EXPORT2 
ucol_getOffset(const UCollationElements *elems);













U_STABLE void U_EXPORT2 
ucol_setOffset(UCollationElements *elems,
               int32_t        offset,
               UErrorCode         *status);







U_STABLE int32_t U_EXPORT2
ucol_primaryOrder (int32_t order); 







U_STABLE int32_t U_EXPORT2
ucol_secondaryOrder (int32_t order); 







U_STABLE int32_t U_EXPORT2
ucol_tertiaryOrder (int32_t order); 

#endif 

#endif
