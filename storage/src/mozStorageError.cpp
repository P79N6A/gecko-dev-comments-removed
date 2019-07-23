






































#include "mozStorageError.h"








NS_IMPL_THREADSAFE_ISUPPORTS1(mozStorageError, mozIStorageError)

mozStorageError::mozStorageError(int aResult, const char *aMessage) :
    mResult(aResult)
  , mMessage(aMessage)
{
}




NS_IMETHODIMP
mozStorageError::GetResult(PRInt32 *_result)
{
  *_result = mResult;
  return NS_OK;
}

NS_IMETHODIMP
mozStorageError::GetMessage(nsACString &_message)
{
  _message = mMessage;
  return NS_OK;
}
