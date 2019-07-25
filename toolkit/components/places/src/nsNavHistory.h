








































#ifndef nsNavHistory_h_
#define nsNavHistory_h_

#include "nsINavHistoryService.h"
#include "nsPIPlacesDatabase.h"
#include "nsPIPlacesHistoryListenersNotifier.h"
#include "nsIBrowserHistory.h"
#include "nsIGlobalHistory.h"
#include "nsIGlobalHistory3.h"
#include "nsIDownloadHistory.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIObserverService.h"
#include "nsICollation.h"
#include "nsIStringBundle.h"
#include "nsITimer.h"
#include "nsMaybeWeakPtr.h"
#include "nsCategoryCache.h"
#include "nsICharsetResolver.h"
#include "nsNetCID.h"
#include "nsToolkitCompsCID.h"
#include "nsThreadUtils.h"

#include "nsINavBookmarksService.h"
#include "nsIPrivateBrowsingService.h"
#include "nsIFaviconService.h"
#include "nsNavHistoryResult.h"
#include "nsNavHistoryQuery.h"

#include "mozilla/storage.h"

#define QUERYUPDATE_TIME 0
#define QUERYUPDATE_SIMPLE 1
#define QUERYUPDATE_COMPLEX 2
#define QUERYUPDATE_COMPLEX_WITH_BOOKMARKS 3
#define QUERYUPDATE_HOST 4



#define PRIVATEBROWSING_NOTINITED (PRBool(0xffffffff))



#define URI_LENGTH_MAX 65536
#define TITLE_LENGTH_MAX 4096



#define RECENT_EVENT_THRESHOLD PRTime((PRInt64)15 * 60 * PR_USEC_PER_SEC)

#ifdef MOZ_XUL

#define TOPIC_AUTOCOMPLETE_FEEDBACK_UPDATED "places-autocomplete-feedback-updated"
#endif

#define TOPIC_PLACES_SHUTDOWN "places-shutdown"



#define TOPIC_PLACES_TEARDOWN "places-teardown"

#define TOPIC_DATABASE_LOCKED "places-database-locked"

#define TOPIC_PLACES_INIT_COMPLETE "places-init-complete"

#define TOPIC_DATABASE_VACUUM_STARTING "places-vacuum-starting"

namespace mozilla {
namespace places {

  enum HistoryStatementId {
    DB_GET_PAGE_INFO_BY_URL = 0
  , DB_GET_TAGS = 1
  , DB_IS_PAGE_VISITED = 2
  , DB_INSERT_VISIT = 3
  , DB_RECENT_VISIT_OF_URL = 4
  , DB_GET_PAGE_VISIT_STATS = 5
  , DB_UPDATE_PAGE_VISIT_STATS = 6
  , DB_ADD_NEW_PAGE = 7
  , DB_GET_URL_PAGE_INFO = 8
  , DB_SET_PLACE_TITLE = 9
  };

} 
} 


class mozIAnnotationService;
class nsNavHistory;
class nsNavBookmarks;
class QueryKeyValuePair;
class nsIEffectiveTLDService;
class nsIIDNService;
class PlacesSQLQueryBuilder;
class nsIAutoCompleteController;



class nsNavHistory : public nsSupportsWeakReference
                   , public nsINavHistoryService
                   , public nsIObserver
                   , public nsIBrowserHistory
                   , public nsIGlobalHistory3
                   , public nsIDownloadHistory
                   , public nsICharsetResolver
                   , public nsPIPlacesDatabase
                   , public nsPIPlacesHistoryListenersNotifier
{
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


  


  static nsNavHistory *GetSingleton();

  


  nsresult Init();

  




  static nsNavHistory *GetHistoryService()
  {
    if (!gHistoryService) {
      nsCOMPtr<nsINavHistoryService> serv =
        do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID);
      NS_ENSURE_TRUE(serv, nsnull);
      NS_ASSERTION(gHistoryService, "Should have static instance pointer now");
    }
    return gHistoryService;
  }

  



  nsresult GetUrlIdFor(nsIURI* aURI, PRInt64* aEntryID,
                       PRBool aAutoCreate);

  nsresult CalculateFullVisitCount(PRInt64 aPlaceId, PRInt32 *aVisitCount);

  nsresult UpdateFrecency(PRInt64 aPlaceId, PRBool aIsBookmark);
  nsresult UpdateFrecencyInternal(PRInt64 aPlaceId, PRInt32 aTyped,
                                  PRInt32 aHidden, PRInt32 aOldFrecency,
                                  PRBool aIsBookmark);

  


  nsresult FixInvalidFrecencies();

  


  nsresult FixInvalidFrecenciesForExcludedPlaces();

  









  mozIStorageConnection* GetStorageConnection()
  {
    return mDBConn;
  }

  



  void SetVacuumInProgress(bool aValue)
  {
    NS_PRECONDITION(NS_IsMainThread(), "SetVacuumInProgress() off main thread!");
    mVacuumInProgress = aValue;
  }

  



  nsIStringBundle* GetBundle();
  nsIStringBundle* GetDateFormatBundle();
  nsICollation* GetCollation();
  void GetStringFromName(const PRUnichar* aName, nsACString& aResult);
  void GetAgeInDaysString(PRInt32 aInt, const PRUnichar *aName,
                          nsACString& aResult);
  void GetMonthName(PRInt32 aIndex, nsACString& aResult);

  
  PRBool IsHistoryDisabled() {
    return !mHistoryEnabled || InPrivateBrowsingMode();
  }

  
  static const PRInt32 kGetInfoIndex_PageID;
  static const PRInt32 kGetInfoIndex_URL;
  static const PRInt32 kGetInfoIndex_Title;
  static const PRInt32 kGetInfoIndex_RevHost;
  static const PRInt32 kGetInfoIndex_VisitCount;
  static const PRInt32 kGetInfoIndex_ItemId;
  static const PRInt32 kGetInfoIndex_ItemDateAdded;
  static const PRInt32 kGetInfoIndex_ItemLastModified;
  static const PRInt32 kGetInfoIndex_ItemTags;
  static const PRInt32 kGetInfoIndex_ItemParentId;

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
                                   const nsAString& aValue);

  


  PRInt32 GetDaysOfHistory();

  
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
  
  mozStorageTransaction* mBatchDBTransaction;

  
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

  




  bool canNotify() { return mCanNotify; }

  enum RecentEventFlags {
    RECENT_TYPED      = 1 << 0,    
    RECENT_ACTIVATED  = 1 << 1,    
    RECENT_BOOKMARKED = 1 << 2     
  };

  




  PRUint32 GetRecentFlags(nsIURI *aURI);

  mozIStorageStatement* GetStatementById(
    enum mozilla::places::HistoryStatementId aStatementId
  )
  {
    using namespace mozilla::places;
    switch(aStatementId) {
      case DB_GET_PAGE_INFO_BY_URL:
        return mDBGetURLPageInfo;
      case DB_GET_TAGS:
        return mDBGetTags;
      case DB_IS_PAGE_VISITED:
        return mDBIsPageVisited;
      case DB_INSERT_VISIT:
        return mDBInsertVisit;
      case DB_RECENT_VISIT_OF_URL:
        return mDBRecentVisitOfURL;
      case DB_GET_PAGE_VISIT_STATS:
        return mDBGetPageVisitStats;
      case DB_UPDATE_PAGE_VISIT_STATS:
        return mDBUpdatePageVisitStats;
      case DB_ADD_NEW_PAGE:
        return mDBAddNewPage;
      case DB_GET_URL_PAGE_INFO:
        return mDBGetURLPageInfo;
      case DB_SET_PLACE_TITLE:
        return mDBSetPlaceTitle;
    }
    return nsnull;
  }

  PRInt64 GetNewSessionID();

  


  void NotifyOnVisit(nsIURI* aURI,
                     PRInt64 aVisitID,
                     PRTime aTime,
                     PRInt64 aSessionID,
                     PRInt64 referringVisitID,
                     PRInt32 aTransitionType);

  


  void NotifyTitleChange(nsIURI* aURI, const nsString& title);

private:
  ~nsNavHistory();

  
  static nsNavHistory *gHistoryService;

protected:

  nsCOMPtr<nsIPrefBranch2> mPrefBranch; 

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
  nsCOMPtr<mozIStorageStatement> mDBGetItemsWithAnno; 
  nsCOMPtr<mozIStorageStatement> mDBSetPlaceTitle; 
  nsCOMPtr<mozIStorageStatement> mDBRegisterOpenPage; 
  nsCOMPtr<mozIStorageStatement> mDBUnregisterOpenPage; 

  
  
  mozIStorageStatement *GetDBVisitToURLResult();
  nsCOMPtr<mozIStorageStatement> mDBVisitToURLResult; 
  mozIStorageStatement *GetDBVisitToVisitResult();
  nsCOMPtr<mozIStorageStatement> mDBVisitToVisitResult; 
  mozIStorageStatement *GetDBBookmarkToUrlResult();
  nsCOMPtr<mozIStorageStatement> mDBBookmarkToUrlResult; 

  


  nsresult FinalizeStatements();

  


  nsresult DecayFrecency();
  


  nsresult VacuumDatabase();

  



  nsresult FinalizeInternalStatements();

  
  NS_DECL_NSICHARSETRESOLVER

  nsresult CalculateFrecency(PRInt64 aPageID, PRInt32 aTyped, PRInt32 aVisitCount, nsCAutoString &aURL, PRInt32 *aFrecency);
  nsresult CalculateFrecencyInternal(PRInt64 aPageID, PRInt32 aTyped, PRInt32 aVisitCount, PRBool aIsBookmarked, PRInt32 *aFrecency);
  nsCOMPtr<mozIStorageStatement> mDBVisitsForFrecency;
  nsCOMPtr<mozIStorageStatement> mDBUpdateFrecencyAndHidden;
  nsCOMPtr<mozIStorageStatement> mDBGetPlaceVisitStats;
  nsCOMPtr<mozIStorageStatement> mDBFullVisitCount;

  








  nsresult InitDBFile(PRBool aForceInit);

  





  nsresult InitDB();

  



  nsresult InitAdditionalDBItems();
  nsresult InitTempTables();
  nsresult InitViews();
  nsresult InitFunctions();
  nsresult InitStatements();
  nsresult ForceMigrateBookmarksDB(mozIStorageConnection *aDBConn);
  nsresult MigrateV3Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV6Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV7Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV8Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV9Up(mozIStorageConnection *aDBConn);
  nsresult MigrateV10Up(mozIStorageConnection *aDBConn);

  nsresult RemovePagesInternal(const nsCString& aPlaceIdsQueryString);
  nsresult PreparePlacesForVisitsDelete(const nsCString& aPlaceIdsQueryString);
  nsresult CleanupPlacesOnVisitsDelete(const nsCString& aPlaceIdsQueryString);

  nsresult AddURIInternal(nsIURI* aURI, PRTime aTime, PRBool aRedirect,
                          PRBool aToplevel, nsIURI* aReferrer);

  nsresult AddVisitChain(nsIURI* aURI, PRTime aTime,
                         PRBool aToplevel, PRBool aRedirect,
                         nsIURI* aReferrer, PRInt64* aVisitID,
                         PRInt64* aSessionID);
  nsresult InternalAddNewPage(nsIURI* aURI, const nsAString& aTitle,
                              PRBool aHidden, PRBool aTyped,
                              PRInt32 aVisitCount, PRBool aCalculateFrecency,
                              PRInt64* aPageID);
  nsresult InternalAddVisit(PRInt64 aPageID, PRInt64 aReferringVisit,
                            PRInt64 aSessionID, PRTime aTime,
                            PRInt32 aTransitionType, PRInt64* aVisitID);
  PRBool FindLastVisit(nsIURI* aURI,
                       PRInt64* aVisitID,
                       PRTime* aTime,
                       PRInt64* aSessionID);
  PRBool IsURIStringVisited(const nsACString& url);

  




  void LoadPrefs();

  





  PRTime GetNow();
  PRTime mCachedNow;
  nsCOMPtr<nsITimer> mExpireNowTimer;
  


  static void expireNowTimerCallback(nsITimer* aTimer, void* aClosure);

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
  RecentEventHash mRecentLink;
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

#ifdef MOZ_XUL
  
  mozIStorageStatement *GetDBFeedbackIncrease();
  nsCOMPtr<mozIStorageStatement> mDBFeedbackIncrease;

  nsresult AutoCompleteFeedback(PRInt32 aIndex,
                                nsIAutoCompleteController *aController);
#endif

  
  
  PRBool mHistoryEnabled;

  
  PRInt32 mNumVisitsForFrecency;
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
  PRInt32 mFramedLinkVisitBonus;
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

  PRInt64 mTagsFolder;

  PRBool mInPrivateBrowsing;

  PRUint16 mDatabaseStatus;

  PRInt8 mHasHistoryEntries;

  
  bool mCanNotify;
  nsCategoryCache<nsINavHistoryObserver> mCacheObservers;

  
  nsCOMPtr<mozIStorageStatementCallback> mVacuumDBListener;
  bool mVacuumInProgress;
};


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
