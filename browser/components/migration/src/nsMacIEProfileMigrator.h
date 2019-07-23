




































#ifndef macieprofilemigrator___h___
#define maciebprofilemigrator___h___

#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsISupportsArray.h"
#include "nsStringAPI.h"

class nsMacIEProfileMigrator : public nsIBrowserProfileMigrator
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_ISUPPORTS

  nsMacIEProfileMigrator();
  virtual ~nsMacIEProfileMigrator();

protected:
  nsresult CopyBookmarks(PRBool aReplace);

protected:
  nsCOMPtr<nsILocalFile> mSourceProfile;
  nsCOMPtr<nsIFile> mTargetProfile;

private:
  nsCOMPtr<nsIObserverService> mObserverService;
};
 
#endif
