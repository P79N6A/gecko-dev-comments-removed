





#ifndef mozilla_storage_mozStorageAsyncStatementParams_h_
#define mozilla_storage_mozStorageAsyncStatementParams_h_

#include "mozIStorageStatementParams.h"
#include "nsIXPCScriptable.h"
#include "mozilla/Attributes.h"

class mozIStorageAsyncStatement;

namespace mozilla {
namespace storage {

class AsyncStatement;





class AsyncStatementParams MOZ_FINAL : public mozIStorageStatementParams
                                     , public nsIXPCScriptable
{
public:
  explicit AsyncStatementParams(AsyncStatement *aStatement);

  
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTPARAMS
  NS_DECL_NSIXPCSCRIPTABLE

protected:
  virtual ~AsyncStatementParams() {}

  AsyncStatement *mStatement;

  friend class AsyncStatement;
};

} 
} 

#endif 
