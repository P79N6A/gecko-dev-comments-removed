





























#include "prtypes.h"

class nsIWebBrowserChrome;

nsresult CreateBrowserWindow(PRUint32 aChromeFlags,
                             nsIWebBrowserChrome *aParent,
                             nsIWebBrowserChrome **aNewWindow);

void     EnableChromeWindow(nsIWebBrowserChrome *aWindow, PRBool aEnabled);

PRUint32 RunEventLoop(PRBool &aRunCondition);

