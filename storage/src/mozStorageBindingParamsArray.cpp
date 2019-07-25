





#include "mozStorageBindingParamsArray.h"
#include "mozStorageBindingParams.h"
#include "StorageBaseStatementInternal.h"

namespace mozilla {
namespace storage {




BindingParamsArray::BindingParamsArray(
  StorageBaseStatementInternal *aOwningStatement
)
: mOwningStatement(aOwningStatement)
, mLocked(false)
{
}

void
BindingParamsArray::lock()
{
  NS_ASSERTION(mLocked == false, "Array has already been locked!");
  mLocked = true;

  
  
  mOwningStatement = nullptr;
}

const StorageBaseStatementInternal *
BindingParamsArray::getOwner() const
{
  return mOwningStatement;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(
  BindingParamsArray,
  mozIStorageBindingParamsArray
)




NS_IMETHODIMP
BindingParamsArray::NewBindingParams(mozIStorageBindingParams **_params)
{
  NS_ENSURE_FALSE(mLocked, NS_ERROR_UNEXPECTED);

  nsCOMPtr<mozIStorageBindingParams> params(
    mOwningStatement->newBindingParams(this));
  NS_ENSURE_TRUE(params, NS_ERROR_UNEXPECTED);

  params.forget(_params);
  return NS_OK;
}

NS_IMETHODIMP
BindingParamsArray::AddParams(mozIStorageBindingParams *aParameters)
{
  NS_ENSURE_FALSE(mLocked, NS_ERROR_UNEXPECTED);

  BindingParams *params = static_cast<BindingParams *>(aParameters);

  
  if (params->getOwner() != this)
    return NS_ERROR_UNEXPECTED;

  NS_ENSURE_TRUE(mArray.AppendElement(params), NS_ERROR_OUT_OF_MEMORY);

  
  params->lock();

  return NS_OK;
}

NS_IMETHODIMP
BindingParamsArray::GetLength(PRUint32 *_length)
{
  *_length = length();
  return NS_OK;
}

} 
} 
