





#ifndef mozStoragePrivateHelpers_h
#define mozStoragePrivateHelpers_h





#include "sqlite3.h"
#include "nsIVariant.h"
#include "nsError.h"
#include "jsapi.h"
#include "nsAutoPtr.h"

class mozIStorageCompletionCallback;
class mozIStorageBaseStatement;
class mozIStorageBindingParams;
class nsIRunnable;

namespace mozilla {
namespace storage {




#define ENSURE_INDEX_VALUE(aIndex, aCount) \
  NS_ENSURE_TRUE(aIndex < aCount, NS_ERROR_INVALID_ARG)











nsresult convertResultCode(int aSQLiteResultCode);










void checkAndLogStatementPerformance(sqlite3_stmt *aStatement);













nsIVariant *convertJSValToVariant(JSContext *aCtx, jsval aValue);








already_AddRefed<nsIRunnable> newCompletionEvent(
  mozIStorageCompletionCallback *aCallback
);

} 
} 

#endif 
