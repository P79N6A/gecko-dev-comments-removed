



































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

  NS_HIDDEN_(PRBool) KeyMatchesAppName(const char *aKeyValue) const;
  NS_HIDDEN_(PRBool) CheckHandlerMatchesAppName(const nsACString& handler) const;

  NS_HIDDEN_(PRBool) GetAppPathFromLauncher();
  PRPackedBool mCheckedThisSession;
  PRPackedBool mUseLocaleFilenames;
  nsCString    mAppPath;
  PRPackedBool mAppIsInPath;
};

#endif 
