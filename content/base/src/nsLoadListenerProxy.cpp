




































#include "nsLoadListenerProxy.h"
#include "nsIDOMEvent.h"

nsLoadListenerProxy::nsLoadListenerProxy(nsWeakPtr aParent) : mParent(aParent)
{
}

nsLoadListenerProxy::~nsLoadListenerProxy()
{
}

NS_IMPL_ISUPPORTS2(nsLoadListenerProxy, nsIDOMEventListener,
                   nsIDOMLoadListener)

NS_IMETHODIMP
nsLoadListenerProxy::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMLoadListener> listener(do_QueryReferent(mParent));

  if (listener) {
    return listener->HandleEvent(aEvent);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsLoadListenerProxy::Load(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMLoadListener> listener(do_QueryReferent(mParent));

  if (listener) {
    return listener->Load(aEvent);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLoadListenerProxy::BeforeUnload(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMLoadListener> listener(do_QueryReferent(mParent));

  if (listener) {
    return listener->BeforeUnload(aEvent);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsLoadListenerProxy::Unload(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMLoadListener> listener(do_QueryReferent(mParent));

  if (listener) {
    return listener->Unload(aEvent);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsLoadListenerProxy::Abort(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMLoadListener> listener(do_QueryReferent(mParent));

  if (listener) {
    return listener->Abort(aEvent);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsLoadListenerProxy::Error(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMLoadListener> listener(do_QueryReferent(mParent));

  if (listener) {
    return listener->Error(aEvent);
  }
  
  return NS_OK;
}
