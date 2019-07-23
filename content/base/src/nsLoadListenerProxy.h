




































#ifndef nsLoadListenerProxy_h
#define nsLoadListenerProxy_h

#include "nsIDOMLoadListener.h"
#include "nsWeakReference.h"













class nsLoadListenerProxy : public nsIDOMLoadListener {
public:
  nsLoadListenerProxy(nsWeakPtr aParent);
  virtual ~nsLoadListenerProxy();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  NS_IMETHOD Load(nsIDOMEvent* aEvent);
  NS_IMETHOD BeforeUnload(nsIDOMEvent* aEvent);
  NS_IMETHOD Unload(nsIDOMEvent* aEvent);
  NS_IMETHOD Abort(nsIDOMEvent* aEvent);
  NS_IMETHOD Error(nsIDOMEvent* aEvent);

protected:
  nsWeakPtr  mParent;
};

#endif
