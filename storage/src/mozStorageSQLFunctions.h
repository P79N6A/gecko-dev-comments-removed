





#ifndef mozStorageSQLFunctions_h
#define mozStorageSQLFunctions_h

#include "sqlite3.h"
#include "nscore.h"

namespace mozilla {
namespace storage {








int registerFunctions(sqlite3 *aDB);















void caseFunction(sqlite3_context *aCtx,
                              int aArgc,
                              sqlite3_value **aArgv);












void likeFunction(sqlite3_context *aCtx,
                              int aArgc,
                              sqlite3_value **aArgv);












void levenshteinDistanceFunction(sqlite3_context *aCtx,
                                             int aArgc,
                                             sqlite3_value **aArgv);

} 
} 

#endif 
