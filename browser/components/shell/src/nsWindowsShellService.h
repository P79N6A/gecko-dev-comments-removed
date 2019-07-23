





































#ifndef nswindowsshellservice_h____
#define nswindowsshellservice_h____

#include "nscore.h"
#include "nsIWindowsShellService.h"

#include <windows.h>
#include <ole2.h>

class nsWindowsShellService : public nsIWindowsShellService
{
public:
  nsWindowsShellService() : mCheckedThisSession(PR_FALSE) {}; 
  virtual ~nsWindowsShellService() {};

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE
  NS_DECL_NSIWINDOWSSHELLSERVICE

protected:
  PRBool    IsDefaultBrowserVista(PRBool aStartupCheck, PRBool* aIsDefaultBrowser);
  PRBool    SetDefaultBrowserVista();

  PRBool    GetMailAccountKey(HKEY* aResult);
  void      SetRegKey(const char* aKeyName, const char* aValueName, 
                      const char* aValue, PRBool aHKLMOnly);
  DWORD     DeleteRegKey(HKEY baseKey, const char *keyName);
  DWORD     DeleteRegKeyDefaultValue(HKEY baseKey, const char *keyName);

private:
  PRBool    mCheckedThisSession;
};

#endif 
