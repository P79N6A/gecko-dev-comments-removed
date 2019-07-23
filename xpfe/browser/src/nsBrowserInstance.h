



































#ifndef nsBrowserInstance_h___
#define nsBrowserInstance_h___


#include "nsCOMPtr.h"
#include "nsWeakReference.h"


#include "nsIBrowserInstance.h"

#include "nscore.h"
#include "nsISupports.h"

class nsIDocShell;
class nsIDOMWindowInternal;
class nsIPrefBranch;





class nsBrowserInstance : public nsIBrowserInstance,
                          public nsSupportsWeakReference 
{
  public:
    friend class PageCycler;

    nsBrowserInstance();
    virtual ~nsBrowserInstance();
    
    NS_DEFINE_STATIC_CID_ACCESSOR( NS_BROWSERINSTANCE_CID )
                 
    NS_DECL_ISUPPORTS

    NS_DECL_NSIBROWSERINSTANCE


  protected:

    nsresult LoadUrl(const PRUnichar * urlToLoad);
    nsresult GetContentAreaDocShell(nsIDocShell** outDocShell);
    
    void ReinitializeContentVariables();
    
    PRBool              mIsClosed;
    static PRBool       sCmdLineURLUsed;

    nsWeakPtr          mContentAreaDocShellWeak;

    nsIDOMWindowInternal*       mDOMWindow;                         

#ifdef DEBUG_warren
    PRIntervalTime      mLoadStartTime;
#endif
};

#endif 
