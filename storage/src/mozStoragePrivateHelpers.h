







































#ifndef _mozStoragePrivateHelpers_h_
#define _mozStoragePrivateHelpers_h_





#include "mozStorage.h"

struct sqlite3_stmt;

namespace mozilla {
namespace storage {




#define ENSURE_INDEX_VALUE(aIndex, aCount) \
  NS_ENSURE_TRUE(aIndex < aCount, NS_ERROR_INVALID_ARG)











nsresult convertResultCode(int aSQLiteResultCode);










void checkAndLogStatementPerformance(sqlite3_stmt *aStatement);

} 
} 

#endif 
