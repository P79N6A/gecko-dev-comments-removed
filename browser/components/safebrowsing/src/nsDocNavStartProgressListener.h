



































#include "nsIDocNavStartProgressListener.h"
#include "nsIObserver.h"
#include "nsIWebProgressListener.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"


class nsITimer;
class nsIRequest;
class nsIURI;

class nsDocNavStartProgressListener : public nsIDocNavStartProgressListener,
                                      public nsIWebProgressListener,
                                      public nsIObserver,
                                      public nsSupportsWeakReference
{
public:
  nsDocNavStartProgressListener();
  virtual ~nsDocNavStartProgressListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOCNAVSTARTPROGRESSLISTENER
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIOBSERVER

protected:

  PRBool mEnabled;
  PRUint32 mDelay;
  nsCOMPtr<nsIDocNavStartProgressCallback> mCallback;

  
  nsCOMArray<nsIRequest> mRequests;
  nsCOMArray<nsITimer> mTimers;

  nsresult AttachListeners();
  nsresult DetachListeners();

  
  nsresult GetRequestUri(nsIRequest* aReq, nsIURI** uri);
};
