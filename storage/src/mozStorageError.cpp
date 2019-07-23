






































#include "mozStorageError.h"

namespace mozilla {
namespace storage {




Error::Error(int aResult,
             const char *aMessage)
: mResult(aResult)
, mMessage(aMessage)
{
}





NS_IMPL_THREADSAFE_ISUPPORTS1(
  Error,
  mozIStorageError
)




NS_IMETHODIMP
Error::GetResult(PRInt32 *_result)
{
  *_result = mResult;
  return NS_OK;
}

NS_IMETHODIMP
Error::GetMessage(nsACString &_message)
{
  _message = mMessage;
  return NS_OK;
}

} 
} 
