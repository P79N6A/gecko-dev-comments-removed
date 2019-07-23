



































#include "nsPrivateBrowsingServiceWrapper.h"
#include "nsServiceManagerUtils.h"
#include "jsapi.h"
#include "nsIJSContextStack.h"

NS_IMPL_ISUPPORTS2(nsPrivateBrowsingServiceWrapper, nsIPrivateBrowsingService, nsIObserver)

nsresult
nsPrivateBrowsingServiceWrapper::Init()
{
  nsresult rv;
  mPBService = do_GetService("@mozilla.org/privatebrowsing;1", &rv);
  return rv;
}

nsresult
nsPrivateBrowsingServiceWrapper::PrepareCall(nsIJSContextStack ** aJSStack)
{
  nsresult rv = CallGetService("@mozilla.org/js/xpc/ContextStack;1", aJSStack);
  NS_ENSURE_SUCCESS(rv, rv);

  if (*aJSStack) {
    rv = (*aJSStack)->Push(nsnull);
    if (NS_FAILED(rv))
      *aJSStack = nsnull;
  }
  return rv;
}

void
nsPrivateBrowsingServiceWrapper::FinishCall(nsIJSContextStack * aJSStack)
{
  if (aJSStack) {
    JSContext *cx;
    aJSStack->Pop(&cx);
    NS_ASSERTION(cx == nsnull, "JSContextStack mismatch");
  }
}

NS_IMETHODIMP
nsPrivateBrowsingServiceWrapper::GetPrivateBrowsingEnabled(PRBool *aPrivateBrowsingEnabled)
{
  if (!aPrivateBrowsingEnabled)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIJSContextStack> jsStack;
  nsresult rv = PrepareCall(getter_AddRefs(jsStack));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mPBService->GetPrivateBrowsingEnabled(aPrivateBrowsingEnabled);
  FinishCall(jsStack);
  return rv;
}

NS_IMETHODIMP
nsPrivateBrowsingServiceWrapper::SetPrivateBrowsingEnabled(PRBool aPrivateBrowsingEnabled)
{
  nsCOMPtr<nsIJSContextStack> jsStack;
  nsresult rv = PrepareCall(getter_AddRefs(jsStack));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mPBService->SetPrivateBrowsingEnabled(aPrivateBrowsingEnabled);
  FinishCall(jsStack);
  return rv;
}

NS_IMETHODIMP
nsPrivateBrowsingServiceWrapper::GetAutoStarted(PRBool *aAutoStarted)
{
  if (!aAutoStarted)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIJSContextStack> jsStack;
  nsresult rv = PrepareCall(getter_AddRefs(jsStack));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mPBService->GetAutoStarted(aAutoStarted);
  FinishCall(jsStack);
  return rv;
}

NS_IMETHODIMP
nsPrivateBrowsingServiceWrapper::RemoveDataFromDomain(const nsACString & aDomain)
{
  nsCOMPtr<nsIJSContextStack> jsStack;
  nsresult rv = PrepareCall(getter_AddRefs(jsStack));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mPBService->RemoveDataFromDomain(aDomain);
  FinishCall(jsStack);
  return rv;
}

NS_IMETHODIMP
nsPrivateBrowsingServiceWrapper::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  nsCOMPtr<nsIJSContextStack> jsStack;
  nsresult rv = PrepareCall(getter_AddRefs(jsStack));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIObserver> observer(do_QueryInterface(mPBService));
  NS_ENSURE_TRUE(observer, NS_ERROR_FAILURE);
  rv = observer->Observe(aSubject, aTopic, aData);
  FinishCall(jsStack);
  return rv;
}
