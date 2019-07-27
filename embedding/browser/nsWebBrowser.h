





#ifndef nsWebBrowser_h__
#define nsWebBrowser_h__


#include "nsDocShellTreeOwner.h"


#include "nsAutoPtr.h"
#include "nsCOMPtr.h"


#include "nsCWebBrowser.h"
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIScrollable.h"
#include "nsISHistory.h"
#include "nsITextScroll.h"
#include "nsIWidget.h"
#include "nsIWebProgress.h"
#include "nsISecureBrowserUI.h"
#include "nsIWebBrowser.h"
#include "nsIWebNavigation.h"
#include "nsIWebBrowserSetup.h"
#include "nsIWebBrowserPersist.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWebBrowserStream.h"
#include "nsIWindowWatcher.h"
#include "nsIPrintSettings.h"
#include "nsEmbedStream.h"
#include "nsIWidgetListener.h"

#include "nsTArray.h"
#include "nsWeakPtr.h"

class nsWebBrowserInitInfo
{
public:
  
  int32_t                 x;
  int32_t                 y;
  int32_t                 cx;
  int32_t                 cy;
  bool                    visible;
  nsCOMPtr<nsISHistory>   sessionHistory;
  nsString                name;
};

class nsWebBrowserListenerState
{
public:
  bool Equals(nsIWeakReference *aListener, const nsIID& aID) {
    return mWeakPtr.get() == aListener && mID.Equals(aID);
  }

  nsWeakPtr mWeakPtr;
  nsIID mID;
};


#define NS_WEBBROWSER_CID \
{0xcda5863a, 0xaa9c, 0x411e, { 0xbe, 0x49, 0xea, 0x0d, 0x52, 0x5a, 0xb4, 0xb5 }}


class nsWebBrowser final : public nsIWebBrowser,
                           public nsIWebNavigation,
                           public nsIWebBrowserSetup,
                           public nsIDocShellTreeItem,
                           public nsIBaseWindow,
                           public nsIScrollable,
                           public nsITextScroll,
                           public nsIInterfaceRequestor,
                           public nsIWebBrowserPersist,
                           public nsIWebBrowserFocus,
                           public nsIWebProgressListener,
                           public nsIWebBrowserStream,
                           public nsIWidgetListener,
                           public nsSupportsWeakReference
{
  friend class nsDocShellTreeOwner;
public:
  nsWebBrowser();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIBASEWINDOW
  NS_DECL_NSIDOCSHELLTREEITEM
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSISCROLLABLE
  NS_DECL_NSITEXTSCROLL
  NS_DECL_NSIWEBBROWSER
  NS_DECL_NSIWEBNAVIGATION
  NS_DECL_NSIWEBBROWSERSETUP
  NS_DECL_NSIWEBBROWSERPERSIST
  NS_DECL_NSICANCELABLE
  NS_DECL_NSIWEBBROWSERFOCUS
  NS_DECL_NSIWEBBROWSERSTREAM
  NS_DECL_NSIWEBPROGRESSLISTENER

protected:
  virtual ~nsWebBrowser();
  NS_IMETHOD InternalDestroy();

  
  NS_IMETHOD SetDocShell(nsIDocShell* aDocShell);
  NS_IMETHOD EnsureDocShellTreeOwner();
  NS_IMETHOD GetPrimaryContentWindow(nsIDOMWindow **aDomWindow);
  NS_IMETHOD BindListener(nsISupports *aListener, const nsIID& aIID);
  NS_IMETHOD UnBindListener(nsISupports *aListener, const nsIID& aIID);
  NS_IMETHOD EnableGlobalHistory(bool aEnable);

  
  virtual void WindowRaised(nsIWidget* aWidget);
  virtual void WindowLowered(nsIWidget* aWidget);
  virtual bool PaintWindow(nsIWidget* aWidget, nsIntRegion aRegion) override;

protected:
  nsRefPtr<nsDocShellTreeOwner> mDocShellTreeOwner;
  nsCOMPtr<nsIDocShell>      mDocShell;
  nsCOMPtr<nsIInterfaceRequestor> mDocShellAsReq;
  nsCOMPtr<nsIBaseWindow>    mDocShellAsWin;
  nsCOMPtr<nsIWebNavigation> mDocShellAsNav;
  nsCOMPtr<nsIScrollable>    mDocShellAsScrollable;
  nsCOMPtr<nsITextScroll>    mDocShellAsTextScroll;
  nsCOMPtr<nsIWidget>        mInternalWidget;
  nsCOMPtr<nsIWindowWatcher> mWWatch;
  nsAutoPtr<nsWebBrowserInitInfo> mInitInfo;
  uint32_t                   mContentType;
  bool                       mActivating;
  bool                       mShouldEnableHistory;
  bool                       mIsActive;
  nativeWindow               mParentNativeWindow;
  nsIWebProgressListener    *mProgressListener;
  nsCOMPtr<nsIWebProgress>      mWebProgress;

  nsCOMPtr<nsIPrintSettings> mPrintSettings;

  
  nscolor                       mBackgroundColor;

  
  nsCOMPtr<nsIWebBrowserPersist> mPersist;
  uint32_t                       mPersistCurrentState;
  nsresult                       mPersistResult;
  uint32_t                       mPersistFlags;

  
  nsRefPtr<nsEmbedStream>        mStream;

  
  nsIWidget*                            mParentWidget;
  nsAutoPtr<nsTArray<nsWebBrowserListenerState>> mListenerArray;
};

#endif 
