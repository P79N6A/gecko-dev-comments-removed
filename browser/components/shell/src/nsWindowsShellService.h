





































#ifndef nswindowsshellservice_h____
#define nswindowsshellservice_h____

#include "nscore.h"
#include "nsStringAPI.h"
#include "nsIWindowsShellService.h"

#include <windows.h>
#include <ole2.h>

class nsWindowsShellService : public nsIWindowsShellService
{
public:
  nsWindowsShellService() : mCheckedThisSession(false) {}; 
  virtual ~nsWindowsShellService() {};

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE
  NS_DECL_NSIWINDOWSSHELLSERVICE

protected:
  bool      IsDefaultBrowserVista(bool* aIsDefaultBrowser);

private:
  bool      mCheckedThisSession;
};

#endif 
