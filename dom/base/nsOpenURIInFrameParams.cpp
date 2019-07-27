




#include "nsOpenURIInFrameParams.h"

NS_IMPL_ISUPPORTS(nsOpenURIInFrameParams, nsIOpenURIInFrameParams)

nsOpenURIInFrameParams::nsOpenURIInFrameParams() :
  mIsPrivate(false)
{
}

nsOpenURIInFrameParams::~nsOpenURIInFrameParams() {
}


NS_IMETHODIMP
nsOpenURIInFrameParams::GetReferrer(nsAString& aReferrer)
{
  aReferrer = mReferrer;
  return NS_OK;
}
NS_IMETHODIMP
nsOpenURIInFrameParams::SetReferrer(const nsAString& aReferrer)
{
  mReferrer = aReferrer;
  return NS_OK;
}


NS_IMETHODIMP
nsOpenURIInFrameParams::GetIsPrivate(bool* aIsPrivate)
{
  NS_ENSURE_ARG_POINTER(aIsPrivate);
  *aIsPrivate = mIsPrivate;
  return NS_OK;
}
NS_IMETHODIMP
nsOpenURIInFrameParams::SetIsPrivate(bool aIsPrivate)
{
  mIsPrivate = aIsPrivate;
  return NS_OK;
}
