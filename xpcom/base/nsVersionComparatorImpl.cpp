





#include "nsVersionComparatorImpl.h"
#include "nsVersionComparator.h"
#include "nsString.h"

NS_IMPL_ISUPPORTS(nsVersionComparatorImpl, nsIVersionComparator)

NS_IMETHODIMP
nsVersionComparatorImpl::Compare(const nsACString& aStr1,
                                 const nsACString& aStr2,
                                 int32_t* aResult)
{
  *aResult = mozilla::CompareVersions(PromiseFlatCString(aStr1).get(),
                                      PromiseFlatCString(aStr2).get());

  return NS_OK;
}
