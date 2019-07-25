





























#include "nscore.h"

class nsIWebBrowserChrome;

namespace AppCallbacks {
  nsresult CreateBrowserWindow(PRUint32 aChromeFlags,
             nsIWebBrowserChrome *aParent,
             nsIWebBrowserChrome **aNewWindow);

  void     EnableChromeWindow(nsIWebBrowserChrome *aWindow, PRBool aEnabled);

  PRUint32 RunEventLoop(PRBool &aRunCondition);
}
