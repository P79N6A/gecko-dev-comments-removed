





























#include "nscore.h"

class nsIWebBrowserChrome;

namespace AppCallbacks {
  nsresult CreateBrowserWindow(PRUint32 aChromeFlags,
             nsIWebBrowserChrome *aParent,
             nsIWebBrowserChrome **aNewWindow);

  void     EnableChromeWindow(nsIWebBrowserChrome *aWindow, bool aEnabled);

  PRUint32 RunEventLoop(bool &aRunCondition);
}
