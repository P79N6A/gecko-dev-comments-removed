





































#include "nsXPITriggerInfo.h"
#include "nsIXPIInstallInfo.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDocShell.h"
#include "nsIURI.h"

class nsXPIInstallInfo : public nsIXPIInstallInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPIINSTALLINFO

  nsXPIInstallInfo(nsIDOMWindowInternal *aOriginatingWindow,
                   nsIURI *aOriginatingURI, nsXPITriggerInfo *aTriggerInfo,
                   PRUint32 aChromeType);

private:
  ~nsXPIInstallInfo();

  nsCOMPtr<nsIDOMWindowInternal> mOriginatingWindow;
  nsCOMPtr<nsIURI> mOriginatingURI;
  nsXPITriggerInfo* mTriggerInfo;
  PRUint32 mChromeType;
};
