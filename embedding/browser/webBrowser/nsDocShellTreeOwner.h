






































#ifndef nsDocShellTreeOwner_h__
#define nsDocShellTreeOwner_h__


#include "nsCOMPtr.h"
#include "nsString.h"


#include "nsIBaseWindow.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebBrowserChrome.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMDocument.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsITimer.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsITooltipListener.h"
#include "nsITooltipTextProvider.h"
#include "nsCTooltipTextProvider.h"
#include "nsIDroppedLinkHandler.h"
#include "nsIDOMEventTarget.h"
#include "nsCommandHandler.h"

class nsWebBrowser;
class ChromeTooltipListener;
class ChromeContextMenuListener;


#define NS_ICDOCSHELLTREEOWNER_IID \
{ 0x6d10c180, 0x6888, 0x11d4, { 0x95, 0x2b, 0x0, 0x20, 0x18, 0x3b, 0xf1, 0x81 } }







class nsICDocShellTreeOwner : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICDOCSHELLTREEOWNER_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICDocShellTreeOwner,
                              NS_ICDOCSHELLTREEOWNER_IID)

class nsDocShellTreeOwner : public nsIDocShellTreeOwner,
                            public nsIBaseWindow,
                            public nsIInterfaceRequestor,
                            public nsIWebProgressListener,
                            public nsIDOMEventListener,
                            public nsICDocShellTreeOwner,
                            public nsSupportsWeakReference
{
friend class nsWebBrowser;
friend class nsCommandHandler;

public:
    NS_DECL_ISUPPORTS

    NS_DECL_NSIBASEWINDOW
    NS_DECL_NSIDOCSHELLTREEOWNER
    NS_DECL_NSIDOMEVENTLISTENER
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIWEBPROGRESSLISTENER

protected:
    nsDocShellTreeOwner();
    virtual ~nsDocShellTreeOwner();

    void WebBrowser(nsWebBrowser* aWebBrowser);
    
    nsWebBrowser* WebBrowser();
    NS_IMETHOD SetTreeOwner(nsIDocShellTreeOwner* aTreeOwner);
    NS_IMETHOD SetWebBrowserChrome(nsIWebBrowserChrome* aWebBrowserChrome);

    NS_IMETHOD AddChromeListeners();
    NS_IMETHOD RemoveChromeListeners();

    nsresult   FindItemWithNameAcrossWindows(const PRUnichar* aName,
                 nsIDocShellTreeItem* aRequestor,
                 nsIDocShellTreeItem* aOriginalRequestor,
                 nsIDocShellTreeItem **aFoundItem);

    void       EnsurePrompter();
    void       EnsureAuthPrompter();

    void AddToWatcher();
    void RemoveFromWatcher();

    
    
    
    
    
    already_AddRefed<nsIWebBrowserChrome>     GetWebBrowserChrome();
    already_AddRefed<nsIEmbeddingSiteWindow>  GetOwnerWin();
    already_AddRefed<nsIInterfaceRequestor>   GetOwnerRequestor();

protected:

   
   nsWebBrowser*           mWebBrowser;
   nsIDocShellTreeOwner*   mTreeOwner;
   nsIDocShellTreeItem*    mPrimaryContentShell; 

   nsIWebBrowserChrome*    mWebBrowserChrome;
   nsIEmbeddingSiteWindow* mOwnerWin;
   nsIInterfaceRequestor*  mOwnerRequestor;

   nsWeakPtr               mWebBrowserChromeWeak;   

    
    
    
   ChromeTooltipListener*         mChromeTooltipListener;
   ChromeContextMenuListener*     mChromeContextMenuListener;

   nsCOMPtr<nsIPrompt>     mPrompter;
   nsCOMPtr<nsIAuthPrompt> mAuthPrompter;
};










class ChromeTooltipListener : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  
  ChromeTooltipListener ( nsWebBrowser* inBrowser, nsIWebBrowserChrome* inChrome ) ;
  virtual ~ChromeTooltipListener ( ) ;

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);
  NS_IMETHOD MouseMove(nsIDOMEvent* aMouseEvent);

    
    
  NS_IMETHOD AddChromeListeners();
  NS_IMETHOD RemoveChromeListeners();

private:

    
  enum {
    kTooltipAutoHideTime = 5000        
  };

  NS_IMETHOD AddTooltipListener();
  NS_IMETHOD RemoveTooltipListener();

  NS_IMETHOD ShowTooltip ( PRInt32 inXCoords, PRInt32 inYCoords, const nsAString & inTipText ) ;
  NS_IMETHOD HideTooltip ( ) ;

  nsWebBrowser* mWebBrowser;
  nsCOMPtr<nsIDOMEventTarget> mEventTarget;
  nsCOMPtr<nsITooltipTextProvider> mTooltipTextProvider;
  
    
    
    
    
  nsCOMPtr<nsIWebBrowserChrome> mWebBrowserChrome;

  bool mTooltipListenerInstalled;

  nsCOMPtr<nsITimer> mTooltipTimer;
  static void sTooltipCallback ( nsITimer* aTimer, void* aListener ) ;
  PRInt32 mMouseClientX, mMouseClientY;       
  PRInt32 mMouseScreenX, mMouseScreenY;       
  bool mShowingTooltip;

    
  nsCOMPtr<nsITimer> mAutoHideTimer;
  static void sAutoHideCallback ( nsITimer* aTimer, void* aListener ) ;
  void CreateAutoHideTimer ( ) ;

    
    
    
    
    
    
    
  nsCOMPtr<nsIDOMNode> mPossibleTooltipNode;

}; 










class ChromeContextMenuListener : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  
  ChromeContextMenuListener ( nsWebBrowser* inBrowser, nsIWebBrowserChrome* inChrome ) ;
  virtual ~ChromeContextMenuListener ( ) ;

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  
  NS_IMETHOD AddChromeListeners();
  NS_IMETHOD RemoveChromeListeners();

private:

  NS_IMETHOD AddContextMenuListener();
  NS_IMETHOD RemoveContextMenuListener();

  bool mContextMenuListenerInstalled;

  nsWebBrowser* mWebBrowser;
  nsCOMPtr<nsIDOMEventTarget> mEventTarget;
  nsCOMPtr<nsIWebBrowserChrome> mWebBrowserChrome;

}; 



#endif












