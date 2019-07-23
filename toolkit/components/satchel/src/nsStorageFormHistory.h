





































#ifndef __nsFormHistory__
#define __nsFormHistory__

#include "nsIFormHistory.h"
#include "nsIFormSubmitObserver.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIPrefBranch.h"
#include "nsWeakReference.h"

#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"

#include "nsServiceManagerUtils.h"
#include "nsToolkitCompsCID.h"
#ifdef MOZ_MORKREADER
#include "nsMorkReader.h"
#endif

class nsIAutoCompleteSimpleResult;
class nsIAutoCompleteResult;
class nsFormHistory;
template <class E> class nsTArray;

#define NS_IFORMHISTORYPRIVATE_IID \
{0xc4a47315, 0xaeb5, 0x4039, {0x9f, 0x34, 0x45, 0x11, 0xb3, 0xa7, 0x58, 0xdd}}

class nsIFormHistoryPrivate : public nsISupports
{
 public:
#ifdef MOZILLA_1_8_BRANCH
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IFORMHISTORYPRIVATE_IID)
#else
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFORMHISTORYPRIVATE_IID)
#endif

  mozIStorageConnection* GetStorageConnection() { return mDBConn; }

 protected:
  nsCOMPtr<mozIStorageConnection> mDBConn;
};

#ifndef MOZILLA_1_8_BRANCH
NS_DEFINE_STATIC_IID_ACCESSOR(nsIFormHistoryPrivate, NS_IFORMHISTORYPRIVATE_IID)
#endif

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

  static nsFormHistory* GetInstance()
    {
      if (!gFormHistory) {
        nsCOMPtr<nsIFormHistory2> fh = do_GetService(NS_FORMHISTORY_CONTRACTID);
      }
      return gFormHistory;
    }

  nsresult AutoCompleteSearch(const nsAString &aInputName,
			      const nsAString &aInputValue,
                              nsIAutoCompleteSimpleResult *aPrevResult,
			      nsIAutoCompleteResult **aNewResult);

 private:
  ~nsFormHistory();

 protected:
  
  nsresult OpenDatabase();
  nsresult CloseDatabase();
  nsresult GetDatabaseFile(nsIFile** aFile);

  static PRBool FormHistoryEnabled();
  static nsFormHistory *gFormHistory;
  static PRBool gFormHistoryEnabled;
  static PRBool gPrefsInitialized;

  nsCOMPtr<nsIPrefBranch> mPrefBranch;
  nsCOMPtr<mozIStorageService> mStorageService;
  nsCOMPtr<mozIStorageStatement> mDBGetMatchingField;
  nsCOMPtr<mozIStorageStatement> mDBFindEntry;
  nsCOMPtr<mozIStorageStatement> mDBFindEntryByName;
  nsCOMPtr<mozIStorageStatement> mDBSelectEntries;
  nsCOMPtr<mozIStorageStatement> mDBInsertNameValue;

  
  nsresult StartCache();
  nsresult StopCache();
  nsCOMPtr<mozIStorageConnection> mDummyConnection;
  nsCOMPtr<mozIStorageStatement> mDummyStatement;
};

#ifdef MOZ_MORKREADER
class nsFormHistoryImporter : public nsIFormHistoryImporter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFORMHISTORYIMPORTER

private:
  
  static PLDHashOperator PR_CALLBACK
  AddToFormHistoryCB(const nsCSubstring &aRowID,
                     const nsTArray<nsCString> *aValues,
                     void *aData);
};
#endif

#endif 
