




#ifndef nsNavHistory_h_
#define nsNavHistory_h_

#include "nsINavHistoryService.h"
#include "nsPIPlacesDatabase.h"
#include "nsIBrowserHistory.h"
#include "nsINavBookmarksService.h"
#include "nsIFaviconService.h"

#include "nsIObserverService.h"
#include "nsICollation.h"
#include "nsIStringBundle.h"
#include "nsITimer.h"
#include "nsMaybeWeakPtr.h"
#include "nsCategoryCache.h"
#include "nsNetCID.h"
#include "nsToolkitCompsCID.h"
#include "nsURIHashKey.h"
#include "nsTHashtable.h"

#include "nsNavHistoryResult.h"
#include "nsNavHistoryQuery.h"
#include "Database.h"
#include "mozilla/Attributes.h"

#define QUERYUPDATE_TIME 0
#define QUERYUPDATE_SIMPLE 1
#define QUERYUPDATE_COMPLEX 2
#define QUERYUPDATE_COMPLEX_WITH_BOOKMARKS 3
#define QUERYUPDATE_HOST 4



#define URI_LENGTH_MAX 65536
#define TITLE_LENGTH_MAX 4096



#define RECENT_EVENT_THRESHOLD PRTime((int64_t)15 * 60 * PR_USEC_PER_SEC)

#ifdef MOZ_XUL

#define TOPIC_AUTOCOMPLETE_FEEDBACK_UPDATED "places-autocomplete-feedback-updated"
#endif


#define TOPIC_FRECENCY_UPDATED "places-frecency-updated"

class nsNavHistory;
class QueryKeyValuePair;
class nsIEffectiveTLDService;
class nsIIDNService;
class PlacesSQLQueryBuilder;
class nsIAutoCompleteController;



class nsNavHistory final : public nsSupportsWeakReference
                         , public nsINavHistoryService
                         , public nsIObserver
                         , public nsIBrowserHistory
                         , public nsPIPlacesDatabase
                         , public mozIStorageVacuumParticipant
{
  friend class PlacesSQLQueryBuilder;

public:
  nsNavHistory();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSINAVHISTORYSERVICE
  NS_DECL_NSIBROWSERHISTORY
  NS_DECL_NSIOBSERVER
  NS_DECL_NSPIPLACESDATABASE
  NS_DECL_MOZISTORAGEVACUUMPARTICIPANT

  


  static already_AddRefed<nsNavHistory> GetSingleton();

  


  nsresult Init();

  




  static nsNavHistory* GetHistoryService()
  {
    if (!gHistoryService) {
      nsCOMPtr<nsINavHistoryService> serv =
        do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID);
      NS_ENSURE_TRUE(serv, nullptr);
      NS_ASSERTION(gHistoryService, "Should have static instance pointer now");
    }
    return gHistoryService;
  }

  






  static const nsNavHistory* GetConstHistoryService()
  {
    const nsNavHistory* const history = gHistoryService;
    return history;
  }

  












  nsresult GetIdForPage(nsIURI* aURI,
                        int64_t* _pageId, nsCString& _GUID);

  












  nsresult GetOrCreateIdForPage(nsIURI* aURI,
                                int64_t* _pageId, nsCString& _GUID);

  







  nsresult UpdateFrecency(int64_t aPlaceId);

  






  nsresult FixInvalidFrecencies();

  







  nsresult invalidateFrecencies(const nsCString& aPlaceIdsQueryString);

  


















  nsresult NotifyOnPageExpired(nsIURI *aURI, PRTime aVisitTime,
                               bool aWholeEntry, const nsACString& aGUID,
                               uint16_t aReason, uint32_t aTransitionType);

  



  nsIStringBundle* GetBundle();
  nsIStringBundle* GetDateFormatBundle();
  nsICollation* GetCollation();
  void GetStringFromName(const char16_t* aName, nsACString& aResult);
  void GetAgeInDaysString(int32_t aInt, const char16_t *aName,
                          nsACString& aResult);
  void GetMonthName(int32_t aIndex, nsACString& aResult);
  void GetMonthYear(int32_t aMonth, int32_t aYear, nsACString& aResult);

  
  bool IsHistoryDisabled() {
    return !mHistoryEnabled;
  }

  
  static const int32_t kGetInfoIndex_PageID;
  static const int32_t kGetInfoIndex_URL;
  static const int32_t kGetInfoIndex_Title;
  static const int32_t kGetInfoIndex_RevHost;
  static const int32_t kGetInfoIndex_VisitCount;
  static const int32_t kGetInfoIndex_VisitDate;
  static const int32_t kGetInfoIndex_FaviconURL;
  static const int32_t kGetInfoIndex_ItemId;
  static const int32_t kGetInfoIndex_ItemDateAdded;
  static const int32_t kGetInfoIndex_ItemLastModified;
  static const int32_t kGetInfoIndex_ItemParentId;
  static const int32_t kGetInfoIndex_ItemTags;
  static const int32_t kGetInfoIndex_Frecency;
  static const int32_t kGetInfoIndex_Hidden;
  static const int32_t kGetInfoIndex_Guid;

  int64_t GetTagsFolder();

  
  
  nsresult GetQueryResults(nsNavHistoryQueryResultNode *aResultNode,
                           const nsCOMArray<nsNavHistoryQuery>& aQueries,
                           nsNavHistoryQueryOptions *aOptions,
                           nsCOMArray<nsNavHistoryResultNode>* aResults);

  
  
  nsresult RowToResult(mozIStorageValueArray* aRow,
                       nsNavHistoryQueryOptions* aOptions,
                       nsNavHistoryResultNode** aResult);
  nsresult QueryRowToResult(int64_t aItemId,
                            const nsACString& aBookmarkGuid,
                            const nsACString& aURI,
                            const nsACString& aTitle,
                            uint32_t aAccessCount, PRTime aTime,
                            const nsACString& aFavicon,
                            nsNavHistoryResultNode** aNode);

  nsresult VisitIdToResultNode(int64_t visitId,
                               nsNavHistoryQueryOptions* aOptions,
                               nsNavHistoryResultNode** aResult);

  nsresult BookmarkIdToResultNode(int64_t aBookmarkId,
                                  nsNavHistoryQueryOptions* aOptions,
                                  nsNavHistoryResultNode** aResult);
  nsresult URIToResultNode(nsIURI* aURI,
                           nsNavHistoryQueryOptions* aOptions,
                           nsNavHistoryResultNode** aResult);

  
  
  void SendPageChangedNotification(nsIURI* aURI, uint32_t aChangedAttribute,
                                   const nsAString& aValue,
                                   const nsACString& aGUID);

  


  int32_t GetDaysOfHistory();

  
  static uint32_t GetUpdateRequirements(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                        nsNavHistoryQueryOptions* aOptions,
                                        bool* aHasSearchTerms);
  bool EvaluateQueryForNode(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions* aOptions,
                              nsNavHistoryResultNode* aNode);

  static nsresult AsciiHostNameFromHostString(const nsACString& aHostName,
                                              nsACString& aAscii);
  void DomainNameFromURI(nsIURI* aURI,
                         nsACString& aDomainName);
  static PRTime NormalizeTime(uint32_t aRelative, PRTime aOffset);

  
  
  nsresult BeginUpdateBatch();
  nsresult EndUpdateBatch();

  
  int32_t mBatchLevel;
  
  mozStorageTransaction* mBatchDBTransaction;

  
  nsresult QueryStringToQueryArray(const nsACString& aQueryString,
                                   nsCOMArray<nsNavHistoryQuery>* aQueries,
                                   nsNavHistoryQueryOptions** aOptions);

  typedef nsDataHashtable<nsCStringHashKey, nsCString> StringHash;

  




  bool canNotify() { return mCanNotify; }

  enum RecentEventFlags {
    RECENT_TYPED      = 1 << 0,    
    RECENT_ACTIVATED  = 1 << 1,    
    RECENT_BOOKMARKED = 1 << 2     
  };

  




  uint32_t GetRecentFlags(nsIURI *aURI);

  







  void registerEmbedVisit(nsIURI* aURI, int64_t aTime);

  






  bool hasEmbedVisit(nsIURI* aURI);

  


  void clearEmbedVisits();

  int32_t GetFrecencyAgedWeight(int32_t aAgeInDays) const
  {
    if (aAgeInDays <= mFirstBucketCutoffInDays) {
      return mFirstBucketWeight;
    }
    if (aAgeInDays <= mSecondBucketCutoffInDays) {
      return mSecondBucketWeight;
    }
    if (aAgeInDays <= mThirdBucketCutoffInDays) {
      return mThirdBucketWeight;
    }
    if (aAgeInDays <= mFourthBucketCutoffInDays) {
      return mFourthBucketWeight;
    }
    return mDefaultWeight;
  }

  int32_t GetFrecencyBucketWeight(int32_t aBucketIndex) const
  {
    switch(aBucketIndex) {
      case 1:
        return mFirstBucketWeight;
      case 2:
        return mSecondBucketWeight;
      case 3:
        return mThirdBucketWeight;
      case 4:
        return mFourthBucketWeight;
      default:
        return mDefaultWeight;
    }
  }

  int32_t GetFrecencyTransitionBonus(int32_t aTransitionType,
                                     bool aVisited) const
  {
    switch (aTransitionType) {
      case nsINavHistoryService::TRANSITION_EMBED:
        return mEmbedVisitBonus;
      case nsINavHistoryService::TRANSITION_FRAMED_LINK:
        return mFramedLinkVisitBonus;
      case nsINavHistoryService::TRANSITION_LINK:
        return mLinkVisitBonus;
      case nsINavHistoryService::TRANSITION_TYPED:
        return aVisited ? mTypedVisitBonus : mUnvisitedTypedBonus;
      case nsINavHistoryService::TRANSITION_BOOKMARK:
        return aVisited ? mBookmarkVisitBonus : mUnvisitedBookmarkBonus;
      case nsINavHistoryService::TRANSITION_DOWNLOAD:
        return mDownloadVisitBonus;
      case nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT:
        return mPermRedirectVisitBonus;
      case nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY:
        return mTempRedirectVisitBonus;
      default:
        
        NS_WARN_IF_FALSE(!aTransitionType, "new transition but no bonus for frecency");
        return mDefaultVisitBonus;
    }
  }

  int32_t GetNumVisitsForFrecency() const
  {
    return mNumVisitsForFrecency;
  }

  


  void NotifyOnVisit(nsIURI* aURI,
                     int64_t aVisitID,
                     PRTime aTime,
                     int64_t referringVisitID,
                     int32_t aTransitionType,
                     const nsACString& aGUID,
                     bool aHidden);

  


  void NotifyTitleChange(nsIURI* aURI,
                         const nsString& title,
                         const nsACString& aGUID);

  


  void NotifyFrecencyChanged(nsIURI* aURI,
                             int32_t aNewFrecency,
                             const nsACString& aGUID,
                             bool aHidden,
                             PRTime aLastVisitDate);

  


  void NotifyManyFrecenciesChanged();

  


  void DispatchFrecencyChangedNotification(const nsACString& aSpec,
                                           int32_t aNewFrecency,
                                           const nsACString& aGUID,
                                           bool aHidden,
                                           PRTime aLastVisitDate) const;

  bool isBatching() {
    return mBatchLevel > 0;
  }

private:
  ~nsNavHistory();

  
  static nsNavHistory *gHistoryService;

protected:

  
  nsRefPtr<mozilla::places::Database> mDB;

  


  nsresult DecayFrecency();

  nsresult CalculateFrecency(int64_t aPageID, int32_t aTyped, int32_t aVisitCount, nsAutoCString &aURL, int32_t *aFrecency);
  nsresult CalculateFrecencyInternal(int64_t aPageID, int32_t aTyped, int32_t aVisitCount, bool aIsBookmarked, int32_t *aFrecency);

  nsresult RemovePagesInternal(const nsCString& aPlaceIdsQueryString);
  nsresult CleanupPlacesOnVisitsDelete(const nsCString& aPlaceIdsQueryString);

  




  void LoadPrefs();

  





  PRTime GetNow();
  PRTime mCachedNow;
  nsCOMPtr<nsITimer> mExpireNowTimer;
  


  static void expireNowTimerCallback(nsITimer* aTimer, void* aClosure);

  nsresult ConstructQueryString(const nsCOMArray<nsNavHistoryQuery>& aQueries, 
                                nsNavHistoryQueryOptions* aOptions,
                                nsCString& queryString,
                                bool& aParamsPresent,
                                StringHash& aAddParams);

  nsresult QueryToSelectClause(nsNavHistoryQuery* aQuery,
                               nsNavHistoryQueryOptions* aOptions,
                               int32_t aQueryIndex,
                               nsCString* aClause);
  nsresult BindQueryClauseParameters(mozIStorageBaseStatement* statement,
                                     int32_t aQueryIndex,
                                     nsNavHistoryQuery* aQuery,
                                     nsNavHistoryQueryOptions* aOptions);

  nsresult ResultsAsList(mozIStorageStatement* statement,
                         nsNavHistoryQueryOptions* aOptions,
                         nsCOMArray<nsNavHistoryResultNode>* aResults);

  void TitleForDomain(const nsCString& domain, nsACString& aTitle);

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

  
  typedef nsDataHashtable<nsCStringHashKey, int64_t> RecentEventHash;
  RecentEventHash mRecentTyped;
  RecentEventHash mRecentLink;
  RecentEventHash mRecentBookmark;

  
  class VisitHashKey : public nsURIHashKey
  {
  public:
    explicit VisitHashKey(const nsIURI* aURI)
    : nsURIHashKey(aURI)
    {
    }
    VisitHashKey(const VisitHashKey& aOther)
    : nsURIHashKey(aOther)
    {
      NS_NOTREACHED("Do not call me!");
    }
    PRTime visitTime;
  };

  nsTHashtable<VisitHashKey> mEmbedVisits;

  bool CheckIsRecentEvent(RecentEventHash* hashTable,
                            const nsACString& url);
  void ExpireNonrecentEvents(RecentEventHash* hashTable);

#ifdef MOZ_XUL
  nsresult AutoCompleteFeedback(int32_t aIndex,
                                nsIAutoCompleteController *aController);
#endif

  
  
  bool mHistoryEnabled;

  
  int32_t mNumVisitsForFrecency;
  int32_t mFirstBucketCutoffInDays;
  int32_t mSecondBucketCutoffInDays;
  int32_t mThirdBucketCutoffInDays;
  int32_t mFourthBucketCutoffInDays;
  int32_t mFirstBucketWeight;
  int32_t mSecondBucketWeight;
  int32_t mThirdBucketWeight;
  int32_t mFourthBucketWeight;
  int32_t mDefaultWeight;
  int32_t mEmbedVisitBonus;
  int32_t mFramedLinkVisitBonus;
  int32_t mLinkVisitBonus;
  int32_t mTypedVisitBonus;
  int32_t mBookmarkVisitBonus;
  int32_t mDownloadVisitBonus;
  int32_t mPermRedirectVisitBonus;
  int32_t mTempRedirectVisitBonus;
  int32_t mDefaultVisitBonus;
  int32_t mUnvisitedBookmarkBonus;
  int32_t mUnvisitedTypedBonus;

  
  nsresult TokensToQueries(const nsTArray<QueryKeyValuePair>& aTokens,
                           nsCOMArray<nsNavHistoryQuery>* aQueries,
                           nsNavHistoryQueryOptions* aOptions);

  int64_t mTagsFolder;

  int32_t mDaysOfHistory;
  int64_t mLastCachedStartOfDay;
  int64_t mLastCachedEndOfDay;

  
  bool mCanNotify;
  nsCategoryCache<nsINavHistoryObserver> mCacheObservers;
};


#define PLACES_URI_PREFIX "place:"


inline bool IsQueryURI(const nsCString &uri)
{
  return StringBeginsWith(uri, NS_LITERAL_CSTRING(PLACES_URI_PREFIX));
}


inline const nsDependentCSubstring QueryURIToQuery(const nsCString &uri)
{
  NS_ASSERTION(IsQueryURI(uri), "should only be called for query URIs");
  return Substring(uri, NS_LITERAL_CSTRING(PLACES_URI_PREFIX).Length());
}

#endif 
