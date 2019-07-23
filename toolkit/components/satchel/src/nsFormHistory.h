




































#ifndef __nsFormHistory__
#define __nsFormHistory__

#include "nsIFormHistory.h"
#include "nsIAutoCompleteResultTypes.h"
#include "nsIFormSubmitObserver.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIPrefBranch.h"
#include "nsWeakReference.h"
#include "mdb.h"
#include "nsIServiceManager.h"
#include "nsToolkitCompsCID.h"

class nsFormHistory : public nsIFormHistory2,
                      public nsIObserver,
                      public nsIFormSubmitObserver,
                      public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFORMHISTORY2
  NS_DECL_NSIOBSERVER
  
  
  NS_IMETHOD Notify(nsIContent* formNode, nsIDOMWindowInternal* window, nsIURI* actionURL, PRBool* cancelSubmit);

  nsFormHistory();
  virtual ~nsFormHistory();
  nsresult Init();

  static nsFormHistory *GetInstance()
    {
      if (!gFormHistory) {
        nsCOMPtr<nsIFormHistory2> fh = do_GetService(NS_FORMHISTORY_CONTRACTID);
      }
      return gFormHistory;
    }

  nsresult AutoCompleteSearch(const nsAString &aInputName, const nsAString &aInputValue,
                              nsIAutoCompleteMdbResult2 *aPrevResult, nsIAutoCompleteResult **aNewResult);

  static mdb_column kToken_ValueColumn;
  static mdb_column kToken_NameColumn;

protected:
  
  nsresult OpenDatabase();
  nsresult OpenExistingFile(const char *aPath);
  nsresult CreateNewFile(const char *aPath);
  nsresult CloseDatabase();
  nsresult CreateTokens();
  nsresult Flush();
  nsresult CopyRowsFromTable(nsIMdbTable *sourceTable);
  
  mdb_err UseThumb(nsIMdbThumb *aThumb, PRBool *aDone);
  
  nsresult AppendRow(const nsAString &aValue, const nsAString &aName, nsIMdbRow **aResult);
  nsresult SetRowValue(nsIMdbRow *aRow, mdb_column aCol, const nsAString &aValue);
  nsresult GetRowValue(nsIMdbRow *aRow, mdb_column aCol, nsAString &aValue);
  
  PRBool RowMatch(nsIMdbRow *aRow, const nsAString &aInputName, const nsAString &aInputValue, PRUnichar **aValue);
  
  PR_STATIC_CALLBACK(int) SortComparison(const void *v1, const void *v2, void *closureVoid);

  nsresult EntriesExistInternal(const nsAString *aName, const nsAString *aValue, PRBool *_retval);

  nsresult RemoveEntriesInternal(const nsAString *aName);

  nsresult InitByteOrder(PRBool aForce);
  nsresult GetByteOrder(nsAString& aByteOrder);
  nsresult SaveByteOrder(const nsAString& aByteOrder);

  static PRBool FormHistoryEnabled();

  static nsFormHistory *gFormHistory;

  static PRBool gFormHistoryEnabled;
  static PRBool gPrefsInitialized;

  nsCOMPtr<nsIMdbFactory> mMdbFactory;
  nsCOMPtr<nsIPrefBranch> mPrefBranch;
  nsIMdbEnv* mEnv;
  nsIMdbStore* mStore;
  nsIMdbTable* mTable;
  PRInt64 mFileSizeOnDisk;
  nsCOMPtr<nsIMdbRow> mMetaRow;
  PRPackedBool mReverseByteOrder;
  
  
  mdb_scope kToken_RowScope;
  mdb_kind kToken_Kind;
  mdb_column kToken_ByteOrder;
};

#endif 
