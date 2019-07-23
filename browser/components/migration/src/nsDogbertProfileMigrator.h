




































#ifndef dogbertprofilemigrator___h___
#define dogbertprofilemigrator___h___

#include "nsIBrowserProfileMigrator.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsISupportsArray.h"
#include "nsNetscapeProfileMigratorBase.h"
#include "nsStringAPI.h"

#ifdef XP_MACOSX
#define NEED_TO_FIX_4X_COOKIES 1
#define SECONDS_BETWEEN_1900_AND_1970 2208988800UL
#endif 

class nsIFile;

class nsDogbertProfileMigrator : public nsNetscapeProfileMigratorBase, 
                                 public nsIBrowserProfileMigrator
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_ISUPPORTS

  nsDogbertProfileMigrator();
  virtual ~nsDogbertProfileMigrator();

public:
  static nsresult GetHomepage(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult GetImagePref(void* aTransform, nsIPrefBranch* aBranch);

protected:
  nsresult CopyPreferences(PRBool aReplace);
  nsresult TransformPreferences(const nsAString& aSourcePrefFileName,
                                const nsAString& aTargetPrefFileName);
  
  nsresult CopyCookies(PRBool aReplace);
#ifdef NEED_TO_FIX_4X_COOKIES
  nsresult FixDogbertCookies();
#endif

  nsresult CopyBookmarks(PRBool aReplace);
  nsresult MigrateDogbertBookmarks();

  void     GetSourceProfile(const PRUnichar* aProfile);

private:
  nsCOMPtr<nsISupportsArray> mProfiles;
  nsCOMPtr<nsIObserverService> mObserverService;
};
 
#endif
