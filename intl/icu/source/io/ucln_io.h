















#ifndef __UCLN_IO_H__
#define __UCLN_IO_H__

#include "unicode/utypes.h"
#include "ucln.h"




typedef enum ECleanupIOType {
    UCLN_IO_START = -1,
    UCLN_IO_LOCBUND,
    UCLN_IO_PRINTF,
    UCLN_IO_COUNT 
} ECleanupIOType;



U_CFUNC void U_EXPORT2 ucln_io_registerCleanup(ECleanupIOType type,
                                                 cleanupFunc *func);

#endif
