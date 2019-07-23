






































#ifndef mozilla_storage_mozStorageAsyncStatementParams_h_
#define mozilla_storage_mozStorageAsyncStatementParams_h_

#include "mozIStorageStatementWrapper.h"
#include "nsIXPCScriptable.h"

class mozIStorageAsyncStatement;

namespace mozilla {
namespace storage {

class AsyncStatement;





class AsyncStatementParams : public mozIStorageStatementParams
                           , public nsIXPCScriptable
{
public:
  AsyncStatementParams(AsyncStatement *aStatement);

  
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTPARAMS
  NS_DECL_NSIXPCSCRIPTABLE

protected:
  AsyncStatement *mStatement;

  friend class AsyncStatement;
};

} 
} 

#endif 
