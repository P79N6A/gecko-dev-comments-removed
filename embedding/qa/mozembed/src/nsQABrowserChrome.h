



































#ifndef __nsQABrowserChrome__
#define __nsQABrowserChrome__

#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsIWebBrowserChrome.h"
#include "nsIQABrowserChrome.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIWebNavigation.h"
#include "nsIWebBrowser.h"
#include "nsWeakReference.h"
#include "nsIWeakReference.h"
#include "nsIQABrowserView.h"
#include "nsIQABrowserUIGlue.h"
#include "nsIWebProgressListener.h"
#include "nsIWebBrowserChromeFocus.h"













class nsQABrowserChrome   : public nsIWebBrowserChrome,
                            public nsIEmbeddingSiteWindow,
                            public nsIInterfaceRequestor,
                            public nsIQABrowserChrome,
                            public nsSupportsWeakReference,
                            public nsIWebProgressListener,
                            public nsIWebBrowserChromeFocus
{
public:
    nsQABrowserChrome();
    virtual ~nsQABrowserChrome();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIQABROWSERCHROME
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSIWEBPROGRESSLISTENER

protected:

    void ContentFinishedLoading();

    nativeWindow mNativeWindow;
    PRUint32     mChromeFlags;
    PRBool       mContinueModalLoop;
    PRBool       mSizeSet;

    nsCOMPtr<nsIWebBrowser> mWebBrowser;
    nsCOMPtr<nsIQABrowserUIGlue> mBrowserUIGlue;    
    nsCOMPtr<nsIWebBrowserChrome> mDependentParent; 
    
};

#endif 
