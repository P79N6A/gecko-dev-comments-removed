





































#ifndef safariprofilemigrator___h___
#define safariprofilemigrator___h___

#include "nsIBrowserProfileMigrator.h"
#include "nsIObserverService.h"
#include "nsISupportsArray.h"
#include "nsStringAPI.h"
#include "nsINavHistoryService.h"

#include <CoreFoundation/CoreFoundation.h>

class nsIRDFDataSource;

class nsSafariProfileMigrator : public nsIBrowserProfileMigrator,
                                public nsINavHistoryBatchCallback
{
public:
  NS_DECL_NSIBROWSERPROFILEMIGRATOR
  NS_DECL_NSINAVHISTORYBATCHCALLBACK
  NS_DECL_ISUPPORTS

  nsSafariProfileMigrator();
  virtual ~nsSafariProfileMigrator();

  typedef enum { STRING, INT, BOOL } PrefType;

  typedef nsresult(*prefConverter)(void*, nsIPrefBranch*);

  struct PrefTransform {
    CFStringRef   keyName;
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
  
  static nsresult SetBool(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetBoolInverted(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetString(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetInt(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetDefaultEncoding(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetDownloadFolder(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetDownloadHandlers(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetDownloadRetention(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetDisplayImages(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetFontName(void* aTransform, nsIPrefBranch* aBranch);
  static nsresult SetFontSize(void* aTransform, nsIPrefBranch* aBranch);
  static void CleanResource(nsIRDFDataSource* aDataSource, nsIRDFResource* aResource);

protected:
  nsresult CopyPreferences(PRBool aReplace);
  nsresult CopyCookies(PRBool aReplace);
  







  nsresult CopyHistory(PRBool aReplace);
  nsresult CopyHistoryBatched(PRBool aReplace);
  








  nsresult CopyBookmarks(PRBool aReplace);
  nsresult CopyBookmarksBatched(PRBool aReplace);
  nsresult ParseBookmarksFolder(CFArrayRef aChildren, 
                                PRInt64 aParentFolder,
                                nsINavBookmarksService * aBookmarksService,
                                PRBool aIsAtRootLevel);
  nsresult CopyFormData(PRBool aReplace);
  nsresult CopyOtherData(PRBool aReplace);

  nsresult ProfileHasContentStyleSheet(PRBool *outExists);
  nsresult GetSafariUserStyleSheet(nsILocalFile** aResult);

private:
  PRBool HasFormDataToImport();
  nsCOMPtr<nsIObserverService> mObserverService;
};
 
#endif
