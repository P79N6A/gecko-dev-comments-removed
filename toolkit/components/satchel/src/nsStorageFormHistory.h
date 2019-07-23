





































#ifndef __nsFormHistory__
#define __nsFormHistory__

#include "nsIFormHistory.h"
#include "nsIFormSubmitObserver.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIPrefBranch.h"
#include "nsIUUIDGenerator.h"
#include "nsWeakReference.h"
#include "nsIMutableArray.h"

#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"

#include "nsServiceManagerUtils.h"
#include "nsToolkitCompsCID.h"

class nsIAutoCompleteSimpleResult;
class nsIAutoCompleteResult;
class nsFormHistory;
class nsIObserverService;
template <class E> class nsTArray;

#define NS_IFORMHISTORYPRIVATE_IID \
{0xc4a47315, 0xaeb5, 0x4039, {0x9f, 0x34, 0x45, 0x11, 0xb3, 0xa7, 0x58, 0xdd}}

class nsIFormHistoryPrivate : public nsISupports
{
 public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMHISTORYPRIVATE_IID)

  mozIStorageConnection* GetStorageConnection() { return mDBConn; }

 protected:
  nsCOMPtr<mozIStorageConnection> mDBConn;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormHistoryPrivate, NS_IFORMHISTORYPRIVATE_IID)

class nsFormHistory : public nsIFormHistory2,
                      public nsIFormHistoryPrivate,
                      public nsIObserver,
                      public nsIFormSubmitObserver,
                      public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFORMHISTORY2
  NS_DECL_NSIOBSERVER
  
  
  NS_IMETHOD Notify(nsIDOMHTMLFormElement* formElt, nsIDOMWindowInternal* window, nsIURI* actionURL, PRBool* cancelSubmit);

  nsFormHistory();
  nsresult Init();

 private:
  ~nsFormHistory();
  bool IsValidCCNumber(const nsAString &aString);

 protected:
  
  nsresult OpenDatabase(PRBool *aDoImport);
  nsresult CloseDatabase();
  nsresult GetDatabaseFile(nsIFile** aFile);

  nsresult dbMigrate();
  nsresult dbCleanup();
  nsresult MigrateToVersion1();
  nsresult MigrateToVersion2();
  nsresult MigrateToVersion3();
  PRBool   dbAreExpectedColumnsPresent();

  nsresult CreateTable();
  nsresult CreateStatements();

  static nsresult InitPrefs();
  static PRBool SaveHttpsForms();
  static PRBool FormHistoryEnabled();
  static nsFormHistory *gFormHistory;
  static PRBool gFormHistoryEnabled;
  static PRBool gSaveHttpsForms;
  static PRBool gPrefsInitialized;

  nsresult GenerateGUID(nsACString &guid);
  nsresult ExpireOldEntries();
  PRInt32 CountAllEntries();
  PRInt64 GetExistingEntryID (const nsAString &aName, const nsAString &aValue);
  PRInt64 GetExistingEntryID (const nsAString &aName, const nsAString &aValue, nsAString &aGuid);

  nsresult SendNotification(const nsAString &aChangeType, nsISupports *aData);
  nsresult SendNotification(const nsAString &aChangeType, const nsAString &aName);
  nsresult SendNotification(const nsAString &aChangeType, const nsAString &aName, const nsAString &aValue, const nsAutoString &aGuid);
  nsresult SendNotification(const nsAString &aChangeType, const PRInt64 &aNumber);
  nsresult SendNotification(const nsAString &aChangeType, const PRInt64 &aOne, const PRInt64 &aTwo);

  nsCOMPtr<nsIUUIDGenerator> mUUIDService;
  nsCOMPtr<nsIPrefBranch> mPrefBranch;
  nsCOMPtr<nsIObserverService> mObserverService;
  nsCOMPtr<mozIStorageService> mStorageService;
  nsCOMPtr<mozIStorageStatement> mDBFindEntry;
  nsCOMPtr<mozIStorageStatement> mDBFindEntryByName;
  nsCOMPtr<mozIStorageStatement> mDBSelectEntries;
  nsCOMPtr<mozIStorageStatement> mDBInsertNameValue;
  nsCOMPtr<mozIStorageStatement> mDBUpdateEntry;
};

#endif 
