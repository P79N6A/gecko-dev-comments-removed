






































#ifndef _mozStorageBindingParams_h_
#define _mozStorageBindingParams_h_

#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIVariant.h"

#include "mozStorageBindingParamsArray.h"
#include "mozStorageStatement.h"
#include "mozIStorageBindingParams.h"

class mozIStorageError;
struct sqlite3_stmt;

namespace mozilla {
namespace storage {

class BindingParams : public mozIStorageBindingParams
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEBINDINGPARAMS

  



  void lock();

  


  void unlock();

  


  const BindingParamsArray *getOwner() const;

  







  already_AddRefed<mozIStorageError> bind(sqlite3_stmt *aStatement);

  BindingParams(BindingParamsArray *aOwningArray,
                Statement *aOwningStatement);

private:
  nsRefPtr<BindingParamsArray> mOwningArray;
  Statement *mOwningStatement;
  nsCOMArray<nsIVariant> mParameters;
  PRUint32 mParamCount;
  bool mLocked;
};

} 
} 

#endif 
