



















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ustring.h"
#include "unicode/ures.h"
#include "unicode/ucnv.h"
#include "cstring.h"
#include "ustr_imp.h"
#include "ucnv_imp.h"
#include "putilimp.h"

U_CAPI int32_t U_EXPORT2
ucnv_getDisplayName(const UConverter *cnv,
                    const char *displayLocale,
                    UChar *displayName, int32_t displayNameCapacity,
                    UErrorCode *pErrorCode) {
    UResourceBundle *rb;
    const UChar *name;
    int32_t length;
    UErrorCode localStatus = U_ZERO_ERROR;

    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    if(cnv==NULL || displayNameCapacity<0 || (displayNameCapacity>0 && displayName==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    rb=ures_open(NULL, displayLocale, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }

    
    name=ures_getStringByKey(rb, cnv->sharedData->staticData->name, &length, &localStatus);
    ures_close(rb);

    if(U_SUCCESS(localStatus)) {
        
        if (*pErrorCode == U_ZERO_ERROR) {
            *pErrorCode = localStatus;
        }
        u_memcpy(displayName, name, uprv_min(length, displayNameCapacity)*U_SIZEOF_UCHAR);
    } else {
        
        length=(int32_t)uprv_strlen(cnv->sharedData->staticData->name);
        u_charsToUChars(cnv->sharedData->staticData->name, displayName, uprv_min(length, displayNameCapacity));
    }
    return u_terminateUChars(displayName, displayNameCapacity, length, pErrorCode);
}

#endif









