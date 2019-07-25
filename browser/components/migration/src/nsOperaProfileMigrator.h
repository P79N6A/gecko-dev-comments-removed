



































 
#ifndef operaprofilemigrator___h___
#define operaprofilemigrator___h___

#include "nsCOMPtr.h"
#include "nsIBinaryInputStream.h"
#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsISupportsArray.h"
#include "nsStringAPI.h"
#include "nsTArray.h"
#include "nsINavHistoryService.h"
#include "nsIStringBundle.h"

class nsICookieManager2;
class nsILineInputStream;
class nsILocalFile;
class nsINIParser;
class nsIPermissionManager;
class nsIPrefBranch;
class nsINavBookmarksService;
class nsIRDFResource;

class nsOperaProfileMigrator : public nsIBrowserProfileMigrator,
                               public nsINavHistoryBatchCallback
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_NSINAVHISTORYBATCHCALLBACK
  NS_DECL_ISUPPORTS

  nsOperaProfileMigrator();
  virtual ~nsOperaProfileMigrator();

public:

  typedef enum { STRING, INT, BOOL, COLOR } PrefType;

  typedef nsresult(*prefConverter)(void*, nsIPrefBranch*);

  struct PrefTransform {
    const char*   sectionName;
    const char*   keyName;
    PrefType      type;
    const char*   targetPrefName;
    prefConverter prefSetterFunc;
    PRBool        prefHasValue;
    union {
      PRInt32     intValue;
      PRBool      boolValue;
      char*       stringValue;
    };
  };

  static nsresult SetFile(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetCookieBehavior(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetCookieLifetime(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetImageBehavior(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetBool(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetWString(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetInt(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetString(void* aTransform, nsIPrefBranch* aBranch);

protected:
  nsresult CopyPreferences(PRBool aReplace);
  nsresult ParseColor(nsINIParser &aParser, const char* aSectionName,
                      char** aResult);
  nsresult CopyUserContentSheet(nsINIParser &aParser);
  nsresult CopyProxySettings(nsINIParser &aParser, nsIPrefBranch* aBranch);
  nsresult GetInteger(nsINIParser &aParser, const char* aSectionName, 
                      const char* aKeyName, PRInt32* aResult);

  nsresult CopyCookies(PRBool aReplace);
  







  nsresult CopyHistory(PRBool aReplace);
  nsresult CopyHistoryBatched(PRBool aReplace);
  








  nsresult CopyBookmarks(PRBool aReplace);
  nsresult CopyBookmarksBatched(PRBool aReplace);
  void     ClearToolbarFolder(nsINavBookmarksService * aBookmarksService,
                              PRInt64 aToolbarFolder);
  nsresult ParseBookmarksFolder(nsILineInputStream* aStream, 
                                PRInt64 aFolder,
                                PRInt64 aToolbar, 
                                nsINavBookmarksService* aBMS);
#if defined(XP_WIN) || (defined(XP_UNIX) && !defined(XP_MACOSX))
  nsresult CopySmartKeywords(nsINavBookmarksService* aBMS, 
                             nsIStringBundle* aBundle, 
                             PRInt64 aParentFolder);
#endif 

  void     GetOperaProfile(const PRUnichar* aProfile, nsILocalFile** aFile);

private:
  nsCOMPtr<nsILocalFile> mOperaProfile;
  nsCOMPtr<nsISupportsArray> mProfiles;
  nsCOMPtr<nsIObserverService> mObserverService;
};

class nsOperaCookieMigrator
{
public:
  nsOperaCookieMigrator(nsIInputStream* aSourceStream);
  virtual ~nsOperaCookieMigrator();

  nsresult Migrate();

  typedef enum { BEGIN_DOMAIN_SEGMENT         = 0x01,
                 DOMAIN_COMPONENT             = 0x1E,
                 END_DOMAIN_SEGMENT           = 0x84 | 0x80, 
                 
                 BEGIN_PATH_SEGMENT           = 0x02,
                 PATH_COMPONENT               = 0x1D,
                 END_PATH_SEGMENT             = 0x05 | 0x80, 
                 
                 FILTERING_INFO               = 0x1F,
                 PATH_HANDLING_INFO           = 0x21,
                 THIRD_PARTY_HANDLING_INFO    = 0x25,

                 BEGIN_COOKIE_SEGMENT         = 0x03,
                 COOKIE_ID                    = 0x10,
                 COOKIE_DATA                  = 0x11,
                 COOKIE_EXPIRY                = 0x12,
                 COOKIE_LASTUSED              = 0x13,
                 COOKIE_COMMENT               = 0x14,
                 COOKIE_COMMENT_URL           = 0x15,
                 COOKIE_V1_DOMAIN             = 0x16,
                 COOKIE_V1_PATH               = 0x17,
                 COOKIE_V1_PORT_LIMITATIONS   = 0x18,
                 COOKIE_SECURE                = 0x19 | 0x80, 
                 COOKIE_VERSION               = 0x1A,
                 COOKIE_OTHERFLAG_1           = 0x1B | 0x80,
                 COOKIE_OTHERFLAG_2           = 0x1C | 0x80,
                 COOKIE_OTHERFLAG_3           = 0x20 | 0x80,
                 COOKIE_OTHERFLAG_4           = 0x22 | 0x80,
                 COOKIE_OTHERFLAG_5           = 0x23 | 0x80,
                 COOKIE_OTHERFLAG_6           = 0x24 | 0x80
  } TAG;

protected:
  nsOperaCookieMigrator() { }

  nsresult ReadHeader();

  void     SynthesizePath(char** aResult);
  void     SynthesizeDomain(char** aResult);
  nsresult AddCookieOverride(nsIPermissionManager* aManager);
  nsresult AddCookie(nsICookieManager2* aManager);

private:
  nsCOMPtr<nsIBinaryInputStream> mStream;

  nsTArray<char*> mDomainStack;
  nsTArray<char*> mPathStack;

  struct Cookie {
    nsCString id;
    nsCString data;
    PRInt32 expiryTime;
    PRBool isSecure;
  };

  PRUint32 mAppVersion;
  PRUint32 mFileVersion;
  PRUint16 mTagTypeLength;
  PRUint16 mPayloadTypeLength;
  PRBool   mCookieOpen;
  Cookie   mCurrCookie;
  PRUint8  mCurrHandlingInfo;
};

#endif

