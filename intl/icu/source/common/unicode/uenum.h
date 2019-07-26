















#ifndef __UENUM_H
#define __UENUM_H

#include "unicode/utypes.h"
#include "unicode/localpointer.h"

#if U_SHOW_CPLUSPLUS_API
#include "unicode/strenum.h"
#endif





 





struct UEnumeration;

typedef struct UEnumeration UEnumeration;








U_STABLE void U_EXPORT2
uenum_close(UEnumeration* en);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUEnumerationPointer, UEnumeration, uenum_close);

U_NAMESPACE_END

#endif















U_STABLE int32_t U_EXPORT2
uenum_count(UEnumeration* en, UErrorCode* status);






















U_STABLE const UChar* U_EXPORT2
uenum_unext(UEnumeration* en,
            int32_t* resultLength,
            UErrorCode* status);





























U_STABLE const char* U_EXPORT2
uenum_next(UEnumeration* en,
           int32_t* resultLength,
           UErrorCode* status);










U_STABLE void U_EXPORT2
uenum_reset(UEnumeration* en, UErrorCode* status);

#if U_SHOW_CPLUSPLUS_API










U_STABLE UEnumeration* U_EXPORT2
uenum_openFromStringEnumeration(icu::StringEnumeration* adopted, UErrorCode* ec);

#endif

#ifndef U_HIDE_DRAFT_API











U_DRAFT UEnumeration* U_EXPORT2
uenum_openUCharStringsEnumeration(const UChar* const strings[], int32_t count,
                                 UErrorCode* ec);
#endif














U_DRAFT UEnumeration* U_EXPORT2
uenum_openCharStringsEnumeration(const char* const strings[], int32_t count,
                                 UErrorCode* ec);


#endif
