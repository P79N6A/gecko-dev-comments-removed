






































#include "nsIWebProgressListener.h"
#include "nsIBrowserHistory.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsWeakReference.h"
#include "nsIGenericFactory.h"

class nsHistoryLoadListener : public nsIWebProgressListener,
                              public nsSupportsWeakReference
{
 public:
    nsHistoryLoadListener(nsIBrowserHistory *);
    virtual ~nsHistoryLoadListener();

    nsresult Init();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER
        
 protected:
    nsCOMPtr<nsIBrowserHistory> mHistory;

};

