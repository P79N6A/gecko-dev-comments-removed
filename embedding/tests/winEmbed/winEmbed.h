





























#include "nscore.h"

class nsIWebBrowserChrome;

namespace AppCallbacks {
  nsresult CreateBrowserWindow(uint32_t aChromeFlags,
             nsIWebBrowserChrome *aParent,
             nsIWebBrowserChrome **aNewWindow);

  void     EnableChromeWindow(nsIWebBrowserChrome *aWindow, bool aEnabled);

  uint32_t RunEventLoop(bool &aRunCondition);
}
