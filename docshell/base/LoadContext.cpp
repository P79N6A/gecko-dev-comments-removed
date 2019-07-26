





#include "mozilla/LoadContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsServiceManagerUtils.h"
#include "nsContentUtils.h"
#include "mozIApplication.h"

namespace mozilla {

NS_IMPL_ISUPPORTS1(LoadContext, nsILoadContext)





NS_IMETHODIMP
LoadContext::GetAssociatedWindow(nsIDOMWindow**)
{
  MOZ_ASSERT(mIsNotNull);

  
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
LoadContext::GetTopWindow(nsIDOMWindow**)
{
  MOZ_ASSERT(mIsNotNull);

  
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
LoadContext::GetTopFrameElement(nsIDOMElement** aElement)
{
  nsCOMPtr<nsIDOMElement> element = do_QueryReferent(mTopFrameElement);
  element.forget(aElement);
  return NS_OK;
}

NS_IMETHODIMP
LoadContext::IsAppOfType(uint32_t, bool*)
{
  MOZ_ASSERT(mIsNotNull);

  
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
LoadContext::GetIsContent(bool* aIsContent)
{
  MOZ_ASSERT(mIsNotNull);

  NS_ENSURE_ARG_POINTER(aIsContent);

  *aIsContent = mIsContent;
  return NS_OK;
}

NS_IMETHODIMP
LoadContext::GetUsePrivateBrowsing(bool* aUsePrivateBrowsing)
{
  MOZ_ASSERT(mIsNotNull);

  NS_ENSURE_ARG_POINTER(aUsePrivateBrowsing);

  *aUsePrivateBrowsing = mUsePrivateBrowsing;
  return NS_OK;
}

NS_IMETHODIMP
LoadContext::SetUsePrivateBrowsing(bool aUsePrivateBrowsing)
{
  MOZ_ASSERT(mIsNotNull);

  
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
LoadContext::SetPrivateBrowsing(bool aUsePrivateBrowsing)
{
  MOZ_ASSERT(mIsNotNull);

  
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
LoadContext::GetIsInBrowserElement(bool* aIsInBrowserElement)
{
  MOZ_ASSERT(mIsNotNull);

  NS_ENSURE_ARG_POINTER(aIsInBrowserElement);

  *aIsInBrowserElement = mIsInBrowserElement;
  return NS_OK;
}

NS_IMETHODIMP
LoadContext::GetAppId(uint32_t* aAppId)
{
  MOZ_ASSERT(mIsNotNull);

  NS_ENSURE_ARG_POINTER(aAppId);

  *aAppId = mAppId;
  return NS_OK;
}

} 
