





#ifndef mozilla_storage_mozStorageAsyncStatement_h_
#define mozilla_storage_mozStorageAsyncStatement_h_

#include "nsAutoPtr.h"
#include "nsString.h"

#include "nsTArray.h"

#include "mozStorageBindingParamsArray.h"
#include "mozStorageStatementData.h"
#include "mozIStorageAsyncStatement.h"
#include "StorageBaseStatementInternal.h"
#include "mozilla/Attributes.h"

class nsIXPConnectJSObjectHolder;

namespace mozilla {
namespace storage {

class AsyncStatementJSHelper;
class Connection;

class AsyncStatement final : public mozIStorageAsyncStatement
                           , public StorageBaseStatementInternal
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZISTORAGEASYNCSTATEMENT
  NS_DECL_MOZISTORAGEBASESTATEMENT
  NS_DECL_MOZISTORAGEBINDINGPARAMS
  NS_DECL_STORAGEBASESTATEMENTINTERNAL

  AsyncStatement();

  










  nsresult initialize(Connection *aDBConnection,
                      sqlite3 *aNativeConnection,
                      const nsACString &aSQLStatement);

  



  inline already_AddRefed<BindingParamsArray> bindingParamsArray()
  {
    return mParamsArray.forget();
  }


private:
  ~AsyncStatement();

  



  mozIStorageBindingParams *getParams();

  



  nsCString mSQLString;

  



  nsRefPtr<BindingParamsArray> mParamsArray;

  


  nsMainThreadPtrHandle<nsIXPConnectJSObjectHolder> mStatementParamsHolder;

  


  bool mFinalized;

  



  friend class AsyncStatementJSHelper;
};

} 
} 

#endif 
