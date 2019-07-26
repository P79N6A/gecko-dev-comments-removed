

















#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/ures.h"
#include "uinvchar.h"
#include "ustr_cnv.h"

U_CAPI UResourceBundle * U_EXPORT2
ures_openU(const UChar *myPath, 
           const char *localeID, 
           UErrorCode *status)
{
    char pathBuffer[1024];
    int32_t length;
    char *path = pathBuffer;

    if(status==NULL || U_FAILURE(*status)) {
        return NULL;
    }
    if(myPath==NULL) {
        path = NULL;
    }
    else {
        length=u_strlen(myPath);
        if(length>=sizeof(pathBuffer)) {
            *status=U_ILLEGAL_ARGUMENT_ERROR;
            return NULL;
        } else if(uprv_isInvariantUString(myPath, length)) {
            



            u_UCharsToChars(myPath, path, length+1); 
        } else {
#if !UCONFIG_NO_CONVERSION
            
            UConverter *cnv=u_getDefaultConverter(status);
            length=ucnv_fromUChars(cnv, path, (int32_t)sizeof(pathBuffer), myPath, length, status);
            u_releaseDefaultConverter(cnv);
            if(U_FAILURE(*status)) {
                return NULL;
            }
            if(length>=sizeof(pathBuffer)) {
                
                *status=U_ILLEGAL_ARGUMENT_ERROR;
                return NULL;
            }
#else
            
            *status=U_UNSUPPORTED_ERROR;
            return NULL;
#endif
        }
    }

    return ures_open(path, localeID, status);
}
