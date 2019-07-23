






































#ifndef _mozStorageUnicodeFunctions_h_
#define _mozStorageUnicodeFunctions_h_

#include "sqlite3.h"
#include "nscore.h"

namespace StorageUnicodeFunctions {

  






  NS_HIDDEN_(int) RegisterFunctions(sqlite3 *aDB);

  










  NS_HIDDEN_(void) caseFunction(sqlite3_context *p,
                                int aArgc,
                                sqlite3_value **aArgv);

  









  NS_HIDDEN_(void) likeFunction(sqlite3_context *p,
                                int aArgc,
                                sqlite3_value **aArgv);
}

#endif 
