







































#ifndef mozilla_storage_mozStorageAsyncStatement_h_
#define mozilla_storage_mozStorageAsyncStatement_h_

#include "nsAutoPtr.h"
#include "nsString.h"

#include "nsTArray.h"

#include "mozStorageBindingParamsArray.h"
#include "mozStorageStatementData.h"
#include "mozIStorageAsyncStatement.h"
#include "StorageBaseStatementInternal.h"

class nsIXPConnectJSObjectHolder;
struct sqlite3_stmt;

namespace mozilla {
namespace storage {

class AsyncStatementJSHelper;
class Connection;

class AsyncStatement : public mozIStorageAsyncStatement
                     , public StorageBaseStatementInternal
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEASYNCSTATEMENT
  NS_DECL_MOZISTORAGEBASESTATEMENT
  NS_DECL_MOZISTORAGEBINDINGPARAMS
  NS_DECL_STORAGEBASESTATEMENTINTERNAL

  AsyncStatement();

  








  nsresult initialize(Connection *aDBConnection,
                      const nsACString &aSQLStatement);

  



  inline already_AddRefed<BindingParamsArray> bindingParamsArray()
  {
    return mParamsArray.forget();
  }


private:
  ~AsyncStatement();

  




  void cleanupJSHelpers();

  



  mozIStorageBindingParams *getParams();

  



  nsCString mSQLString;

  



  nsRefPtr<BindingParamsArray> mParamsArray;

  


  nsCOMPtr<nsIXPConnectJSObjectHolder> mStatementParamsHolder;

  


  bool mFinalized;

  



  friend class AsyncStatementJSHelper;
};

} 
} 

#endif 
