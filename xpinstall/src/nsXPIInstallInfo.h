





































#include "nsXPITriggerInfo.h"
#include "nsIXPIInstallInfo.h"
#include "nsIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIURI.h"

class nsXPIInstallInfo : public nsIXPIInstallInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPIINSTALLINFO

  nsXPIInstallInfo(nsIDOMWindow *aOriginatingWindow,
                   nsIURI *aOriginatingURI, nsXPITriggerInfo *aTriggerInfo,
                   PRUint32 aChromeType);

private:
  ~nsXPIInstallInfo();

  nsCOMPtr<nsIDOMWindow> mOriginatingWindow;
  nsCOMPtr<nsIURI> mOriginatingURI;
  nsXPITriggerInfo* mTriggerInfo;
  PRUint32 mChromeType;
};
