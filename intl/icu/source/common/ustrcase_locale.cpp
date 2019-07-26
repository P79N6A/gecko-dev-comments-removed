
















#include "unicode/utypes.h"
#include "unicode/ucasemap.h"
#include "unicode/uloc.h"
#include "unicode/ustring.h"
#include "ucase.h"
#include "ustr_imp.h"

U_CFUNC void
ustrcase_setTempCaseMapLocale(UCaseMap *csm, const char *locale) {
    











    int i;
    char c;

    
    if(locale==NULL) {
        
        
        
        
        
        
        
        
        
        
        locale=uloc_getDefault();
    }
    for(i=0; i<4 && (c=locale[i])!=0 && c!='-' && c!='_'; ++i) {
        csm->locale[i]=c;
    }
    if(i<=3) {
        csm->locale[i]=0;  
    } else {
        csm->locale[0]=0;  
    }
}





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
u_strToLower(UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             const char *locale,
             UErrorCode *pErrorCode) {
    UCaseMap csm=UCASEMAP_INITIALIZER;
    setTempCaseMap(&csm, locale);
    return ustrcase_map(
        &csm,
        dest, destCapacity,
        src, srcLength,
        ustrcase_internalToLower, pErrorCode);
}

U_CAPI int32_t U_EXPORT2
u_strToUpper(UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             const char *locale,
             UErrorCode *pErrorCode) {
    UCaseMap csm=UCASEMAP_INITIALIZER;
    setTempCaseMap(&csm, locale);
    return ustrcase_map(
        &csm,
        dest, destCapacity,
        src, srcLength,
        ustrcase_internalToUpper, pErrorCode);
}
