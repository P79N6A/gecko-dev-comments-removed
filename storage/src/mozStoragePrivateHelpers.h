







































#ifndef _mozStoragePrivateHelpers_h_
#define _mozStoragePrivateHelpers_h_





#include "sqlite3.h"
#include "nsIVariant.h"
#include "mozStorage.h"

namespace mozilla {
namespace storage {




#define ENSURE_INDEX_VALUE(aIndex, aCount) \
  NS_ENSURE_TRUE(aIndex < aCount, NS_ERROR_INVALID_ARG)











nsresult convertResultCode(int aSQLiteResultCode);










void checkAndLogStatementPerformance(sqlite3_stmt *aStatement);




template <typename T>
int variantToSQLiteT(T aObj, nsIVariant *aValue);
#include "variantToSQLiteT_impl.h" 

} 
} 

#endif 
