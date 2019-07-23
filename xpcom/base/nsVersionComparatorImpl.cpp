




































#include "nsVersionComparatorImpl.h"
#include "nsVersionComparator.h"
#include "nsString.h"

NS_IMPL_ISUPPORTS1(nsVersionComparatorImpl, nsIVersionComparator)

NS_IMETHODIMP
nsVersionComparatorImpl::Compare(const nsACString& A, const nsACString& B,
				 PRInt32 *aResult)
{
  *aResult = NS_CompareVersions(PromiseFlatCString(A).get(),
				PromiseFlatCString(B).get());

  return NS_OK;
}
