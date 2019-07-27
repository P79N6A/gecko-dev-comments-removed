




#ifndef nswindowsshellservice_h____
#define nswindowsshellservice_h____

#include "nscore.h"
#include "nsStringAPI.h"
#include "nsIWindowsShellService.h"
#include "nsITimer.h"

#include <windows.h>
#include <ole2.h>

class nsWindowsShellService : public nsIWindowsShellService
{
  virtual ~nsWindowsShellService();

public:
  nsWindowsShellService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE
  NS_DECL_NSIWINDOWSSHELLSERVICE

protected:
  bool IsDefaultBrowserVista(bool aCheckAllTypes, bool* aIsDefaultBrowser);
  nsresult LaunchControlPanelDefaultPrograms();
  nsresult LaunchModernSettingsDialogDefaultApps();
  nsresult InvokeHTTPOpenAsVerb();
  nsresult LaunchHTTPHandlerPane();

private:
  bool      mCheckedThisSession;
};

#endif 
