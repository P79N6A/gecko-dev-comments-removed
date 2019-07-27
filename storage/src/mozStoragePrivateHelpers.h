





#ifndef mozStoragePrivateHelpers_h
#define mozStoragePrivateHelpers_h





#include "sqlite3.h"
#include "nsIVariant.h"
#include "nsError.h"
#include "nsAutoPtr.h"
#include "js/TypeDecls.h"
#include "Variant.h"

class mozIStorageCompletionCallback;
class nsIRunnable;

namespace mozilla {
namespace storage {




#define ENSURE_INDEX_VALUE(aIndex, aCount) \
  NS_ENSURE_TRUE(aIndex < aCount, NS_ERROR_INVALID_ARG)











nsresult convertResultCode(int aSQLiteResultCode);










void checkAndLogStatementPerformance(sqlite3_stmt *aStatement);













nsIVariant *convertJSValToVariant(JSContext *aCtx, JS::Value aValue);









Variant_base *convertVariantToStorageVariant(nsIVariant *aVariant);








already_AddRefed<nsIRunnable> newCompletionEvent(
  mozIStorageCompletionCallback *aCallback
);

} 
} 

#endif 
