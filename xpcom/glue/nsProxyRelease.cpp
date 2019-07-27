





#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"

class nsProxyReleaseEvent : public nsRunnable
{
public:
  explicit nsProxyReleaseEvent(nsISupports* aDoomed) : mDoomed(aDoomed) {}

  NS_IMETHOD Run()
  {
    mDoomed->Release();
    return NS_OK;
  }

private:
  nsISupports* MOZ_OWNING_REF mDoomed;
};

nsresult
NS_ProxyRelease(nsIEventTarget* aTarget, nsISupports* aDoomed,
                bool aAlwaysProxy)
{
  nsresult rv;

  if (!aDoomed) {
    
    return NS_OK;
  }

  if (!aTarget) {
    NS_RELEASE(aDoomed);
    return NS_OK;
  }

  if (!aAlwaysProxy) {
    bool onCurrentThread = false;
    rv = aTarget->IsOnCurrentThread(&onCurrentThread);
    if (NS_SUCCEEDED(rv) && onCurrentThread) {
      NS_RELEASE(aDoomed);
      return NS_OK;
    }
  }

  nsCOMPtr<nsIRunnable> ev = new nsProxyReleaseEvent(aDoomed);
  if (!ev) {
    
    
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = aTarget->Dispatch(ev, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to post proxy release event");
    
    
  }
  return rv;
}
