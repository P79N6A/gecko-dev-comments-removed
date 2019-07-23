





































#ifndef nsNavHistory_h_
#define nsNavHistory_h_

#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsDataHashtable.h"
#include "nsINavHistoryService.h"
#include "nsIAutoCompleteSearch.h"
#include "nsIAutoCompleteResult.h"
#include "nsIAutoCompleteSimpleResult.h"
#include "nsIBrowserHistory.h"
#include "nsICollation.h"
#include "nsIDateTimeFormat.h"
#include "nsIGlobalHistory.h"
#include "nsIGlobalHistory3.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsIStringBundle.h"
#include "nsITimer.h"
#include "nsITreeSelection.h"
#include "nsITreeView.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "nsWeakReference.h"
#include "nsTArray.h"
#include "nsINavBookmarksService.h"
#include "nsMaybeWeakPtr.h"

#include "nsNavHistoryExpire.h"
#include "nsNavHistoryResult.h"
#include "nsNavHistoryQuery.h"





#define LAZY_ADD

#define QUERYUPDATE_TIME 0
#define QUERYUPDATE_SIMPLE 1
#define QUERYUPDATE_COMPLEX 2
#define QUERYUPDATE_COMPLEX_WITH_BOOKMARKS 3

class AutoCompleteIntermediateResult;
class AutoCompleteResultComparator;
class mozIAnnotationService;
class nsNavHistory;
class nsNavBookmarks;
class QueryKeyValuePair;



class nsNavHistory : public nsSupportsWeakReference,
                     public nsINavHistoryService,
                     public nsIObserver,
                     public nsIBrowserHistory,
                     public nsIGlobalHistory3,
                     public nsIAutoCompleteSearch
{
  friend class AutoCompleteIntermediateResultSet;
  friend class AutoCompleteResultComparator;
public:
  nsNavHistory();

  NS_DECL_ISUPPORTS

  NS_DECL_NSINAVHISTORYSERVICE
  NS_DECL_NSIGLOBALHISTORY2
  NS_DECL_NSIGLOBALHISTORY3
  NS_DECL_NSIBROWSERHISTORY
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIAUTOCOMPLETESEARCH

  nsresult Init();

  




  static nsNavHistory* GetHistoryService()
  {
    if (! gHistoryService) {
      nsresult rv;
      nsCOMPtr<nsINavHistoryService> serv(do_GetService("@mozilla.org/browser/nav-history-service;1", &rv));
      NS_ENSURE_SUCCESS(rv, nsnull);

      
      
      NS_ASSERTION(gHistoryService, "History service creation failed");
    }
    return gHistoryService;
  }

  



  void SyncDB()
  {
    #ifdef LAZY_ADD
      CommitLazyMessages();
    #endif
  }

#ifdef LAZY_ADD
  



  nsresult AddLazyLoadFaviconMessage(nsIURI* aPage, nsIURI* aFavicon,
                                     PRBool aForceReload);
#endif

  



  nsresult GetUrlIdFor(nsIURI* aURI, PRInt64* aEntryID,
                       PRBool aAutoCreate);

  









  mozIStorageConnection* GetStorageConnection()
  {
    return mDBConn;
  }

#ifdef IN_MEMORY_LINKS
  mozIStorageConnection* GetMemoryStorageConnection()
  {
    return mMemDBConn;
  }
#endif

  
  nsresult StartDummyStatement();
  nsresult StopDummyStatement();

  



  nsIStringBundle* GetBundle()
    { return mBundle; }
  nsILocale* GetLocale()
    { return mLocale; }
  nsICollation* GetCollation()
    { return mCollation; }
  nsIDateTimeFormat* GetDateFormatter()
    { return mDateFormatter; }

  
  PRBool IsHistoryDisabled() { return mExpireDays == 0; }

  
  void SaveExpandItem(const nsAString& aTitle);
  void SaveCollapseItem(const nsAString& aTitle);

  
  mozIStorageStatement* DBGetURLPageInfo() { return mDBGetURLPageInfo; }

  
  static const PRInt32 kGetInfoIndex_PageID;
  static const PRInt32 kGetInfoIndex_URL;
  static const PRInt32 kGetInfoIndex_Title;
  static const PRInt32 kGetInfoIndex_RevHost;
  static const PRInt32 kGetInfoIndex_VisitCount;
  static const PRInt32 kGetInfoIndex_ItemId;
  static const PRInt32 kGetInfoIndex_ItemDateAdded;
  static const PRInt32 kGetInfoIndex_ItemLastModified;

  
  mozIStorageStatement* DBGetURLPageInfoFull()
  { return mDBGetURLPageInfoFull; }

  
  mozIStorageStatement* DBGetIdPageInfo() { return mDBGetIdPageInfo; }

  
  mozIStorageStatement* DBGetIdPageInfoFull()
  { return mDBGetIdPageInfoFull; }

  
  
  static const PRInt32 kGetInfoIndex_VisitDate;
  static const PRInt32 kGetInfoIndex_FaviconURL;

  
  static const PRInt32 kGetInfoIndex_SessionId;

  
  
  nsresult GetQueryResults(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                           nsNavHistoryQueryOptions *aOptions,
                           nsCOMArray<nsNavHistoryResultNode>* aResults);

  
  
  nsresult RowToResult(mozIStorageValueArray* aRow,
                       nsNavHistoryQueryOptions* aOptions,
                       nsNavHistoryResultNode** aResult);
  nsresult QueryRowToResult(const nsACString& aURI, const nsACString& aTitle,
                            PRUint32 aAccessCount, PRTime aTime,
                            const nsACString& aFavicon,
                            nsNavHistoryResultNode** aNode);

  nsresult VisitIdToResultNode(PRInt64 visitId,
                               nsNavHistoryQueryOptions* aOptions,
                               nsNavHistoryResultNode** aResult);
  nsresult UriToResultNode(nsIURI* aUri,
                           nsNavHistoryQueryOptions* aOptions,
                           nsNavHistoryResultNode** aResult);
  nsresult BookmarkIdToResultNode(PRInt64 aBookmarkId,
                                  nsNavHistoryQueryOptions* aOptions,
                                  nsNavHistoryResultNode** aResult);

  
  
  void SendPageChangedNotification(nsIURI* aURI, PRUint32 aWhat,
                                   const nsAString& aValue)
  {
    ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                        OnPageChanged(aURI, aWhat, aValue));
  }

  
  PRTime GetNow();

  
  static const char kAnnotationPreviousEncoding[];

  
  static PRUint32 GetUpdateRequirements(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                        nsNavHistoryQueryOptions* aOptions,
                                        PRBool* aHasSearchTerms);
  PRBool EvaluateQueryForNode(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions* aOptions,
                              nsNavHistoryResultNode* aNode);

  static nsresult AsciiHostNameFromHostString(const nsACString& aHostName,
                                              nsACString& aAscii);
  static void DomainNameFromHostName(const nsCString& aHostName,
                                     nsACString& aDomainName);
  static PRTime NormalizeTime(PRUint32 aRelative, PRTime aOffset);
  nsresult RecursiveGroup(const nsCOMArray<nsNavHistoryResultNode>& aSource,
                          const PRUint16* aGroupingMode, PRUint32 aGroupCount,
                          nsCOMArray<nsNavHistoryResultNode>* aDest);

  
  nsresult QueryStringToQueryArray(const nsACString& aQueryString,
                                   nsCOMArray<nsNavHistoryQuery>* aQueries,
                                   nsNavHistoryQueryOptions** aOptions);

  
  
  
  
  
  nsresult AddPageWithVisit(nsIURI *aURI,
                            const nsString &aTitle,
                            const nsString &aUserTitle,
                            PRBool aHidden, PRBool aTyped,
                            PRInt32 aVisitCount,
                            PRInt32 aLastVisitTransition,
                            PRTime aLastVisitDate);

  
  
  
  nsresult RemoveDuplicateURIs();

  
  nsresult UpdateSchemaVersion();

 private:
  ~nsNavHistory();

  
  static nsNavHistory* gHistoryService;

protected:

  
  
  
  nsCOMPtr<nsIPrefBranch> mPrefBranch; 
  nsDataHashtable<nsStringHashKey, int> gExpandedItems;

  
  
  
  nsCOMPtr<mozIStorageService> mDBService;
  nsCOMPtr<mozIStorageConnection> mDBConn;

  nsCOMPtr<mozIStorageStatement> mDBGetURLPageInfo;   
  nsCOMPtr<mozIStorageStatement> mDBGetURLPageInfoFull; 
  nsCOMPtr<mozIStorageStatement> mDBGetIdPageInfo;     
  nsCOMPtr<mozIStorageStatement> mDBGetIdPageInfoFull; 
  nsCOMPtr<mozIStorageStatement> mDBFullAutoComplete; 
  static const PRInt32 kAutoCompleteIndex_URL;
  static const PRInt32 kAutoCompleteIndex_Title;
  static const PRInt32 kAutoCompleteIndex_VisitCount;
  static const PRInt32 kAutoCompleteIndex_Typed;

  nsCOMPtr<mozIStorageStatement> mDBRecentVisitOfURL; 
  nsCOMPtr<mozIStorageStatement> mDBInsertVisit; 
  nsCOMPtr<mozIStorageStatement> mDBGetPageVisitStats; 
  nsCOMPtr<mozIStorageStatement> mDBUpdatePageVisitStats; 
  nsCOMPtr<mozIStorageStatement> mDBAddNewPage; 

  
  nsCOMPtr<mozIStorageStatement> mDBVisitToURLResult; 
  nsCOMPtr<mozIStorageStatement> mDBVisitToVisitResult; 
  nsCOMPtr<mozIStorageStatement> mDBUrlToUrlResult; 
  nsCOMPtr<mozIStorageStatement> mDBBookmarkToUrlResult; 

  nsresult InitDB(PRBool *aDoImport);
  nsresult InitStatements();
  nsresult ForceMigrateBookmarksDB(mozIStorageConnection *aDBConn);
  nsresult MigrateV3Up(mozIStorageConnection *aDBConn);

#ifdef IN_MEMORY_LINKS
  
  nsCOMPtr<mozIStorageConnection> mMemDBConn;
  nsCOMPtr<mozIStorageStatement> mMemDBAddPage;
  nsCOMPtr<mozIStorageStatement> mMemDBGetPage;

  nsresult InitMemDB();
#endif

  
  nsCOMPtr<mozIStorageConnection> mDummyDBConn;
  nsCOMPtr<mozIStorageStatement> mDBDummyStatement;

  nsresult AddURIInternal(nsIURI* aURI, PRTime aTime, PRBool aRedirect,
                          PRBool aToplevel, nsIURI* aReferrer);

  nsresult AddVisitChain(nsIURI* aURI, PRTime aTime,
                         PRBool aToplevel, PRBool aRedirect,
                         nsIURI* aReferrer, PRInt64* aVisitID,
                         PRInt64* aSessionID, PRInt64* aRedirectBookmark);
  nsresult InternalAddNewPage(nsIURI* aURI, const nsAString& aTitle,
                              PRBool aHidden, PRBool aTyped,
                              PRInt32 aVisitCount, PRInt64* aPageID);
  nsresult InternalAddVisit(PRInt64 aPageID, PRInt64 aReferringVisit,
                            PRInt64 aSessionID, PRTime aTime,
                            PRInt32 aTransitionType, PRInt64* aVisitID);
  PRBool FindLastVisit(nsIURI* aURI, PRInt64* aVisitID,
                       PRInt64* aSessionID);
  PRBool IsURIStringVisited(const nsACString& url);
  nsresult LoadPrefs();

  
  PRTime mLastNow;
  PRBool mNowValid;
  nsCOMPtr<nsITimer> mExpireNowTimer;
  static void expireNowTimerCallback(nsITimer* aTimer, void* aClosure);

  
  friend class nsNavHistoryExpire;
  nsNavHistoryExpire mExpire;

#ifdef LAZY_ADD
  
  struct LazyMessage {
    enum MessageType { Type_Invalid, Type_AddURI, Type_Title, Type_Favicon };
    LazyMessage()
    {
      type = Type_Invalid;
      isRedirect = PR_FALSE;
      isToplevel = PR_FALSE;
      time = 0;
      alwaysLoadFavicon = PR_FALSE;
    }

    
    
    nsresult Init(MessageType aType, nsIURI* aURI)
    {
      type = aType;
      nsresult rv = aURI->Clone(getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, rv);
      return uri->GetSpec(uriSpec);
    }

    
    MessageType type;
    nsCOMPtr<nsIURI> uri;
    nsCString uriSpec; 

    
    nsCOMPtr<nsIURI> referrer;
    PRBool isRedirect;
    PRBool isToplevel;
    PRTime time;

    
    nsString title;

    
    nsCOMPtr<nsIURI> favicon;
    PRBool alwaysLoadFavicon;
  };
  nsTArray<LazyMessage> mLazyMessages;
  nsCOMPtr<nsITimer> mLazyTimer;
  PRBool mLazyTimerSet;
  PRUint32 mLazyTimerDeferments; 
  nsresult StartLazyTimer();
  nsresult AddLazyMessage(const LazyMessage& aMessage);
  static void LazyTimerCallback(nsITimer* aTimer, void* aClosure);
  void CommitLazyMessages();
#endif

  nsresult QueryToSelectClause(nsNavHistoryQuery* aQuery,
                               nsNavHistoryQueryOptions* aOptions,
                               PRInt32 aStartParameter,
                               nsCString* aClause,
                               PRInt32* aParamCount,
                               const nsACString& aCommonConditions);
  nsresult BindQueryClauseParameters(mozIStorageStatement* statement,
                                     PRInt32 aStartParameter,
                                     nsNavHistoryQuery* aQuery,
                                     PRInt32* aParamCount);

  nsresult ResultsAsList(mozIStorageStatement* statement,
                         nsNavHistoryQueryOptions* aOptions,
                         nsCOMArray<nsNavHistoryResultNode>* aResults);

  void GetAgeInDaysString(PRInt32 aInt, const PRUnichar *aName, 
                          nsACString& aResult);

  void GetStringFromName(const PRUnichar *aName, nsACString& aResult);

  void TitleForDomain(const nsCString& domain, nsACString& aTitle);

  nsresult SetPageTitleInternal(nsIURI* aURI, PRBool aIsUserTitle,
                                const nsAString& aTitle);

  nsresult GroupByDay(const nsCOMArray<nsNavHistoryResultNode>& aSource,
                       nsCOMArray<nsNavHistoryResultNode>* aDest);

  nsresult GroupByHost(const nsCOMArray<nsNavHistoryResultNode>& aSource,
                       nsCOMArray<nsNavHistoryResultNode>* aDest,
                       PRBool aIsDomain);

  nsresult FilterResultSet(const nsCOMArray<nsNavHistoryResultNode>& aSet,
                           nsCOMArray<nsNavHistoryResultNode>* aFiltered,
                           const nsString& aSearch);

  
  nsMaybeWeakPtrArray<nsINavHistoryObserver> mObservers;
  PRInt32 mBatchesInProgress;

  
  nsCOMPtr<nsIStringBundle> mBundle;
  nsCOMPtr<nsILocale> mLocale;
  nsCOMPtr<nsICollation> mCollation;
  nsCOMPtr<nsIDateTimeFormat> mDateFormatter;

  
  

  
  typedef nsDataHashtable<nsCStringHashKey, PRInt64> RecentEventHash;
  RecentEventHash mRecentTyped;
  RecentEventHash mRecentBookmark;

  PRBool CheckIsRecentEvent(RecentEventHash* hashTable,
                            const nsACString& url);
  void ExpireNonrecentEvents(RecentEventHash* hashTable);

  
  struct RedirectInfo {
    nsCString mSourceURI;
    PRTime mTimeCreated;
    PRUint32 mType; 
  };
  typedef nsDataHashtable<nsCStringHashKey, RedirectInfo> RedirectHash;
  RedirectHash mRecentRedirects;
  PR_STATIC_CALLBACK(PLDHashOperator) ExpireNonrecentRedirects(
      nsCStringHashKey::KeyType aKey, RedirectInfo& aData, void* aUserArg);
  PRBool GetRedirectFor(const nsACString& aDestination, nsACString& aSource,
                        PRTime* aTime, PRUint32* aRedirectType);

  
  PRInt64 mLastSessionID;
  PRInt64 GetNewSessionID() { mLastSessionID ++; return mLastSessionID; }

  
  
  
  struct AutoCompletePrefix
  {
    AutoCompletePrefix(const nsAString& aPrefix, PRBool aSecondLevel) :
      prefix(aPrefix), secondLevel(aSecondLevel) {}

    
    nsString prefix;

    
    
    
    
    
    PRBool secondLevel;
  };
  nsTArray<AutoCompletePrefix> mAutoCompletePrefixes;

  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteQuery;
  nsresult InitAutoComplete();
  nsresult CreateAutoCompleteQuery();
  PRInt32 mAutoCompleteMaxCount;
  PRInt32 mExpireDays;
  PRBool mAutoCompleteOnlyTyped;

  
  
  struct AutoCompleteExclude {
    
    PRInt32 schemePrefix;
    PRInt32 hostnamePrefix;

    
    PRInt32 postPrefixOffset;
  };

  nsresult AutoCompleteTypedSearch(nsIAutoCompleteSimpleResult* result);
  nsresult AutoCompleteFullHistorySearch(const nsAString& aSearchString,
                                         nsIAutoCompleteSimpleResult* result);
  nsresult AutoCompleteQueryOnePrefix(const nsString& aSearchString,
                                      const nsTArray<PRInt32>& aExcludePrefixes,
                                      PRInt32 aPriorityDelta,
                                      nsTArray<AutoCompleteIntermediateResult>* aResult);
  PRInt32 AutoCompleteGetPrefixLength(const nsString& aSpec);

  
  nsresult TokensToQueries(const nsTArray<QueryKeyValuePair>& aTokens,
                           nsCOMArray<nsNavHistoryQuery>* aQueries,
                           nsNavHistoryQueryOptions* aOptions);

  
  
  nsresult CreateLookupIndexes();
};





nsresult BindStatementURI(mozIStorageStatement* statement, PRInt32 index,
                          nsIURI* aURI);

#define PLACES_URI_PREFIX "place:"


inline PRBool IsQueryURI(const nsCString &uri)
{
  return StringBeginsWith(uri, NS_LITERAL_CSTRING(PLACES_URI_PREFIX));
}


inline const nsDependentCSubstring QueryURIToQuery(const nsCString &uri)
{
  NS_ASSERTION(IsQueryURI(uri), "should only be called for query URIs");
  return Substring(uri, NS_LITERAL_CSTRING(PLACES_URI_PREFIX).Length());
}

#endif 
