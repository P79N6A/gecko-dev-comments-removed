




































#ifndef browserprofilemigratorutils___h___
#define browserprofilemigratorutils___h___

#define MIGRATION_ITEMBEFOREMIGRATE "Migration:ItemBeforeMigrate"
#define MIGRATION_ITEMMIGRATEERROR  "Migration:ItemError"
#define MIGRATION_ITEMAFTERMIGRATE  "Migration:ItemAfterMigrate"
#define MIGRATION_STARTED           "Migration:Started"
#define MIGRATION_ENDED             "Migration:Ended"

#define NOTIFY_OBSERVERS(message, item) \
  mObserverService->NotifyObservers(nsnull, message, item)

#define COPY_DATA(func, replace, itemIndex) \
  if ((aItems & itemIndex || !aItems)) { \
    nsAutoString index; \
    index.AppendInt(itemIndex); \
    NOTIFY_OBSERVERS(MIGRATION_ITEMBEFOREMIGRATE, index.get()); \
    if (NS_FAILED(func(replace))) \
      NOTIFY_OBSERVERS(MIGRATION_ITEMMIGRATEERROR, index.get()); \
    NOTIFY_OBSERVERS(MIGRATION_ITEMAFTERMIGRATE, index.get()); \
  }

#define NC_URI(property) \
  NS_LITERAL_CSTRING("http://home.netscape.com/NC-rdf#"#property)

#define BATCH_ACTION_HISTORY 0
#define BATCH_ACTION_HISTORY_REPLACE 1
#define BATCH_ACTION_BOOKMARKS 2
#define BATCH_ACTION_BOOKMARKS_REPLACE 3

#include "nsIPrefBranch.h"
#include "nsIFile.h"
#include "nsStringAPI.h"
#include "nsCOMPtr.h"

class nsIProfileStartup;


void SetUnicharPref(const char* aPref, const nsAString& aValue,
                    nsIPrefBranch* aPrefs);


void ParseOverrideServers(const nsAString& aServers, nsIPrefBranch* aBranch);
void SetProxyPref(const nsAString& aHostPort, const char* aPref, 
                  const char* aPortPref, nsIPrefBranch* aPrefs);

struct MigrationData { 
  PRUnichar* fileName; 
  PRUint32 sourceFlag;
  bool replaceOnly;
};

class nsILocalFile;
void GetMigrateDataFromArray(MigrationData* aDataArray, 
                             PRInt32 aDataArrayLength,
                             bool aReplace,
                             nsIFile* aSourceProfile, 
                             PRUint16* aResult);




void GetProfilePath(nsIProfileStartup* aStartup, nsCOMPtr<nsIFile>& aProfileDir);






nsresult AnnotatePersonalToolbarFolder(nsIFile* aSourceBookmarksFile,
                                       nsIFile* aTargetBookmarksFile,
                                       const char* aToolbarFolderName);







nsresult ImportBookmarksHTML(nsIFile* aBookmarksFile, 
                             bool aImportIntoRoot,
                             bool aOverwriteDefaults,
                             const PRUnichar* aImportSourceNameKey);

nsresult InitializeBookmarks(nsIFile* aTargetProfile);

#endif

