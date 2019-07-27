



#ifndef nsStreamListenerWrapper_h__
#define nsStreamListenerWrapper_h__

#include "nsCOMPtr.h"
#include "nsIStreamListener.h"
#include "nsIRequestObserver.h"
#include "nsIThreadRetargetableStreamListener.h"
#include "mozilla/Attributes.h"



class nsStreamListenerWrapper MOZ_FINAL : public nsIStreamListener
                                        , public nsIThreadRetargetableStreamListener
{
public:
  explicit nsStreamListenerWrapper(nsIStreamListener *listener)
    : mListener(listener)
  {
    NS_ASSERTION(mListener, "no stream listener specified");
  }

  NS_DECL_ISUPPORTS
  NS_FORWARD_NSIREQUESTOBSERVER(mListener->)
  NS_FORWARD_NSISTREAMLISTENER(mListener->)
  NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER

private:
  ~nsStreamListenerWrapper() {}
  nsCOMPtr<nsIStreamListener> mListener;
};

#endif 

