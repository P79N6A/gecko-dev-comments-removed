















#ifndef __UCLN_H__
#define __UCLN_H__

#include "unicode/utypes.h"
































typedef enum ECleanupLibraryType {
    UCLN_START = -1,
    UCLN_UPLUG,     
    UCLN_CUSTOM,    
    UCLN_CTESTFW,
    UCLN_TOOLUTIL,
    UCLN_LAYOUTEX,
    UCLN_LAYOUT,
    UCLN_IO,
    UCLN_I18N,
    UCLN_COMMON 
} ECleanupLibraryType;




U_CDECL_BEGIN
typedef UBool U_CALLCONV cleanupFunc(void);
typedef void U_CALLCONV initFunc(UErrorCode *);
U_CDECL_END






U_CAPI void U_EXPORT2 ucln_registerCleanup(ECleanupLibraryType type,
                                           cleanupFunc *func);






U_CAPI void U_EXPORT2 ucln_cleanupOne(ECleanupLibraryType type);


U_CFUNC UBool ucln_mutexedInit(initFunc *func, UErrorCode *status);

#endif
