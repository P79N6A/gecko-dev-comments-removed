




























#ifndef __WebBrowserChrome__
#define __WebBrowserChrome__



#include <windows.h>



#include "nsStringAPI.h"

#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserChromeFocus.h"

#include "nsIContextMenuListener.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsISHistoryListener.h"
#include "nsITooltipListener.h"
#include "nsIWebProgressListener.h"
#include "nsIWebBrowser.h"


#include "nsCOMPtr.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsWeakReference.h"

class WebBrowserChromeUI
{
public:
    static HWND CreateNativeWindow(nsIWebBrowserChrome* chrome);
    static void Destroy(nsIWebBrowserChrome* chrome);
    static void Destroyed(nsIWebBrowserChrome* chrome);
    static void SetFocus(nsIWebBrowserChrome *chrome);
    static void UpdateStatusBarText(nsIWebBrowserChrome *aChrome, const PRUnichar* aStatusText);
    static void UpdateCurrentURI(nsIWebBrowserChrome *aChrome);
    static void UpdateBusyState(nsIWebBrowserChrome *aChrome, bool aBusy);
    static void UpdateProgress(nsIWebBrowserChrome *aChrome, int32_t aCurrent, int32_t aMax);
    static void GetResourceStringById(int32_t aID, char ** aReturn);
    static void ShowContextMenu(nsIWebBrowserChrome *aChrome, uint32_t aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode);
    static void ShowTooltip(nsIWebBrowserChrome *aChrome, int32_t aXCoords, int32_t aYCoords, const PRUnichar *aTipText);
    static void HideTooltip(nsIWebBrowserChrome *aChrome);
    static void ShowWindow(nsIWebBrowserChrome *aChrome, bool aShow);
    static void SizeTo(nsIWebBrowserChrome *aChrome, int32_t aWidth, int32_t aHeight);
};

class WebBrowserChrome   : public nsIWebBrowserChrome,
                           public nsIWebBrowserChromeFocus,
                           public nsIWebProgressListener,
                           public nsIEmbeddingSiteWindow,
                           public nsIInterfaceRequestor,
                           public nsISHistoryListener,
                           public nsIObserver,
                           public nsIContextMenuListener,
                           public nsITooltipListener,
                           public nsSupportsWeakReference

{
public:
    WebBrowserChrome();
    virtual ~WebBrowserChrome();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSISHISTORYLISTENER
    NS_DECL_NSIOBSERVER
    NS_DECL_NSICONTEXTMENULISTENER
    NS_DECL_NSITOOLTIPLISTENER

    nsresult CreateBrowser(int32_t aX, int32_t aY, int32_t aCX, int32_t aCY,
                           nsIWebBrowser **aBrowser);

    void     SetParent(nsIWebBrowserChrome *aParent)
               { mDependentParent = aParent; }
   
protected:
    nsresult SendHistoryStatusMessage(nsIURI * aURI, char * operation, int32_t info1=0, uint32_t info2=0);

    void ContentFinishedLoading();

    HWND         mNativeWindow;
    uint32_t     mChromeFlags;
    bool         mContinueModalLoop;
    bool         mSizeSet;

    nsCOMPtr<nsIWebBrowser> mWebBrowser;
    nsCOMPtr<nsIWebBrowserChrome> mDependentParent; 
};

#endif 
