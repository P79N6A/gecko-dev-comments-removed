





#ifndef nsDocShellTreeOwner_h__
#define nsDocShellTreeOwner_h__


#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsString.h"


#include "nsIBaseWindow.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebBrowserChrome.h"
#include "nsIDOMEventListener.h"
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
#include "nsCommandHandler.h"

namespace mozilla {
namespace dom {
class EventTarget;
} 
} 

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

NS_DEFINE_STATIC_IID_ACCESSOR(nsICDocShellTreeOwner, NS_ICDOCSHELLTREEOWNER_IID)

class nsDocShellTreeOwner final : public nsIDocShellTreeOwner,
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

  nsresult FindItemWithNameAcrossWindows(
    const char16_t* aName,
    nsIDocShellTreeItem* aRequestor, nsIDocShellTreeItem* aOriginalRequestor,
    nsIDocShellTreeItem** aFoundItem);

  void EnsurePrompter();
  void EnsureAuthPrompter();

  void AddToWatcher();
  void RemoveFromWatcher();

  
  
  
  
  
  already_AddRefed<nsIWebBrowserChrome> GetWebBrowserChrome();
  already_AddRefed<nsIEmbeddingSiteWindow> GetOwnerWin();
  already_AddRefed<nsIInterfaceRequestor> GetOwnerRequestor();

protected:
  
  nsWebBrowser* mWebBrowser;
  nsIDocShellTreeOwner* mTreeOwner;
  nsIDocShellTreeItem* mPrimaryContentShell;

  nsIWebBrowserChrome* mWebBrowserChrome;
  nsIEmbeddingSiteWindow* mOwnerWin;
  nsIInterfaceRequestor* mOwnerRequestor;

  nsWeakPtr mWebBrowserChromeWeak; 

  
  
  
  nsRefPtr<ChromeTooltipListener> mChromeTooltipListener;
  nsRefPtr<ChromeContextMenuListener> mChromeContextMenuListener;

  nsCOMPtr<nsIPrompt> mPrompter;
  nsCOMPtr<nsIAuthPrompt> mAuthPrompter;
};





class ChromeTooltipListener final : public nsIDOMEventListener
{
protected:
  virtual ~ChromeTooltipListener();

public:
  NS_DECL_ISUPPORTS

  ChromeTooltipListener(nsWebBrowser* aInBrowser, nsIWebBrowserChrome* aInChrome);

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) override;
  NS_IMETHOD MouseMove(nsIDOMEvent* aMouseEvent);

  
  
  NS_IMETHOD AddChromeListeners();
  NS_IMETHOD RemoveChromeListeners();

private:
  
  enum
  {
    kTooltipAutoHideTime = 5000,    
    kTooltipMouseMoveTolerance = 7  
  };

  NS_IMETHOD AddTooltipListener();
  NS_IMETHOD RemoveTooltipListener();

  NS_IMETHOD ShowTooltip(int32_t aInXCoords, int32_t aInYCoords,
                         const nsAString& aInTipText);
  NS_IMETHOD HideTooltip();

  nsWebBrowser* mWebBrowser;
  nsCOMPtr<mozilla::dom::EventTarget> mEventTarget;
  nsCOMPtr<nsITooltipTextProvider> mTooltipTextProvider;

  
  
  
  
  nsCOMPtr<nsIWebBrowserChrome> mWebBrowserChrome;

  bool mTooltipListenerInstalled;

  nsCOMPtr<nsITimer> mTooltipTimer;
  static void sTooltipCallback(nsITimer* aTimer, void* aListener);

  
  int32_t mMouseClientX;
  int32_t mMouseClientY;

  
  int32_t mMouseScreenX;
  int32_t mMouseScreenY;

  bool mShowingTooltip;
  bool mTooltipShownOnce;

  
  
  
  
  
  
  
  nsCOMPtr<nsIDOMNode> mPossibleTooltipNode;
};




class ChromeContextMenuListener : public nsIDOMEventListener
{
protected:
  virtual ~ChromeContextMenuListener();

public:
  NS_DECL_ISUPPORTS

  ChromeContextMenuListener(nsWebBrowser* aInBrowser,
                            nsIWebBrowserChrome* aInChrome);

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) override;

  
  
  NS_IMETHOD AddChromeListeners();
  NS_IMETHOD RemoveChromeListeners();

private:
  NS_IMETHOD AddContextMenuListener();
  NS_IMETHOD RemoveContextMenuListener();

  bool mContextMenuListenerInstalled;

  nsWebBrowser* mWebBrowser;
  nsCOMPtr<mozilla::dom::EventTarget> mEventTarget;
  nsCOMPtr<nsIWebBrowserChrome> mWebBrowserChrome;
};

#endif
