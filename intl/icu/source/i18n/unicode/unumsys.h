






#ifndef UNUMSYS_H
#define UNUMSYS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uenum.h"
#include "unicode/localpointer.h"
























struct UNumberingSystem;
typedef struct UNumberingSystem UNumberingSystem;  












U_STABLE UNumberingSystem * U_EXPORT2
unumsys_open(const char *locale, UErrorCode *status);



















U_STABLE UNumberingSystem * U_EXPORT2
unumsys_openByName(const char *name, UErrorCode *status);






U_STABLE void U_EXPORT2
unumsys_close(UNumberingSystem *unumsys);

#if U_SHOW_CPLUSPLUS_API
U_NAMESPACE_BEGIN









U_DEFINE_LOCAL_OPEN_POINTER(LocalUNumberingSystemPointer, UNumberingSystem, unumsys_close);

U_NAMESPACE_END
#endif









U_STABLE UEnumeration * U_EXPORT2
unumsys_openAvailableNames(UErrorCode *status);










U_STABLE const char * U_EXPORT2
unumsys_getName(const UNumberingSystem *unumsys);









U_STABLE UBool U_EXPORT2
unumsys_isAlgorithmic(const UNumberingSystem *unumsys);









U_STABLE int32_t U_EXPORT2
unumsys_getRadix(const UNumberingSystem *unumsys);

















U_STABLE int32_t U_EXPORT2
unumsys_getDescription(const UNumberingSystem *unumsys, UChar *result,
                       int32_t resultLength, UErrorCode *status);

#endif 

#endif
