













































#ifndef nsQABrowserUIGlue_h_
#define nsQABrowserUIGlue_h_

#include "nsISupports.h"
#include "nsIQABrowserUIGlue.h"
#include "nsIQABrowserChrome.h"
#include "nsIQABrowserView.h"


extern nsresult NS_NewQABrowserUIGlueFactory(nsISupports *, const nsIID& iid, void ** result);

class nsQABrowserUIGlue : public nsIQABrowserUIGlue
{
public:
  nsQABrowserUIGlue();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIQABROWSERUIGLUE

protected:
  virtual ~nsQABrowserUIGlue();
  nativeWindow CreateNativeWindow(nsIWebBrowserChrome  * aChrome);

private:
  PRBool mAllowNewWindows;
  nsCOMPtr<nsIQABrowserView> mBrowserView;
};


#endif 