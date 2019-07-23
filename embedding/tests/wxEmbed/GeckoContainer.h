





























#ifndef __WebBrowserChrome__
#define __WebBrowserChrome__

#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebBrowserChromeFocus.h"

#include "nsIContentViewer.h"
#include "nsIContentViewerFile.h"
#include "nsIBaseWindow.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsIWebNavigation.h"
#include "nsIWebProgressListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebBrowser.h"
#include "nsIURIContentListener.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIContextMenuListener.h"
#include "nsIContextMenuListener2.h"
#include "nsITooltipListener.h"

class GeckoContainerUI;





#define NS_IGECKOCONTAINER_IID \
    { 0xbf47a2ec, 0x9be0, 0x4f18, { 0x9a, 0xf0, 0x8e, 0x1c, 0x89, 0x2a, 0xa3, 0x1d } }

class NS_NO_VTABLE nsIGeckoContainer : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IGECKOCONTAINER_IID)

    NS_IMETHOD GetRole(nsACString &aRole) = 0;
    NS_IMETHOD GetContainerUI(GeckoContainerUI **pUI) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIGeckoContainer, NS_IGECKOCONTAINER_IID)

#define NS_DECL_NSIGECKOCONTAINER \
    NS_IMETHOD GetRole(nsACString &aRole); \
    NS_IMETHOD GetContainerUI(GeckoContainerUI **pUI);

class GeckoContainer   :
    public nsIWebBrowserChrome,
    public nsIWebBrowserChromeFocus,
    public nsIWebProgressListener,
    public nsIEmbeddingSiteWindow2,
    public nsIInterfaceRequestor,
    public nsIObserver,
    public nsIContextMenuListener2,
    public nsITooltipListener,
    public nsIURIContentListener,
    public nsIGeckoContainer,
    public nsSupportsWeakReference
{
public:
    
    
    
    GeckoContainer(GeckoContainerUI *pUI, const char *aRole = NULL);
    virtual ~GeckoContainer();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIEMBEDDINGSITEWINDOW2
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIOBSERVER
    NS_DECL_NSICONTEXTMENULISTENER2
    NS_DECL_NSITOOLTIPLISTENER
    NS_DECL_NSIURICONTENTLISTENER
    NS_DECL_NSIGECKOCONTAINER

    nsresult CreateBrowser(PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY, nativeWindow aParent,
                           nsIWebBrowser **aBrowser);

    void     SetParent(nsIWebBrowserChrome *aParent)
               { mDependentParent = aParent; }
   
protected:
    nsresult SendHistoryStatusMessage(nsIURI * aURI, char * operation, PRInt32 info1=0, PRUint32 info2=0);

    void ContentFinishedLoading();

    GeckoContainerUI *mUI;
    nativeWindow mNativeWindow;
    PRUint32     mChromeFlags;
    PRBool       mContinueModalLoop;
    PRBool       mSizeSet;
    PRBool       mIsChromeContainer;
    PRBool       mIsURIContentListener;

    nsCString    mRole;
    nsCOMPtr<nsIWebBrowser> mWebBrowser;
    nsCOMPtr<nsIWebBrowserChrome> mDependentParent; 
};


class GeckoContainerUI
{
protected:
    PRBool mBusy;

public:
    GeckoContainerUI() :
        mBusy(PR_FALSE)
    {
    }
    
    virtual nsresult CreateBrowserWindow(PRUint32 aChromeFlags,
         nsIWebBrowserChrome *aParent, nsIWebBrowserChrome **aNewWindow);
    
    virtual void Destroy();
    
    virtual void Destroyed();
    
    virtual void SetFocus();
    
    virtual void KillFocus();
    
    virtual void UpdateStatusBarText(const PRUnichar* aStatusText);
    
    virtual void UpdateCurrentURI();
    
    virtual void UpdateBusyState(PRBool aBusy);
    
    virtual void UpdateProgress(PRInt32 aCurrent, PRInt32 aMax);
    virtual void GetResourceStringById(PRInt32 aID, char ** aReturn);
    
    virtual void ShowContextMenu(PRUint32 aContextFlags, nsIContextMenuInfo *aContextMenuInfo);
    
    virtual void ShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords, const PRUnichar *aTipText);
    
    virtual void HideTooltip();
    
    virtual void ShowWindow(PRBool aShow);
    
    virtual void SizeTo(PRInt32 aWidth, PRInt32 aHeight);
    virtual void EnableChromeWindow(PRBool aEnabled);
    virtual PRUint32 RunEventLoop(PRBool &aRunCondition);
};

#endif 
