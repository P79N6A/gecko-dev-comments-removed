













#include "unicode/utypes.h"
#include "unicode/icuplug.h"
#include "unicode/uclean.h"
#include "cmemory.h"
#include "icuplugimp.h"
#include "ucln_cmn.h"
#include "ucnv_io.h"
#include "umutex.h"
#include "utracimp.h"

U_NAMESPACE_BEGIN

static UInitOnce gICUInitOnce = U_INITONCE_INITIALIZER;

static UBool U_CALLCONV uinit_cleanup() {
    gICUInitOnce.reset();
    return TRUE;
}

static void U_CALLCONV
initData(UErrorCode &status)
{
    
    uplug_init(&status);

#if !UCONFIG_NO_CONVERSION
    











    ucnv_io_countKnownConverters(&status);
#endif
    ucln_common_registerCleanup(UCLN_COMMON_UINIT, uinit_cleanup);
}

U_NAMESPACE_END

U_NAMESPACE_USE




U_CAPI void U_EXPORT2
u_init(UErrorCode *status) {
    UTRACE_ENTRY_OC(UTRACE_U_INIT);
    umtx_initOnce(gICUInitOnce, &initData, *status);
    UTRACE_EXIT_STATUS(*status);
}
