







































#include "nsError.h"
#include "prtypes.h"

PR_BEGIN_EXTERN_C







typedef void    (*pfnXPIProgress)(const char* msg, PRInt32 val, PRInt32 max);
















PR_EXTERN(nsresult) XPI_Init( const char*       aProgramDir,
                              const char*       aLogName,
                              pfnXPIProgress    progressCB );











PR_EXTERN(PRInt32) XPI_Install( const char*    file,
                                const char* args, 
                                long flags         );






PR_EXTERN(void) XPI_Exit();

PR_END_EXTERN_C

