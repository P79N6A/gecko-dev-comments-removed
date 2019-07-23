





































#ifndef nswindowsshellservice_h____
#define nswindowsshellservice_h____

#include "nscore.h"
#include "nsStringAPI.h"
#include "nsIWindowsShellService.h"

#include <windows.h>
#include <ole2.h>

#ifndef WINCE
class nsWindowsShellService : public nsIWindowsShellService
#else
class nsWindowsShellService : public nsIShellService
#endif
{
public:
  nsWindowsShellService() : mCheckedThisSession(PR_FALSE) {}; 
  virtual ~nsWindowsShellService() {};

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE
#ifndef WINCE
  NS_DECL_NSIWINDOWSSHELLSERVICE
#endif

protected:
#ifndef WINCE
  PRBool    IsDefaultBrowserVista(PRBool* aIsDefaultBrowser);

  PRBool    GetMailAccountKey(HKEY* aResult);
#else
  void      SetRegKey(const nsString& aKeyName,
                      const nsString& aValueName,
                      const nsString& aValue);
#endif

private:
  PRBool    mCheckedThisSession;
};

#endif 
