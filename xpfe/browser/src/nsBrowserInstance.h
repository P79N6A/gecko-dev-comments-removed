



































#ifndef nsBrowserInstance_h___
#define nsBrowserInstance_h___


#include "nsCOMPtr.h"
#include "nsWeakReference.h"


#include "nsIBrowserInstance.h"

#include "nscore.h"
#include "nsISupports.h"

#ifndef MOZ_XUL_APP

#include "nsIContentHandler.h"
#include "nsICmdLineHandler.h"
#endif

class nsIDocShell;
class nsIDOMWindowInternal;
class nsIPref;





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

#ifndef MOZ_XUL_APP
class nsChromeStartupHandler : public nsICmdLineHandler
{
public:
  NS_DECL_NSICMDLINEHANDLER
  NS_DECL_ISUPPORTS
  CMDLINEHANDLER_REGISTERPROC_DECLS
};

class nsBrowserContentHandler : public nsIContentHandler, public nsICmdLineHandler
{
public:
  NS_DECL_NSICONTENTHANDLER
  NS_DECL_NSICMDLINEHANDLER
  NS_DECL_ISUPPORTS
  CMDLINEHANDLER_REGISTERPROC_DECLS

protected:
  PRBool NeedHomepageOverride(nsIPref *aPrefService);
};
#endif

#endif
