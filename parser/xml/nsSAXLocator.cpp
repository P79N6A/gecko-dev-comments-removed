




#include "nsSAXLocator.h"

NS_IMPL_ISUPPORTS(nsSAXLocator, nsISAXLocator)

nsSAXLocator::nsSAXLocator(nsString& aPublicId,
                           nsString& aSystemId,
                           int32_t aLineNumber,
                           int32_t aColumnNumber) :
  mPublicId(aPublicId),
  mSystemId(aSystemId),
  mLineNumber(aLineNumber),
  mColumnNumber(aColumnNumber)
{
}

NS_IMETHODIMP
nsSAXLocator::GetColumnNumber(int32_t *aColumnNumber)
{
  *aColumnNumber = mColumnNumber;
  return NS_OK;
}

NS_IMETHODIMP
nsSAXLocator::GetLineNumber(int32_t *aLineNumber)
{
  *aLineNumber = mLineNumber;
  return NS_OK;
}

NS_IMETHODIMP
nsSAXLocator::GetPublicId(nsAString &aPublicId)
{
  aPublicId = mPublicId;
  return NS_OK;
}

NS_IMETHODIMP
nsSAXLocator::GetSystemId(nsAString &aSystemId)
{
  aSystemId = mSystemId;
  return NS_OK;
}
