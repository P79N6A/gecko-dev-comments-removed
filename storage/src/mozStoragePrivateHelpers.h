







































#ifndef _mozStoragePrivateHelpers_h_
#define _mozStoragePrivateHelpers_h_

#include "mozStorage.h"





namespace mozilla {
namespace storage {








nsresult convertResultCode(int aSQLiteResultCode);










void checkAndLogStatementPerformance(sqlite3_stmt *aStatement);

} 
} 

#endif 
