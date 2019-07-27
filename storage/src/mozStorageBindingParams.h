





#ifndef mozStorageBindingParams_h
#define mozStorageBindingParams_h

#include "nsCOMArray.h"
#include "nsIVariant.h"
#include "nsInterfaceHashtable.h"

#include "mozStorageBindingParamsArray.h"
#include "mozStorageStatement.h"
#include "mozStorageAsyncStatement.h"

#include "mozIStorageBindingParams.h"
#include "IStorageBindingParamsInternal.h"

namespace mozilla {
namespace storage {

class BindingParams : public mozIStorageBindingParams
                    , public IStorageBindingParamsInternal
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZISTORAGEBINDINGPARAMS
  NS_DECL_ISTORAGEBINDINGPARAMSINTERNAL

  



  void lock();

  







  void unlock(Statement *aOwningStatement);

  



  const mozIStorageBindingParamsArray *getOwner() const;

  BindingParams(mozIStorageBindingParamsArray *aOwningArray,
                Statement *aOwningStatement);
protected:
  virtual ~BindingParams() {}

  explicit BindingParams(mozIStorageBindingParamsArray *aOwningArray);
  nsCOMArray<nsIVariant> mParameters;
  bool mLocked;

private:

  






  nsCOMPtr<mozIStorageBindingParamsArray> mOwningArray;
  




  Statement *mOwningStatement;
  uint32_t mParamCount;
};










class AsyncBindingParams : public BindingParams
{
public:
  NS_IMETHOD BindByName(const nsACString & aName,
                                      nsIVariant *aValue);
  NS_IMETHOD BindByIndex(uint32_t aIndex, nsIVariant *aValue);

  virtual already_AddRefed<mozIStorageError> bind(sqlite3_stmt * aStatement);

  explicit AsyncBindingParams(mozIStorageBindingParamsArray *aOwningArray);
  virtual ~AsyncBindingParams() {}

private:
  nsInterfaceHashtable<nsCStringHashKey, nsIVariant> mNamedParameters;

  struct NamedParameterIterationClosureThunk
  {
    AsyncBindingParams *self;
    sqlite3_stmt *statement;
    nsCOMPtr<mozIStorageError> err;
  };

  static PLDHashOperator iterateOverNamedParameters(const nsACString &aName,
                                                    nsIVariant *aValue,
                                                    void *voidClosureThunk);
};

} 
} 

#endif 
