





#ifndef mozStorageStatement_h
#define mozStorageStatement_h

#include "nsAutoPtr.h"
#include "nsString.h"

#include "nsTArray.h"

#include "mozStorageBindingParamsArray.h"
#include "mozStorageStatementData.h"
#include "mozIStorageStatement.h"
#include "mozIStorageValueArray.h"
#include "StorageBaseStatementInternal.h"
#include "mozilla/Attributes.h"

class nsIXPConnectJSObjectHolder;
struct sqlite3_stmt;

namespace mozilla {
namespace storage {
class StatementJSHelper;
class Connection;

class Statement MOZ_FINAL : public mozIStorageStatement
                          , public mozIStorageValueArray
                          , public StorageBaseStatementInternal
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENT
  NS_DECL_MOZISTORAGEBASESTATEMENT
  NS_DECL_MOZISTORAGEBINDINGPARAMS
  
  NS_DECL_STORAGEBASESTATEMENTINTERNAL

  Statement();

  








  nsresult initialize(Connection *aDBConnection,
                      const nsACString &aSQLStatement);


  


  inline sqlite3_stmt *nativeStatement() { return mDBStatement; }

  



  inline already_AddRefed<BindingParamsArray> bindingParamsArray()
  {
    return mParamsArray.forget();
  }

private:
    ~Statement();

    sqlite3_stmt *mDBStatement;
    uint32_t mParamCount;
    uint32_t mResultColumnCount;
    nsTArray<nsCString> mColumnNames;
    bool mExecuting;

    



    mozIStorageBindingParams *getParams();

    



    nsRefPtr<BindingParamsArray> mParamsArray;

    



    nsCOMPtr<nsIXPConnectJSObjectHolder> mStatementParamsHolder;
    nsCOMPtr<nsIXPConnectJSObjectHolder> mStatementRowHolder;

  







  nsresult internalFinalize(bool aDestructing);

    friend class StatementJSHelper;
};

} 
} 

#endif 
