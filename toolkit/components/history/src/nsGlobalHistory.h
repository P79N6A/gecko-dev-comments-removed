







































#ifndef nsglobalhistory__h____
#define nsglobalhistory__h____

#include "nsIBrowserHistory.h"
#include "nsIGlobalHistory3.h"
#include "mdb.h"
#include "nsIObserver.h"
#include "nsIPrefBranch.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsISupportsArray.h"
#include "nsIStringBundle.h"
#include "nsWeakReference.h"
#include "nsVoidArray.h"
#include "nsHashtable.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsITimer.h"
#include "nsIAutoCompleteSearch.h"
#include "nsIAutoCompleteResult.h"
#include "nsIAutoCompleteResultTypes.h"
#include "nsHashSets.h"









class nsMdbTableEnumerator : public nsISimpleEnumerator
{
protected:
  nsMdbTableEnumerator();
  virtual ~nsMdbTableEnumerator();

  nsIMdbEnv*   mEnv;

private:
  
  nsIMdbTable* mTable;
  nsIMdbTableRowCursor* mCursor;
  nsIMdbRow*            mCurrent;

public:
  
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD HasMoreElements(PRBool* _result);
  NS_IMETHOD GetNext(nsISupports** _result);

  
  virtual nsresult Init(nsIMdbEnv* aEnv, nsIMdbTable* aTable);

protected:
  virtual PRBool   IsResult(nsIMdbRow* aRow) = 0;
  virtual nsresult ConvertToISupports(nsIMdbRow* aRow, nsISupports** aResult) = 0;
};

typedef PRBool (*rowMatchCallback)(nsIMdbRow *aRow, void *closure);

struct matchHost_t;
struct searchQuery;
class searchTerm;


#define AUTOCOMPLETE_PREFIX_LIST_COUNT 6

#define AUTOCOMPLETE_NONPAGE_VISIT_COUNT_BOOST 5












struct AutocompleteExclude {
  PRInt32 schemePrefix;
  PRInt32 hostnamePrefix;
};

class nsGlobalHistory : nsSupportsWeakReference,
                        public nsIBrowserHistory,
                        public nsIObserver,
                        public nsIRDFDataSource,
                        public nsIRDFRemoteDataSource,
                        public nsIAutoCompleteSearch,
                        public nsIGlobalHistory3
{
public:
  
  NS_DECL_ISUPPORTS

  NS_DECL_NSIGLOBALHISTORY2
  NS_DECL_NSIGLOBALHISTORY3
  NS_DECL_NSIBROWSERHISTORY
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIRDFDATASOURCE
  NS_DECL_NSIRDFREMOTEDATASOURCE
  NS_DECL_NSIAUTOCOMPLETESEARCH

  NS_METHOD Init();

  nsGlobalHistory(void);
  virtual ~nsGlobalHistory();

  
  PRBool MatchExpiration(nsIMdbRow *row, PRTime* expirationDate);
  PRBool MatchHost(nsIMdbRow *row, matchHost_t *hostInfo);
  PRBool RowMatches(nsIMdbRow* aRow, searchQuery *aQuery, PRBool caseSensitive);

protected:

  
  
  
  enum eCommitType 
  {
    kLargeCommit = 0,
    kSessionCommit = 1,
    kCompressCommit = 2
  };
  
  PRInt64   mFileSizeOnDisk;
  nsresult OpenDB();
  nsresult OpenExistingFile(nsIMdbFactory *factory, const char *filePath);
  nsresult OpenNewFile(nsIMdbFactory *factory, const char *filePath);
  nsresult CreateTokens();
  nsresult CloseDB();
  nsresult CheckHostnameEntries();
  nsresult Commit(eCommitType commitType);

  
  
  
  PRInt32   mExpireDays;
  nsresult ExpireEntries(PRBool notify);
  nsresult RemoveMatchingRows(rowMatchCallback aMatchFunc,
                              void *aClosure, PRBool notify);

  
  
  
  nsresult GetRootDayQueries(nsISimpleEnumerator **aResult, PRBool aBySite);
  nsresult GetFindUriName(const char *aURL, nsIRDFNode **aResult);
  nsresult CreateFindEnumerator(nsIRDFResource *aSource,
                                nsISimpleEnumerator **aResult);
  
  static nsresult FindUrlToTokenList(const char *aURL, nsVoidArray& aResult);
  static void FreeTokenList(nsVoidArray& tokens);
  static void FreeSearchQuery(searchQuery& aQuery);
  static PRBool IsFindResource(nsIRDFResource *aResource);
  void GetFindUriPrefix(const searchQuery& aQuery,
                        const PRBool aDoGroupBy,
                        nsACString& aResult);
  
  nsresult TokenListToSearchQuery(const nsVoidArray& tokens,
                                  searchQuery& aResult);
  nsresult FindUrlToSearchQuery(const char *aURL, searchQuery& aResult);
  nsresult NotifyFindAssertions(nsIRDFResource *aSource, nsIMdbRow *aRow);
  nsresult NotifyFindUnassertions(nsIRDFResource *aSource, nsIMdbRow *aRow);
    
  
  
  
  PRBool mAutocompleteOnlyTyped;
  nsStringArray mIgnoreSchemes;
  nsStringArray mIgnoreHostnames;
  
  nsresult AutoCompleteTypedSearch(nsIAutoCompleteMdbResult2 **aResult);
  nsresult AutoCompleteSearch(const nsAString& aSearchString,
                              AutocompleteExclude* aExclude,
                              nsIAutoCompleteMdbResult2* aPrevResult,
                              nsIAutoCompleteMdbResult2** aResult);
  void AutoCompleteCutPrefix(nsAString& aURL, AutocompleteExclude* aExclude);
  void AutoCompleteGetExcludeInfo(const nsAString& aURL, AutocompleteExclude* aExclude);
  nsString AutoCompletePrefilter(const nsAString& aSearchString);
  PRBool AutoCompleteCompare(nsAString& aHistoryURL, 
                             const nsAString& aUserURL,
                             AutocompleteExclude* aExclude);
  PR_STATIC_CALLBACK(int)
  AutoCompleteSortComparison(nsIMdbRow *row1, nsIMdbRow *row2, void *closureVoid);

  
  
  struct AutoCompleteSortClosure
  {
    nsGlobalHistory* history;
    size_t prefixCount;
    const nsAFlatString* prefixes[AUTOCOMPLETE_PREFIX_LIST_COUNT];
  };

  
  
  PRTime    mLastNow;           
  PRInt32   mBatchesInProgress;
  PRBool    mNowValid;          
  nsCOMPtr<nsITimer> mExpireNowTimer;
  
  PRTime GetNow();
  void ExpireNow();
  
  static void expireNowTimer(nsITimer *aTimer, void *aClosure)
  {((nsGlobalHistory *)aClosure)->ExpireNow(); }
  
  
  
  
  PRBool    mDirty;             
  nsCOMPtr<nsITimer> mSyncTimer;
  
  void Sync();
  nsresult SetDirty();
  
  static void fireSyncTimer(nsITimer *aTimer, void *aClosure)
  {((nsGlobalHistory *)aClosure)->Sync(); }

  
  
  
  nsCOMPtr<nsISupportsArray> mObservers;
  
  PRBool IsURLInHistory(nsIRDFResource* aResource);
  
  nsresult NotifyAssert(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aValue);
  nsresult NotifyUnassert(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aValue);
  nsresult NotifyChange(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aOldValue, nsIRDFNode* aNewValue);

  
  
  
  
  
  nsIMdbEnv* mEnv;         
  nsIMdbStore* mStore;     
  nsIMdbTable* mTable;     
  
  nsCOMPtr<nsIMdbRow> mMetaRow;
  
  mdb_scope  kToken_HistoryRowScope;
  mdb_kind   kToken_HistoryKind;

  mdb_column kToken_URLColumn;
  mdb_column kToken_ReferrerColumn;
  mdb_column kToken_LastVisitDateColumn;
  mdb_column kToken_FirstVisitDateColumn;
  mdb_column kToken_VisitCountColumn;
  mdb_column kToken_NameColumn;
  mdb_column kToken_HostnameColumn;
  mdb_column kToken_HiddenColumn;
  mdb_column kToken_TypedColumn;
  mdb_column kToken_GeckoFlagsColumn;

  mdb_column kToken_ByteOrder;
  
  mdb_column kToken_LastPageVisited;

  
  nsCStringHashSet mTypedHiddenURIs;
  
  
  
  
  nsresult AddPageToDatabase(nsIURI* aURI, PRBool aRedirect, PRBool aTopLevel,
                             PRTime aLastVisitDate, nsIURI * aReferrer);
  nsresult AddExistingPageToDatabase(nsIMdbRow *row,
                                     PRTime aDate,
                                     nsIURI *aReferrer,
                                     PRTime *aOldDate,
                                     PRInt32 *aOldCount);
  nsresult AddNewPageToDatabase(nsIURI* aURI,
                                PRTime aDate,
                                PRBool aRedirect,
                                PRBool aTopLevel,
                                nsIURI *aReferrer,
                                nsIMdbRow **aResult);

  nsresult RemovePageInternal(const char *aSpec);

  
  
  
  nsresult SetRowValue(nsIMdbRow *aRow, mdb_column aCol, const PRTime& aValue);
  nsresult SetRowValue(nsIMdbRow *aRow, mdb_column aCol, const PRInt32 aValue);
  nsresult SetRowValue(nsIMdbRow *aRow, mdb_column aCol, const char *aValue);
  nsresult SetRowValue(nsIMdbRow *aRow, mdb_column aCol, const PRUnichar *aValue);

  nsresult GetRowValue(nsIMdbRow *aRow, mdb_column aCol, nsAString& aResult);
  
  
  nsresult SaveByteOrder(const char *aByteOrder);
  nsresult GetByteOrder(char **_retval);
  nsresult InitByteOrder(PRBool aForce);
  void SwapBytes(const PRUnichar *source, PRUnichar *dest, PRInt32 aLen);
  PRBool mReverseByteOrder;

  
  nsresult GetRowValue(nsIMdbRow *aRow, mdb_column aCol, nsACString& aResult);
  nsresult GetRowValue(nsIMdbRow *aRow, mdb_column aCol, PRTime* aResult);
  nsresult GetRowValue(nsIMdbRow *aRow, mdb_column aCol, PRInt32* aResult);

  
  
  nsresult FindRow(mdb_column aCol, const char *aURL, nsIMdbRow **aResult);

  
  
  
  nsCOMPtr<nsIStringBundle> mBundle;

  
  
  static PRInt32 gRefCnt;
  static nsIRDFService* gRDFService;
  static nsIRDFResource* kNC_Page; 
  static nsIRDFResource* kNC_Date;
  static nsIRDFResource* kNC_FirstVisitDate;
  static nsIRDFResource* kNC_VisitCount;
  static nsIRDFResource* kNC_AgeInDays;
  static nsIRDFResource* kNC_Name;
  static nsIRDFResource* kNC_NameSort;
  static nsIRDFResource* kNC_Hostname;
  static nsIRDFResource* kNC_Referrer;
  static nsIRDFResource* kNC_child;
  static nsIRDFResource* kNC_URL;  
  static nsIRDFResource* kNC_HistoryRoot;
  static nsIRDFResource* kNC_HistoryByDate;
  static nsIRDFResource* kNC_HistoryByDateAndSite;
  static nsIRDFResource* kNC_DayFolderIndex;

  static nsIMdbFactory* gMdbFactory;
  static nsIPrefBranch* gPrefBranch;
  
  
  

  
  
  class URLEnumerator : public nsMdbTableEnumerator
  {
  protected:
    mdb_column mURLColumn;
    mdb_column mHiddenColumn;
    mdb_column mSelectColumn;
    void*      mSelectValue;
    PRInt32    mSelectValueLen;

    virtual ~URLEnumerator();

  public:
    URLEnumerator(mdb_column aURLColumn,
                  mdb_column aHiddenColumn,
                  mdb_column aSelectColumn = mdb_column(0),
                  void* aSelectValue = nsnull,
                  PRInt32 aSelectValueLen = 0) :
      mURLColumn(aURLColumn),
      mHiddenColumn(aHiddenColumn),
      mSelectColumn(aSelectColumn),
      mSelectValue(aSelectValue),
      mSelectValueLen(aSelectValueLen)
    {}

  protected:
    virtual PRBool   IsResult(nsIMdbRow* aRow);
    virtual nsresult ConvertToISupports(nsIMdbRow* aRow, nsISupports** aResult);
  };

  
  class SearchEnumerator : public nsMdbTableEnumerator
  {
  public:
    SearchEnumerator(searchQuery *aQuery,
                     mdb_column aHiddenColumn,
                     nsGlobalHistory *aHistory) :
      mQuery(aQuery),
      mHiddenColumn(aHiddenColumn),
      mHistory(aHistory)
    {}

    virtual ~SearchEnumerator();

  protected:
    searchQuery *mQuery;
    mdb_column mHiddenColumn;
    nsGlobalHistory *mHistory;
    nsHashtable mUniqueRows;
    
    nsCString mFindUriPrefix;

    virtual PRBool IsResult(nsIMdbRow* aRow);
    virtual nsresult ConvertToISupports(nsIMdbRow* aRow,
                                        nsISupports** aResult);
    
    PRBool RowMatches(nsIMdbRow* aRow, searchQuery *aQuery);
  };


  friend class URLEnumerator;
  friend class SearchEnumerator;
};


#endif
