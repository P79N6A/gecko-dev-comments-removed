






































#ifndef _mozStorageSQLFunctions_h_
#define _mozStorageSQLFunctions_h_

#include "sqlite3.h"
#include "nscore.h"

namespace mozilla {
namespace storage {








NS_HIDDEN_(int) registerFunctions(sqlite3 *aDB);















NS_HIDDEN_(void) caseFunction(sqlite3_context *aCtx,
                              int aArgc,
                              sqlite3_value **aArgv);












NS_HIDDEN_(void) likeFunction(sqlite3_context *aCtx,
                              int aArgc,
                              sqlite3_value **aArgv);

} 
} 

#endif 
