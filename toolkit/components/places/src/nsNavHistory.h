







































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
#include "nsPIPlacesDatabase.h"
#include "nsPIPlacesHistoryListenersNotifier.h"
#ifdef MOZ_XUL
#include "nsIAutoCompleteController.h"
#include "nsIAutoCompleteInput.h"
#include "nsIAutoCompletePopup.h"
#include "nsIAutoCompleteSearch.h"
#include "nsIAutoCompleteResult.h"
#include "nsIAutoCompleteSimpleResult.h"
#endif
#include "nsIBrowserHistory.h"
#include "nsICollation.h"
#include "nsIGlobalHistory.h"
#include "nsIGlobalHistory3.h"
#include "nsIDownloadHistory.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsIStringBundle.h"
#include "nsITextToSubURI.h"
#include "nsITimer.h"
#ifdef MOZ_XUL
#include "nsITreeSelection.h"
#include "nsITreeView.h"
#endif
#include "nsString.h"
#include "nsWeakReference.h"
#include "nsTArray.h"
#include "nsINavBookmarksService.h"
#include "nsMaybeWeakPtr.h"

#include "nsNavHistoryExpire.h"
#include "nsNavHistoryResult.h"
#include "nsNavHistoryQuery.h"

#include "nsICharsetResolver.h"

#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"


#define IN_MEMORY_SQLITE_TEMP_STORE


#define LAZY_ADD

#define QUERYUPDATE_TIME 0
#define QUERYUPDATE_SIMPLE 1
#define QUERYUPDATE_COMPLEX 2
#define QUERYUPDATE_COMPLEX_WITH_BOOKMARKS 3
#define QUERYUPDATE_HOST 4




#define SQL_STR_FRAGMENT_MAX_VISIT_DATE_BASE( __place_relation, __table_name ) \
  "(SELECT visit_date FROM " __table_name \
  " WHERE place_id = " __place_relation \
  " AND visit_type NOT IN (0,4,7) ORDER BY visit_date DESC LIMIT 1)"

#define SQL_STR_FRAGMENT_MAX_VISIT_DATE( __place_relation ) \
  "IFNULL( " SQL_STR_FRAGMENT_MAX_VISIT_DATE_BASE( __place_relation, "moz_historyvisits_temp") \
          ", " SQL_STR_FRAGMENT_MAX_VISIT_DATE_BASE( __place_relation, "moz_historyvisits") ")"



#define PRIVATEBROWSING_NOTINITED (PRBool(0xffffffff))

#define PLACES_INIT_COMPLETE_EVENT_TOPIC "places-init-complete"
#define PLACES_DB_LOCKED_EVENT_TOPIC "places-database-locked"

struct AutoCompleteIntermediateResult;
class AutoCompleteResultComparator;
class mozIAnnotationService;
class nsNavHistory;
class nsNavBookmarks;
class QueryKeyValuePair;
class nsIEffectiveTLDService;
class nsIIDNService;
class PlacesSQLQueryBuilder;



class nsNavHistory : public nsSupportsWeakReference
                   , public nsINavHistoryService
                   , public nsIObserver
                   , public nsIBrowserHistory
                   , public nsIGlobalHistory3
                   , public nsIDownloadHistory
                   , public nsICharsetResolver
                   , public nsPIPlacesDatabase
                   , public nsPIPlacesHistoryListenersNotifier
#ifdef MOZ_XUL
                   , public nsIAutoCompleteSearch
                   , public nsIAutoCompleteSimpleResultListener
#endif
{
  friend class AutoCompleteIntermediateResultSet;
  friend class AutoCompleteResultComparator;
  friend class PlacesSQLQueryBuilder;

public:
  nsNavHistory();

  NS_DECL_ISUPPORTS

  NS_DECL_NSINAVHISTORYSERVICE
  NS_DECL_NSIGLOBALHISTORY2
  NS_DECL_NSIGLOBALHISTORY3
  NS_DECL_NSIDOWNLOADHISTORY
  NS_DECL_NSIBROWSERHISTORY
  NS_DECL_NSIOBSERVER
  NS_DECL_NSPIPLACESDATABASE
  NS_DECL_NSPIPLACESHISTORYLISTENERSNOTIFIER
#ifdef MOZ_XUL
  NS_DECL_NSIAUTOCOMPLETESEARCH
  NS_DECL_NSIAUTOCOMPLETESIMPLERESULTLISTENER
#endif


  


  static nsNavHistory *GetSingleton();

  


  nsresult Init();

  




  static nsNavHistory* GetHistoryService()
  {
    if (gHistoryService)
      return gHistoryService;

    nsCOMPtr<nsINavHistoryService> serv =
      do_GetService("@mozilla.org/browser/nav-history-service;1");
    NS_ENSURE_TRUE(serv, nsnull);

    return gHistoryService;
  }

#ifdef LAZY_ADD
  



  nsresult AddLazyLoadFaviconMessage(nsIURI* aPage, nsIURI* aFavicon,
                                     PRBool aForceReload);
#endif

  



  nsresult GetUrlIdFor(nsIURI* aURI, PRInt64* aEntryID,
                       PRBool aAutoCreate);

  nsresult CalculateFullVisitCount(PRInt64 aPlaceId, PRInt32 *aVisitCount);

  nsresult UpdateFrecency(PRInt64 aPageID, PRBool isBookmark);

  nsresult FixInvalidFrecenciesForExcludedPlaces();

  









  mozIStorageConnection* GetStorageConnection()
  {
    return mDBConn;
  }

  



  nsIStringBundle* GetBundle();
  nsIStringBundle* GetDateFormatBundle();
  nsICollation* GetCollation();
  void GetStringFromName(const PRUnichar* aName, nsACString& aResult);
  void GetAgeInDaysString(PRInt32 aInt, const PRUnichar *aName,
                          nsACString& aResult);
  void GetMonthName(PRInt32 aIndex, nsACString& aResult);

  
  PRBool IsHistoryDisabled() { return mExpireDaysMax == 0 || InPrivateBrowsingMode(); }

  
  mozIStorageStatement* DBGetURLPageInfo() { return mDBGetURLPageInfo; }

  
  static const PRInt32 kGetInfoIndex_PageID;
  static const PRInt32 kGetInfoIndex_URL;
  static const PRInt32 kGetInfoIndex_Title;
  static const PRInt32 kGetInfoIndex_RevHost;
  static const PRInt32 kGetInfoIndex_VisitCount;
  static const PRInt32 kGetInfoIndex_ItemId;
  static const PRInt32 kGetInfoIndex_ItemDateAdded;
  static const PRInt32 kGetInfoIndex_ItemLastModified;

  
  mozIStorageStatement* DBGetIdPageInfo() { return mDBGetIdPageInfo; }

  mozIStorageStatement* DBGetTags() { return mDBGetTags; }
  PRInt64 GetTagsFolder();

  
  
  static const PRInt32 kGetInfoIndex_VisitDate;
  static const PRInt32 kGetInfoIndex_FaviconURL;

  
  static const PRInt32 kGetInfoIndex_SessionId;

  
  
  nsresult GetQueryResults(nsNavHistoryQueryResultNode *aResultNode,
                           const nsCOMArray<nsNavHistoryQuery>& aQueries,
                           nsNavHistoryQueryOptions *aOptions,
                           nsCOMArray<nsNavHistoryResultNode>* aResults);

  
  
  nsresult RowToResult(mozIStorageValueArray* aRow,
                       nsNavHistoryQueryOptions* aOptions,
                       nsNavHistoryResultNode** aResult);
  nsresult QueryRowToResult(PRInt64 aItemId, const nsACString& aURI,
                            const nsACString& aTitle,
                            PRUint32 aAccessCount, PRTime aTime,
                            const nsACString& aFavicon,
                            nsNavHistoryResultNode** aNode);

  nsresult VisitIdToResultNode(PRInt64 visitId,
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
  void DomainNameFromURI(nsIURI* aURI,
                         nsACString& aDomainName);
  static PRTime NormalizeTime(PRUint32 aRelative, PRTime aOffset);

  
  
  nsresult BeginUpdateBatch();
  nsresult EndUpdateBatch();

  
  PRInt32 mBatchLevel;

  
  
  PRBool mBatchHasTransaction;

  
  nsresult QueryStringToQueryArray(const nsACString& aQueryString,
                                   nsCOMArray<nsNavHistoryQuery>* aQueries,
                                   nsNavHistoryQueryOptions** aOptions);

  
  
  
  
  
  
  nsresult AddPageWithVisits(nsIURI *aURI,
                             const nsString &aTitle,
                             PRInt32 aVisitCount,
                             PRInt32 aTransitionType,
                             PRTime aFirstVisitDate,
                             PRTime aLastVisitDate);

  
  
  
  nsresult RemoveDuplicateURIs();

  
  nsresult UpdateSchemaVersion();

  
  PRBool InPrivateBrowsingMode()
  {
    if (mInPrivateBrowsing == PRIVATEBROWSING_NOTINITED) {
      mInPrivateBrowsing = PR_FALSE;
      nsCOMPtr<nsIPrivateBrowsingService> pbs =
        do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
      if (pbs) {
        pbs->GetPrivateBrowsingEnabled(&mInPrivateBrowsing);
      }
    }

    return mInPrivateBrowsing;
  }

  typedef nsDataHashtable<nsCStringHashKey, nsCString> StringHash;

  


  static nsresult
  FinalizeStatement(mozIStorageStatement *aStatement) {
    nsresult rv;
    if (aStatement) {
      rv = aStatement->Finalize();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }

 private:
  ~nsNavHistory();

  
  static nsNavHistory* gHistoryService;

protected:

  
  
  
  nsCOMPtr<nsIPrefBranch> mPrefBranch; 
  nsDataHashtable<nsStringHashKey, int> gExpandedItems;

  
  
  
  nsCOMPtr<mozIStorageService> mDBService;
  nsCOMPtr<mozIStorageConnection> mDBConn;
  nsCOMPtr<nsIFile> mDBFile;

  nsCOMPtr<mozIStorageStatement> mDBGetURLPageInfo;   
  nsCOMPtr<mozIStorageStatement> mDBGetIdPageInfo;     

  nsCOMPtr<mozIStorageStatement> mDBRecentVisitOfURL; 
  nsCOMPtr<mozIStorageStatement> mDBRecentVisitOfPlace; 
  nsCOMPtr<mozIStorageStatement> mDBInsertVisit; 
  nsCOMPtr<mozIStorageStatement> mDBGetPageVisitStats; 
  nsCOMPtr<mozIStorageStatement> mDBIsPageVisited; 
  nsCOMPtr<mozIStorageStatement> mDBUpdatePageVisitStats; 
  nsCOMPtr<mozIStorageStatement> mDBAddNewPage; 
  nsCOMPtr<mozIStorageStatement> mDBGetTags; 
  nsCOMPtr<mozIStorageStatement> mFoldersWithAnnotationQuery;  
  nsCOMPtr<mozIStorageStatement> mDBSetPlaceTitle; 

  
  
  mozIStorageStatement *GetDBVisitToURLResult();
  nsCOMPtr<mozIStorageStatement> mDBVisitToURLResult; 
  mozIStorageStatement *GetDBVisitToVisitResult();
  nsCOMPtr<mozIStorageStatement> mDBVisitToVisitResult; 
  mozIStorageStatement *GetDBBookmarkToUrlResult();
  nsCOMPtr<mozIStorageStatement> mDBBookmarkToUrlResult; 

  


  nsresult FinalizeStatements();

  
  NS_DECL_NSICHARSETRESOLVER

  








  nsresult RecalculateFrecencies(PRInt32 aCount, PRBool aRecalcOld);
  nsresult RecalculateFrecenciesInternal(mozIStorageStatement *aStatement, PRInt32 aCount);

  nsresult CalculateFrecency(PRInt64 aPageID, PRInt32 aTyped, PRInt32 aVisitCount, nsCAutoString &aURL, PRInt32 *aFrecency);
  nsresult CalculateFrecencyInternal(PRInt64 aPageID, PRInt32 aTyped, PRInt32 aVisitCount, PRBool aIsBookmarked, PRInt32 *aFrecency);
  nsCOMPtr<mozIStorageStatement> mDBVisitsForFrecency;
  nsCOMPtr<mozIStorageStatement> mDBUpdateFrecencyAndHidden;
  nsCOMPtr<mozIStorageStatement> mDBGetPlaceVisitStats;
  nsCOMPtr<mozIStorageStatement> mDBFullVisitCount;
  mozIStorageStatement *GetDBInvalidFrecencies();
  nsCOMPtr<mozIStorageStatement> mDBInvalidFrecencies;
  mozIStorageStatement *GetDBOldFrecencies();
  nsCOMPtr<mozIStorageStatement> mDBOldFrecencies;

  








  nsresult InitDBFile(PRBool aForceInit);

  





  nsresult InitDB();
  nsresult InitTempTables();
  nsresult InitViews();
  nsresult InitFunctions();
  nsresult InitStatements();
  nsresult ForceMigrateBookmarksDB(mozIStorageConnection *aDBConn);
  nsresult MigrateV3Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV6Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV7Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV8Up(mozIStorageConnection *aDBConn);

  nsresult RemovePagesInternal(const nsCString& aPlaceIdsQueryString);

  nsresult AddURIInternal(nsIURI* aURI, PRTime aTime, PRBool aRedirect,
                          PRBool aToplevel, nsIURI* aReferrer);

  nsresult AddVisitChain(nsIURI* aURI, PRTime aTime,
                         PRBool aToplevel, PRBool aRedirect,
                         nsIURI* aReferrer, PRInt64* aVisitID,
                         PRInt64* aSessionID, PRInt64* aRedirectBookmark);
  nsresult InternalAddNewPage(nsIURI* aURI, const nsAString& aTitle,
                              PRBool aHidden, PRBool aTyped,
                              PRInt32 aVisitCount, PRBool aCalculateFrecency,
                              PRInt64* aPageID);
  nsresult InternalAddVisit(PRInt64 aPageID, PRInt64 aReferringVisit,
                            PRInt64 aSessionID, PRTime aTime,
                            PRInt32 aTransitionType, PRInt64* aVisitID);
  PRBool FindLastVisit(nsIURI* aURI, PRInt64* aVisitID,
                       PRInt64* aSessionID);
  PRBool IsURIStringVisited(const nsACString& url);

  






  nsresult LoadPrefs(PRBool aInitializing);

  
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
      NS_ENSURE_ARG_POINTER(aURI);
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

  nsresult ConstructQueryString(const nsCOMArray<nsNavHistoryQuery>& aQueries, 
                                nsNavHistoryQueryOptions* aOptions,
                                nsCString& queryString,
                                PRBool& aParamsPresent,
                                StringHash& aAddParams);

  nsresult QueryToSelectClause(nsNavHistoryQuery* aQuery,
                               nsNavHistoryQueryOptions* aOptions,
                               PRInt32 aQueryIndex,
                               nsCString* aClause);
  nsresult BindQueryClauseParameters(mozIStorageStatement* statement,
                                     PRInt32 aQueryIndex,
                                     nsNavHistoryQuery* aQuery,
                                     nsNavHistoryQueryOptions* aOptions);

  nsresult ResultsAsList(mozIStorageStatement* statement,
                         nsNavHistoryQueryOptions* aOptions,
                         nsCOMArray<nsNavHistoryResultNode>* aResults);

  void TitleForDomain(const nsCString& domain, nsACString& aTitle);

  nsresult SetPageTitleInternal(nsIURI* aURI, const nsAString& aTitle);

  nsresult FilterResultSet(nsNavHistoryQueryResultNode *aParentNode,
                           const nsCOMArray<nsNavHistoryResultNode>& aSet,
                           nsCOMArray<nsNavHistoryResultNode>* aFiltered,
                           const nsCOMArray<nsNavHistoryQuery>& aQueries,
                           nsNavHistoryQueryOptions* aOptions);

  
  nsMaybeWeakPtrArray<nsINavHistoryObserver> mObservers;

  
  nsCOMPtr<nsIEffectiveTLDService> mTLDService;
  nsCOMPtr<nsIIDNService>          mIDNService;

  
  nsCOMPtr<nsIStringBundle> mBundle;
  nsCOMPtr<nsIStringBundle> mDateFormatBundle;
  nsCOMPtr<nsICollation> mCollation;

  
  

  
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
  static PLDHashOperator ExpireNonrecentRedirects(
      nsCStringHashKey::KeyType aKey, RedirectInfo& aData, void* aUserArg);
  PRBool GetRedirectFor(const nsACString& aDestination, nsACString& aSource,
                        PRTime* aTime, PRUint32* aRedirectType);

  
  PRInt64 mLastSessionID;
  PRInt64 GetNewSessionID() { mLastSessionID ++; return mLastSessionID; }

  
  
  
  static const PRInt32 kAutoCompleteIndex_URL;
  static const PRInt32 kAutoCompleteIndex_Title;
  static const PRInt32 kAutoCompleteIndex_FaviconURL;
  static const PRInt32 kAutoCompleteIndex_ParentId;
  static const PRInt32 kAutoCompleteIndex_BookmarkTitle;
  static const PRInt32 kAutoCompleteIndex_Tags;
  static const PRInt32 kAutoCompleteIndex_VisitCount;
  static const PRInt32 kAutoCompleteIndex_Typed;
  nsCOMPtr<mozIStorageStatement> mDBCurrentQuery; 
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteQuery; 
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteTypedQuery; 
  mozIStorageStatement* GetDBAutoCompleteHistoryQuery();
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteHistoryQuery; 
  mozIStorageStatement* GetDBAutoCompleteStarQuery();
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteStarQuery; 
  mozIStorageStatement* GetDBAutoCompleteTagsQuery();
  nsCOMPtr<mozIStorageStatement> mDBAutoCompleteTagsQuery; 
  nsCOMPtr<mozIStorageStatement> mDBPreviousQuery; 
  nsCOMPtr<mozIStorageStatement> mDBAdaptiveQuery; 
  nsCOMPtr<mozIStorageStatement> mDBKeywordQuery; 
  mozIStorageStatement* GetDBFeedbackIncrease();
  nsCOMPtr<mozIStorageStatement> mDBFeedbackIncrease;

  



  enum MatchType {
    MATCH_ANYWHERE,
    MATCH_BOUNDARY_ANYWHERE,
    MATCH_BOUNDARY,
    MATCH_BEGINNING
  };

  nsresult InitAutoComplete();
  nsresult CreateAutoCompleteQueries();
  PRBool mAutoCompleteEnabled;
  MatchType mAutoCompleteMatchBehavior;
  PRBool mAutoCompleteFilterJavascript;
  PRInt32 mAutoCompleteMaxResults;
  nsString mAutoCompleteRestrictHistory;
  nsString mAutoCompleteRestrictBookmark;
  nsString mAutoCompleteRestrictTag;
  nsString mAutoCompleteMatchTitle;
  nsString mAutoCompleteMatchUrl;
  nsString mAutoCompleteRestrictTyped;
  PRInt32 mAutoCompleteSearchChunkSize;
  PRInt32 mAutoCompleteSearchTimeout;
  nsCOMPtr<nsITimer> mAutoCompleteTimer;

  static const PRInt32 kAutoCompleteBehaviorHistory;
  static const PRInt32 kAutoCompleteBehaviorBookmark;
  static const PRInt32 kAutoCompleteBehaviorTag;
  static const PRInt32 kAutoCompleteBehaviorTitle;
  static const PRInt32 kAutoCompleteBehaviorUrl;
  static const PRInt32 kAutoCompleteBehaviorTyped;

  PRInt32 mAutoCompleteDefaultBehavior; 
  PRInt32 mAutoCompleteCurrentBehavior; 

  
  nsString mOrigSearchString;
  
  nsString mCurrentSearchString;
  nsTArray<nsString> mCurrentSearchTokens;
  void GenerateSearchTokens();
  void AddSearchToken(nsAutoString &aToken);
  void ProcessTokensForSpecialSearch();

#ifdef MOZ_XUL
  nsresult AutoCompleteFeedback(PRInt32 aIndex,
                                nsIAutoCompleteController *aController);

  nsCOMPtr<nsIAutoCompleteObserver> mCurrentListener;
  nsCOMPtr<nsIAutoCompleteSimpleResult> mCurrentResult;
#endif

  MatchType mCurrentMatchType;
  MatchType mPreviousMatchType;
  nsDataHashtable<nsStringHashKey, PRBool> mCurrentResultURLs;
  PRInt32 mCurrentChunkOffset;
  PRInt32 mPreviousChunkOffset;

  nsDataHashtable<nsTrimInt64HashKey, PRBool> mLivemarkFeedItemIds;
  nsDataHashtable<nsStringHashKey, PRBool> mLivemarkFeedURIs;

  nsresult AutoCompleteFullHistorySearch(PRBool* aHasMoreResults);
  nsresult AutoCompletePreviousSearch();
  nsresult AutoCompleteAdaptiveSearch();
  nsresult AutoCompleteKeywordSearch();

  



  enum QueryType {
    QUERY_KEYWORD,
    QUERY_FILTERED
  };
  nsresult AutoCompleteProcessSearch(mozIStorageStatement* aQuery,
                                     const QueryType aType,
                                     PRBool *aHasMoreResults = nsnull);
  PRBool AutoCompleteHasEnoughResults();

  nsresult PerformAutoComplete();
  nsresult StartAutoCompleteTimer(PRUint32 aMilliseconds);
  static void AutoCompleteTimerCallback(nsITimer* aTimer, void* aClosure);

  PRBool mAutoCompleteFinishedSearch;
  void DoneSearching(PRBool aFinished);

  
  nsCOMPtr<nsITextToSubURI> mTextURIService;
  nsString FixupURIText(const nsAString &aURIText);

  PRInt32 mExpireDaysMin;
  PRInt32 mExpireDaysMax;
  PRInt32 mExpireSites;

  
  PRInt32 mNumVisitsForFrecency;
  PRInt32 mNumCalculateFrecencyOnIdle;
  PRInt32 mNumCalculateFrecencyOnMigrate;
  PRInt32 mFrecencyUpdateIdleTime;
  PRInt32 mFirstBucketCutoffInDays;
  PRInt32 mSecondBucketCutoffInDays;
  PRInt32 mThirdBucketCutoffInDays;
  PRInt32 mFourthBucketCutoffInDays;
  PRInt32 mFirstBucketWeight;
  PRInt32 mSecondBucketWeight;
  PRInt32 mThirdBucketWeight;
  PRInt32 mFourthBucketWeight;
  PRInt32 mDefaultWeight;
  PRInt32 mEmbedVisitBonus;
  PRInt32 mLinkVisitBonus;
  PRInt32 mTypedVisitBonus;
  PRInt32 mBookmarkVisitBonus;
  PRInt32 mDownloadVisitBonus;
  PRInt32 mPermRedirectVisitBonus;
  PRInt32 mTempRedirectVisitBonus;
  PRInt32 mDefaultVisitBonus;
  PRInt32 mUnvisitedBookmarkBonus;
  PRInt32 mUnvisitedTypedBonus;

  
  nsresult TokensToQueries(const nsTArray<QueryKeyValuePair>& aTokens,
                           nsCOMArray<nsNavHistoryQuery>* aQueries,
                           nsNavHistoryQueryOptions* aOptions);

  



  nsCOMPtr<nsITimer> mIdleTimer;
  nsresult InitializeIdleTimer();
  static void IdleTimerCallback(nsITimer* aTimer, void* aClosure);
  nsresult OnIdle();

  PRInt64 mTagsFolder;

  PRBool mInPrivateBrowsing;

  PRUint16 mDatabaseStatus;
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
