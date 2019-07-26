



#include "nsStreamListenerWrapper.h"
#ifdef DEBUG
#include "MainThreadUtils.h"
#endif

NS_IMPL_ISUPPORTS3(nsStreamListenerWrapper,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIThreadRetargetableStreamListener)

NS_IMETHODIMP
nsStreamListenerWrapper::CheckListenerChain()
{
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread!");
    nsresult rv = NS_OK;
    nsCOMPtr<nsIThreadRetargetableStreamListener> retargetableListener =
        do_QueryInterface(mListener, &rv);
    if (retargetableListener) {
        rv = retargetableListener->CheckListenerChain();
    }
    return rv;
}
