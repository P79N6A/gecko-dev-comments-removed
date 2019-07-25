


































#include "prtypes.h"
#include "nssutil.h"




const char *
SECU_Strerror(PRErrorCode errNum) {
    return NSS_Strerror(errNum, formatSimple);
}
