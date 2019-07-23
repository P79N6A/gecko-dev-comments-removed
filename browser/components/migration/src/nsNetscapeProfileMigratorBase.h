




































#ifndef netscapeprofilemigratorbase___h___
#define netscapeprofilemigratorbase___h___

#include "nsILocalFile.h"
#include "nsIStringBundle.h"
#include "nsISupportsArray.h"
#include "nsStringAPI.h"

class nsIFile;
class nsIPrefBranch;

class nsNetscapeProfileMigratorBase
{
public:
  nsNetscapeProfileMigratorBase();
  virtual ~nsNetscapeProfileMigratorBase() { };

public:
  typedef nsresult(*prefConverter)(void*, nsIPrefBranch*);

  struct PrefTransform {
    char*         sourcePrefName;
    char*         targetPrefName;
    prefConverter prefGetterFunc;
    prefConverter prefSetterFunc;
    PRBool        prefHasValue;
    union {
      PRInt32     intValue;
      PRBool      boolValue;
      char*       stringValue;
    };
  };

  static nsresult GetString(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetString(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult GetWString(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetWString(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetWStringFromASCII(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult GetBool(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetBool(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult GetInt(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetInt(void* aTransform, nsIPrefBranch* aBranch);

protected:
  nsresult GetProfileDataFromRegistry(nsILocalFile* aRegistryFile,
                                      nsISupportsArray* aProfileNames,
                                      nsISupportsArray* aProfileLocations);

  nsresult CopyFile(const nsAString& aSourceFileName, const nsAString& aTargetFileName);

  nsresult ImportNetscapeBookmarks(const nsAString& aBookmarksFileName,
                                   const PRUnichar* aImportSourceNameKey);

  nsresult ImportNetscapeCookies(nsIFile* aCookiesFile);

  nsresult GetSignonFileName(PRBool aReplace, char** aFileName);
  nsresult LocateSignonsFile(char** aResult);

protected:
  nsCOMPtr<nsILocalFile> mSourceProfile;
  nsCOMPtr<nsIFile> mTargetProfile;
};
 
#endif
