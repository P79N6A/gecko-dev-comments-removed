










































#include "nsIAccessProxy.h"
#include "nsIDOMEventListener.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIObserver.h"
#include "nsIAppStartupNotifier.h"














class nsAccessProxy : public nsIDOMEventListener,
                      public nsIObserver,
                      public nsIWebProgressListener,
                      public nsSupportsWeakReference
{
public:
  nsAccessProxy();
  virtual ~nsAccessProxy();

  NS_DEFINE_STATIC_CID_ACCESSOR(NS_ACCESSPROXY_CID);

  NS_DECL_ISUPPORTS  
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIWEBPROGRESSLISTENER

  
  NS_IMETHOD HandleEvent(nsIDOMEvent *event);  

  static nsAccessProxy *GetInstance();
  static void ReleaseInstance(void);

private:
  static nsAccessProxy *mInstance;
};
