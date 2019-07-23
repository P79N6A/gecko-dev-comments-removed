





































#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsAutoPtr.h"

class nsProxyReleaseEvent : public nsRunnable
{
public:
    nsProxyReleaseEvent(nsISupports *doomed)
        : mDoomed(doomed) {
    }

    NS_IMETHOD Run()
    {
        mDoomed->Release();
        return NS_OK;
    }

private:
    nsISupports *mDoomed;
};

nsresult
NS_ProxyRelease(nsIEventTarget *target, nsISupports *doomed,
                PRBool alwaysProxy)
{
    nsresult rv;

    if (!target) {
        NS_RELEASE(doomed);
        return NS_OK;
    }

    if (!alwaysProxy) {
        PRBool onCurrentThread = PR_FALSE;
        rv = target->IsOnCurrentThread(&onCurrentThread);
        if (NS_SUCCEEDED(rv) && onCurrentThread) {
            NS_RELEASE(doomed);
            return NS_OK;
        }
    }

    nsRefPtr<nsIRunnable> ev = new nsProxyReleaseEvent(doomed);
    if (!ev) {
        
        
        return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = target->Dispatch(ev, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
        NS_WARNING("failed to post proxy release event");
        
        
    }
    return rv;
}
