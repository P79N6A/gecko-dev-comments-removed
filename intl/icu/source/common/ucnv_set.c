



















#include "unicode/utypes.h"
#include "unicode/uset.h"
#include "unicode/ucnv.h"
#include "ucnv_bld.h"
#include "uset_imp.h"

#if !UCONFIG_NO_CONVERSION

U_CAPI void U_EXPORT2
ucnv_getUnicodeSet(const UConverter *cnv,
                   USet *setFillIn,
                   UConverterUnicodeSet whichSet,
                   UErrorCode *pErrorCode) {
    
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }
    if(cnv==NULL || setFillIn==NULL || whichSet<UCNV_ROUNDTRIP_SET || UCNV_SET_COUNT<=whichSet) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    
    if(cnv->sharedData->impl->getUnicodeSet==NULL) {
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return;
    }

    {
        USetAdder sa={
            NULL,
            uset_add,
            uset_addRange,
            uset_addString,
            uset_remove,
            uset_removeRange
        };
        sa.set=setFillIn;

        
        uset_clear(setFillIn);

        
        cnv->sharedData->impl->getUnicodeSet(cnv, &sa, whichSet, pErrorCode);
    }
}

#endif
