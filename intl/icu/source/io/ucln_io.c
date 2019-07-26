















#include "ucln.h"
#include "ucln_io.h"
#include "umutex.h"
#include "uassert.h"

#ifndef U_IO_IMPLEMENTATION
#error U_IO_IMPLEMENTATION not set - must be set for all ICU source files in io/ - see http://userguide.icu-project.org/howtouseicu
#endif



#define UCLN_TYPE UCLN_IO
#include "ucln_imp.h"


static const char copyright[] = U_COPYRIGHT_STRING;

static cleanupFunc *gCleanupFunctions[UCLN_IO_COUNT];

static UBool io_cleanup(void)
{
    ECleanupIOType libType = UCLN_IO_START;

    while (++libType<UCLN_IO_COUNT) {
        if (gCleanupFunctions[libType])
        {
            gCleanupFunctions[libType]();
            gCleanupFunctions[libType] = NULL;
        }
    }
#if !UCLN_NO_AUTO_CLEANUP && (defined(UCLN_AUTO_ATEXIT) || defined(UCLN_AUTO_LOCAL))
    ucln_unRegisterAutomaticCleanup();
#endif
    return TRUE;
}

void ucln_io_registerCleanup(ECleanupIOType type,
                               cleanupFunc *func)
{
    U_ASSERT(UCLN_IO_START < type && type < UCLN_IO_COUNT);
    ucln_registerCleanup(UCLN_IO, io_cleanup);
    if (UCLN_IO_START < type && type < UCLN_IO_COUNT)
    {
        gCleanupFunctions[type] = func;
    }

#if !UCLN_NO_AUTO_CLEANUP && (defined(UCLN_AUTO_ATEXIT) || defined(UCLN_AUTO_LOCAL))
    ucln_registerAutomaticCleanup();
#endif
}

