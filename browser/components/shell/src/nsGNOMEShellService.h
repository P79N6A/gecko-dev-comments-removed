



































#ifndef nsgnomeshellservice_h____
#define nsgnomeshellservice_h____

#include "nsIShellService.h"
#include "nsStringAPI.h"

class nsGNOMEShellService : public nsIShellService
{
public:
  nsGNOMEShellService() : mCheckedThisSession(PR_FALSE) { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE

  nsresult Init() NS_HIDDEN;

private:
  ~nsGNOMEShellService() {}

  NS_HIDDEN_(PRBool) KeyMatchesAppName(const char *aKeyValue) const;

  PRPackedBool mCheckedThisSession;
  PRPackedBool mUseLocaleFilenames;
  nsCString    mAppPath;
};

#endif 
