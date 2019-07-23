




































#include "prtypes.h"

class nsIWebBrowserChrome;

namespace AppCallbacks {
  void     EnableChromeWindow(nsIWebBrowserChrome *aWindow, PRBool aEnabled);

  PRUint32 RunEventLoop(PRBool &aRunCondition);
}
