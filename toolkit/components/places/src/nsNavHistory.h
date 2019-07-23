








































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
#include "nsIObserverService.h"
#include "nsICollation.h"
#include "nsIStringBundle.h"
#include "nsITimer.h"
#include "nsMaybeWeakPtr.h"
#include "nsCategoryCache.h"
#include "nsICharsetResolver.h"
#include "nsNetCID.h"
#include "nsToolkitCompsCID.h"

#include "nsINavBookmarksService.h"
#include "nsIPrivateBrowsingService.h"

#include "nsNavHistoryExpire.h"
#include "nsNavHistoryResult.h"
#include "nsNavHistoryQuery.h"

#include "mozilla/storage.h"


#define LAZY_ADD

#define QUERYUPDATE_TIME 0
#define QUERYUPDATE_SIMPLE 1
#define QUERYUPDATE_COMPLEX 2
#define QUERYUPDATE_COMPLEX_WITH_BOOKMARKS 3
#define QUERYUPDATE_HOST 4



#define PRIVATEBROWSING_NOTINITED (PRBool(0xffffffff))

#define PLACES_INIT_COMPLETE_TOPIC "places-init-complete"
#define PLACES_DB_LOCKED_TOPIC "places-database-locked"
#define PLACES_AUTOCOMPLETE_FEEDBACK_UPDATED_TOPIC "places-autocomplete-feedback-updated"
#define PLACES_VACUUM_STARTING_TOPIC "places-vacuum-starting"

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

#ifdef LAZY_ADD
  



  nsresult AddLazyLoadFaviconMessage(nsIURI* aPage, nsIURI* aFavicon,
                                     PRBool aForceReload);
#endif

  



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

  



  nsIStringBundle* GetBundle();
  nsIStringBundle* GetDateFormatBundle();
  nsICollation* GetCollation();
  void GetStringFromName(const PRUnichar* aName, nsACString& aResult);
  void GetAgeInDaysString(PRInt32 aInt, const PRUnichar *aName,
                          nsACString& aResult);
  void GetMonthName(PRInt32 aIndex, nsACString& aResult);

  
  PRBool IsHistoryDisabled() { return mExpireDaysMax == 0 || InPrivateBrowsingMode(); }

  
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

  
  mozIStorageStatement *DBGetIdPageInfo() { return mDBGetIdPageInfo; }

  mozIStorageStatement *DBGetTags() { return mDBGetTags; }
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

  




  bool canNotify() { return mCanNotify; }
 private:
  ~nsNavHistory();

  
  static nsNavHistory *gHistoryService;

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
  nsCOMPtr<mozIStorageStatement> mDBGetItemsWithAnno; 
  nsCOMPtr<mozIStorageStatement> mDBSetPlaceTitle; 

  
  
  mozIStorageStatement *GetDBVisitToURLResult();
  nsCOMPtr<mozIStorageStatement> mDBVisitToURLResult; 
  mozIStorageStatement *GetDBVisitToVisitResult();
  nsCOMPtr<mozIStorageStatement> mDBVisitToVisitResult; 
  mozIStorageStatement *GetDBBookmarkToUrlResult();
  nsCOMPtr<mozIStorageStatement> mDBBookmarkToUrlResult; 

  


  NS_HIDDEN_(nsresult) FinalizeStatements();

  


  NS_HIDDEN_(nsresult) DecayFrecency();
  


  NS_HIDDEN_(nsresult) VacuumDatabase();

  



  NS_HIDDEN_(nsresult) FinalizeInternalStatements();

  
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
  nsNavHistoryExpire *mExpire;

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
  NS_HIDDEN_(void) CommitLazyMessages(PRBool aIsShutdown = PR_FALSE);
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

#ifdef MOZ_XUL
  
  mozIStorageStatement *GetDBFeedbackIncrease();
  nsCOMPtr<mozIStorageStatement> mDBFeedbackIncrease;

  nsresult AutoCompleteFeedback(PRInt32 aIndex,
                                nsIAutoCompleteController *aController);
#endif

  PRInt32 mExpireDaysMin;
  PRInt32 mExpireDaysMax;
  PRInt32 mExpireSites;

  
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

  
  bool mCanNotify;
  nsCategoryCache<nsINavHistoryObserver> mCacheObservers;
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
