







































#ifndef __CWebBrowserChrome__
#define __CWebBrowserChrome__


#include "nsCOMPtr.h"


#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIWebProgressListener.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPrompt.h"
#include "nsIContextMenuListener.h"
#include "nsITooltipListener.h"
#include "nsWeakReference.h"


#include "nsIWebBrowser.h"

class CBrowserWindow;
class CBrowserShell;

class CWebBrowserChrome : public nsIWebBrowserChrome,
                          public nsIWebBrowserChromeFocus,
                          public nsIWebProgressListener,
                          public nsIEmbeddingSiteWindow,
                          public nsIInterfaceRequestor,
                          public nsIContextMenuListener,
                          public nsITooltipListener,
                          public nsSupportsWeakReference
{
friend class CBrowserWindow;

public:
   NS_DECL_ISUPPORTS
   NS_DECL_NSIWEBBROWSERCHROME
   NS_DECL_NSIWEBBROWSERCHROMEFOCUS
   NS_DECL_NSIWEBPROGRESSLISTENER
   NS_DECL_NSIEMBEDDINGSITEWINDOW
   NS_DECL_NSIINTERFACEREQUESTOR
   NS_DECL_NSICONTEXTMENULISTENER
   NS_DECL_NSITOOLTIPLISTENER
  
protected:
   CWebBrowserChrome();
   virtual ~CWebBrowserChrome();

   CBrowserWindow*& BrowserWindow();
   CBrowserShell*& BrowserShell();

protected:
   CBrowserWindow*  mBrowserWindow;
   CBrowserShell*   mBrowserShell;
   
   Boolean mPreviousBalloonState;     
   Boolean mInModalLoop;
   
   nsCOMPtr<nsIPrompt> mPrompter;   
};

#endif 
