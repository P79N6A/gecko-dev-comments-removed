



































#ifndef nsgnomeshellservice_h____
#define nsgnomeshellservice_h____

#include "nsIShellService.h"
#include "nsStringAPI.h"

class nsGNOMEShellService : public nsIShellService
{
public:
  nsGNOMEShellService() : mCheckedThisSession(PR_FALSE), mAppIsInPath(PR_FALSE) { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE

  nsresult Init() NS_HIDDEN;

private:
  ~nsGNOMEShellService() {}

  NS_HIDDEN_(bool) KeyMatchesAppName(const char *aKeyValue) const;
  NS_HIDDEN_(bool) CheckHandlerMatchesAppName(const nsACString& handler) const;

  NS_HIDDEN_(bool) GetAppPathFromLauncher();
  bool mCheckedThisSession;
  bool mUseLocaleFilenames;
  nsCString    mAppPath;
  bool mAppIsInPath;
};

#endif 
