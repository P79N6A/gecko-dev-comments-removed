













































#ifndef nsQABrowserView_h_
#define nsQABrowserView_h_

#include "nsCOMPtr.h"
#include "nsISupports.h"
#include "nsIInterfaceRequestor.h"
#include "nsIQABrowserView.h"
#include "nsQABrowserCID.h"
#include "nsIQABrowserChrome.h"

#include "nsIWebNavigation.h"
#include "nsIBaseWindow.h"
#include "nsIWebBrowser.h"



class nsQABrowserView : public nsIInterfaceRequestor,
                        public nsIQABrowserView
{
public:
  nsQABrowserView();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIQABROWSERVIEW

protected:
  virtual ~nsQABrowserView();

private:
  nsCOMPtr<nsIWebBrowser>  mWebBrowser;
  nsCOMPtr<nsIWebNavigation> mWebNav;
  nsCOMPtr<nsIBaseWindow> mBaseWindow;
};


#endif 