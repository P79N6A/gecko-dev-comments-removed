




#ifndef nsgnomeshellservice_h____
#define nsgnomeshellservice_h____

#include "nsIShellService.h"
#include "nsStringAPI.h"
#include "mozilla/Attributes.h"

class nsGNOMEShellService MOZ_FINAL : public nsIShellService
{
public:
  nsGNOMEShellService() : mCheckedThisSession(false), mAppIsInPath(false) { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSISHELLSERVICE

  nsresult Init() NS_HIDDEN;

private:
  ~nsGNOMEShellService() {}

  bool KeyMatchesAppName(const char *aKeyValue) const;
  bool CheckHandlerMatchesAppName(const nsACString& handler) const;

  bool GetAppPathFromLauncher();
  bool mCheckedThisSession;
  bool mUseLocaleFilenames;
  nsCString    mAppPath;
  bool mAppIsInPath;
};

#endif 
