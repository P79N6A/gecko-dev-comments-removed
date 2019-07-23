





































#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsString.h"
#include "nsIWebBrowserChrome.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIWebNavigation.h"
#include "nsIWebBrowser.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBaseWindow.h"
#include "nsIWindowCreator.h"

class WebBrowserChrome : public nsIWebBrowserChrome,
                         public nsIEmbeddingSiteWindow

{
  public:
    WebBrowserChrome();
    virtual ~WebBrowserChrome();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIEMBEDDINGSITEWINDOW

    nsresult CreateBrowser(HWND hWnd, PRInt32 aX, PRInt32 aY,
                                     PRInt32 aCX, PRInt32 aCY,
                                     nsIWebBrowser **aBrowser);           

  protected:
    nativeWindow mNativeWindow;
    PRUint32     mChromeFlags;

    nsCOMPtr<nsIWebBrowser> mWebBrowser;
};

