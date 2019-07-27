





#include "mozilla/GenericFactory.h"

namespace mozilla {

NS_IMPL_ISUPPORTS(GenericFactory, nsIFactory)

NS_IMETHODIMP
GenericFactory::CreateInstance(nsISupports* aOuter, REFNSIID aIID,
                               void** aResult)
{
  return mCtor(aOuter, aIID, aResult);
}

NS_IMETHODIMP
GenericFactory::LockFactory(bool aLock)
{
  NS_ERROR("Vestigial method, never called!");
  return NS_ERROR_FAILURE;
}

} 
