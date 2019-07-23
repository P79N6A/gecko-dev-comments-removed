





































#ifndef mozilla_storage_mozStorageAsyncStatementJSHelper_h_
#define mozilla_storage_mozStorageAsyncStatementJSHelper_h_

#include "nsIXPCScriptable.h"

class AsyncStatement;

namespace mozilla {
namespace storage {






class AsyncStatementJSHelper : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

private:
  nsresult getParams(AsyncStatement *, JSContext *, JSObject *, jsval *);
};

} 
} 

#endif 
