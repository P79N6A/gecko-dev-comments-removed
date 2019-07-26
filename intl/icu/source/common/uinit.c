















#include "unicode/utypes.h"
#include "unicode/icuplug.h"
#include "unicode/uclean.h"
#include "cmemory.h"
#include "icuplugimp.h"
#include "ucln.h"
#include "ucnv_io.h"
#include "utracimp.h"

static void U_CALLCONV
initData(UErrorCode *status)
{
    











#if !UCONFIG_NO_CONVERSION
    ucnv_io_countKnownConverters(status);
#endif
}




U_CAPI void U_EXPORT2
u_init(UErrorCode *status) {
    UTRACE_ENTRY_OC(UTRACE_U_INIT);

    
    uplug_init(status);
    ucln_mutexedInit(initData, status);

    UTRACE_EXIT_STATUS(*status);
}
