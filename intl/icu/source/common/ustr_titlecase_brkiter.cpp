
















#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/brkiter.h"
#include "unicode/ubrk.h"
#include "unicode/ucasemap.h"
#include "cmemory.h"
#include "ucase.h"
#include "ustr_imp.h"








static inline void
setTempCaseMap(UCaseMap *csm, const char *locale) {
    if(csm->csp==NULL) {
        csm->csp=ucase_getSingleton();
    }
    if(locale!=NULL && locale[0]==0) {
        csm->locale[0]=0;
    } else {
        ustrcase_setTempCaseMapLocale(csm, locale);
    }
}



U_CAPI int32_t U_EXPORT2
u_strToTitle(UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             UBreakIterator *titleIter,
             const char *locale,
             UErrorCode *pErrorCode) {
    UCaseMap csm=UCASEMAP_INITIALIZER;
    setTempCaseMap(&csm, locale);
    if(titleIter!=NULL) {
        ubrk_setText(csm.iter=titleIter, src, srcLength, pErrorCode);
    } else {
        csm.iter=ubrk_open(UBRK_WORD, csm.locale, src, srcLength, pErrorCode);
    }
    int32_t length=ustrcase_map(
        &csm,
        dest, destCapacity,
        src, srcLength,
        ustrcase_internalToTitle, pErrorCode);
    if(titleIter==NULL && csm.iter!=NULL) {
        ubrk_close(csm.iter);
    }
    return length;
}

U_CAPI int32_t U_EXPORT2
ucasemap_toTitle(UCaseMap *csm,
                 UChar *dest, int32_t destCapacity,
                 const UChar *src, int32_t srcLength,
                 UErrorCode *pErrorCode) {
    if(csm->iter!=NULL) {
        ubrk_setText(csm->iter, src, srcLength, pErrorCode);
    } else {
        csm->iter=ubrk_open(UBRK_WORD, csm->locale, src, srcLength, pErrorCode);
    }
    return ustrcase_map(
        csm,
        dest, destCapacity,
        src, srcLength,
        ustrcase_internalToTitle, pErrorCode);
}

#endif  
