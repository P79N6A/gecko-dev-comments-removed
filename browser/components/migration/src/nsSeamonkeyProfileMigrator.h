




































#ifndef seamonkeyprofilemigrator___h___
#define seamonkeyprofilemigrator___h___

#include "nsIBrowserProfileMigrator.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsISupportsArray.h"
#include "nsNetscapeProfileMigratorBase.h"
#include "nsStringAPI.h"

class nsIFile;
class nsIPrefBranch;
class nsIPrefService;
class nsVoidArray;

class nsSeamonkeyProfileMigrator : public nsNetscapeProfileMigratorBase, 
                                   public nsIBrowserProfileMigrator
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_ISUPPORTS

  nsSeamonkeyProfileMigrator();
  virtual ~nsSeamonkeyProfileMigrator();

public:
  static nsresult SetImage(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetCookie(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetDownloadManager(void* aTransform, nsIPrefBranch* aBranch);

protected:
  nsresult FillProfileDataFromSeamonkeyRegistry();
  nsresult GetSourceProfile(const PRUnichar* aProfile);

  nsresult CopyPreferences(PRBool aReplace);
  nsresult TransformPreferences(const nsAString& aSourcePrefFileName,
                                const nsAString& aTargetPrefFileName);
  void     ReadFontsBranch(nsIPrefService* aPrefService, nsVoidArray* aPrefs);
  void     WriteFontsBranch(nsIPrefService* aPrefService, nsVoidArray* aPrefs);

  nsresult CopyUserContentSheet();

  nsresult CopyCookies(PRBool aReplace);
  nsresult CopyHistory(PRBool aReplace);
  nsresult CopyPasswords(PRBool aReplace);
  nsresult LocateSignonsFile(char** aResult);
  nsresult CopyBookmarks(PRBool aReplace);
  nsresult CopyOtherData(PRBool aReplace);

private:
  nsCOMPtr<nsISupportsArray> mProfileNames;
  nsCOMPtr<nsISupportsArray> mProfileLocations;
  nsCOMPtr<nsIObserverService> mObserverService;
};
 
#endif
