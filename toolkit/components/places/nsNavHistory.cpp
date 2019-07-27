




#include <stdio.h>

#include "mozilla/DebugOnly.h"

#include "nsNavHistory.h"

#include "mozIPlacesAutoComplete.h"
#include "nsNavBookmarks.h"
#include "nsAnnotationService.h"
#include "nsFaviconService.h"
#include "nsPlacesMacros.h"
#include "History.h"
#include "Helpers.h"

#include "nsTArray.h"
#include "nsCollationCID.h"
#include "nsILocaleService.h"
#include "nsNetUtil.h"
#include "nsPrintfCString.h"
#include "nsPromiseFlatString.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "prsystem.h"
#include "prtime.h"
#include "nsEscape.h"
#include "nsIEffectiveTLDService.h"
#include "nsIClassInfoImpl.h"
#include "nsThreadUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsMathUtils.h"
#include "mozilla/storage.h"
#include "mozilla/Preferences.h"
#include <algorithm>

#ifdef MOZ_XUL
#include "nsIAutoCompleteInput.h"
#include "nsIAutoCompletePopup.h"
#endif

using namespace mozilla;
using namespace mozilla::places;






#define RECENT_EVENT_QUEUE_MAX_LENGTH 128


#define PREF_HISTORY_ENABLED                    "places.history.enabled"

#define PREF_FREC_NUM_VISITS                    "places.frecency.numVisits"
#define PREF_FREC_NUM_VISITS_DEF                10
#define PREF_FREC_FIRST_BUCKET_CUTOFF           "places.frecency.firstBucketCutoff"
#define PREF_FREC_FIRST_BUCKET_CUTOFF_DEF       4
#define PREF_FREC_SECOND_BUCKET_CUTOFF          "places.frecency.secondBucketCutoff"
#define PREF_FREC_SECOND_BUCKET_CUTOFF_DEF      14
#define PREF_FREC_THIRD_BUCKET_CUTOFF           "places.frecency.thirdBucketCutoff"
#define PREF_FREC_THIRD_BUCKET_CUTOFF_DEF       31
#define PREF_FREC_FOURTH_BUCKET_CUTOFF          "places.frecency.fourthBucketCutoff"
#define PREF_FREC_FOURTH_BUCKET_CUTOFF_DEF      90
#define PREF_FREC_FIRST_BUCKET_WEIGHT           "places.frecency.firstBucketWeight"
#define PREF_FREC_FIRST_BUCKET_WEIGHT_DEF       100
#define PREF_FREC_SECOND_BUCKET_WEIGHT          "places.frecency.secondBucketWeight"
#define PREF_FREC_SECOND_BUCKET_WEIGHT_DEF      70
#define PREF_FREC_THIRD_BUCKET_WEIGHT           "places.frecency.thirdBucketWeight"
#define PREF_FREC_THIRD_BUCKET_WEIGHT_DEF       50
#define PREF_FREC_FOURTH_BUCKET_WEIGHT          "places.frecency.fourthBucketWeight"
#define PREF_FREC_FOURTH_BUCKET_WEIGHT_DEF      30
#define PREF_FREC_DEFAULT_BUCKET_WEIGHT         "places.frecency.defaultBucketWeight"
#define PREF_FREC_DEFAULT_BUCKET_WEIGHT_DEF     10
#define PREF_FREC_EMBED_VISIT_BONUS             "places.frecency.embedVisitBonus"
#define PREF_FREC_EMBED_VISIT_BONUS_DEF         0
#define PREF_FREC_FRAMED_LINK_VISIT_BONUS       "places.frecency.framedLinkVisitBonus"
#define PREF_FREC_FRAMED_LINK_VISIT_BONUS_DEF   0
#define PREF_FREC_LINK_VISIT_BONUS              "places.frecency.linkVisitBonus"
#define PREF_FREC_LINK_VISIT_BONUS_DEF          100
#define PREF_FREC_TYPED_VISIT_BONUS             "places.frecency.typedVisitBonus"
#define PREF_FREC_TYPED_VISIT_BONUS_DEF         2000
#define PREF_FREC_BOOKMARK_VISIT_BONUS          "places.frecency.bookmarkVisitBonus"
#define PREF_FREC_BOOKMARK_VISIT_BONUS_DEF      75
#define PREF_FREC_DOWNLOAD_VISIT_BONUS          "places.frecency.downloadVisitBonus"
#define PREF_FREC_DOWNLOAD_VISIT_BONUS_DEF      0
#define PREF_FREC_PERM_REDIRECT_VISIT_BONUS     "places.frecency.permRedirectVisitBonus"
#define PREF_FREC_PERM_REDIRECT_VISIT_BONUS_DEF 0
#define PREF_FREC_TEMP_REDIRECT_VISIT_BONUS     "places.frecency.tempRedirectVisitBonus"
#define PREF_FREC_TEMP_REDIRECT_VISIT_BONUS_DEF 0
#define PREF_FREC_DEFAULT_VISIT_BONUS           "places.frecency.defaultVisitBonus"
#define PREF_FREC_DEFAULT_VISIT_BONUS_DEF       0
#define PREF_FREC_UNVISITED_BOOKMARK_BONUS      "places.frecency.unvisitedBookmarkBonus"
#define PREF_FREC_UNVISITED_BOOKMARK_BONUS_DEF  140
#define PREF_FREC_UNVISITED_TYPED_BONUS         "places.frecency.unvisitedTypedBonus"
#define PREF_FREC_UNVISITED_TYPED_BONUS_DEF     200



#define RENEW_CACHED_NOW_TIMEOUT ((int32_t)3 * PR_MSEC_PER_SEC)


#define CHARSET_ANNO NS_LITERAL_CSTRING("URIProperties/characterSet")



#define HISTORY_ADDITIONAL_DATE_CONT_NUM 3


#define HISTORY_DATE_CONT_NUM(_daysFromOldestVisit) \
  (HISTORY_ADDITIONAL_DATE_CONT_NUM + \
   std::min(6, (int32_t)ceilf((float)_daysFromOldestVisit/30)))

#define HISTORY_DATE_CONT_LENGTH 8


#define EMBED_VISITS_INITIAL_CACHE_LENGTH 64


#define RECENT_EVENTS_INITIAL_CACHE_LENGTH 64


#ifdef MOZ_XUL
#define TOPIC_AUTOCOMPLETE_FEEDBACK_INCOMING "autocomplete-will-enter-text"
#endif
#define TOPIC_IDLE_DAILY "idle-daily"
#define TOPIC_PREF_CHANGED "nsPref:changed"
#define TOPIC_PROFILE_TEARDOWN "profile-change-teardown"
#define TOPIC_PROFILE_CHANGE "profile-before-change"

static const char* kObservedPrefs[] = {
  PREF_HISTORY_ENABLED
, PREF_FREC_NUM_VISITS
, PREF_FREC_FIRST_BUCKET_CUTOFF
, PREF_FREC_SECOND_BUCKET_CUTOFF
, PREF_FREC_THIRD_BUCKET_CUTOFF
, PREF_FREC_FOURTH_BUCKET_CUTOFF
, PREF_FREC_FIRST_BUCKET_WEIGHT
, PREF_FREC_SECOND_BUCKET_WEIGHT
, PREF_FREC_THIRD_BUCKET_WEIGHT
, PREF_FREC_FOURTH_BUCKET_WEIGHT
, PREF_FREC_DEFAULT_BUCKET_WEIGHT
, PREF_FREC_EMBED_VISIT_BONUS
, PREF_FREC_FRAMED_LINK_VISIT_BONUS
, PREF_FREC_LINK_VISIT_BONUS
, PREF_FREC_TYPED_VISIT_BONUS
, PREF_FREC_BOOKMARK_VISIT_BONUS
, PREF_FREC_DOWNLOAD_VISIT_BONUS
, PREF_FREC_PERM_REDIRECT_VISIT_BONUS
, PREF_FREC_TEMP_REDIRECT_VISIT_BONUS
, PREF_FREC_DEFAULT_VISIT_BONUS
, PREF_FREC_UNVISITED_BOOKMARK_BONUS
, PREF_FREC_UNVISITED_TYPED_BONUS
, nullptr
};

NS_IMPL_ADDREF(nsNavHistory)
NS_IMPL_RELEASE(nsNavHistory)

NS_IMPL_CLASSINFO(nsNavHistory, nullptr, nsIClassInfo::SINGLETON,
                  NS_NAVHISTORYSERVICE_CID)
NS_INTERFACE_MAP_BEGIN(nsNavHistory)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryService)
  NS_INTERFACE_MAP_ENTRY(nsIBrowserHistory)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsPIPlacesDatabase)
  NS_INTERFACE_MAP_ENTRY(mozIStorageVacuumParticipant)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINavHistoryService)
  NS_IMPL_QUERY_CLASSINFO(nsNavHistory)
NS_INTERFACE_MAP_END


NS_IMPL_CI_INTERFACE_GETTER(nsNavHistory,
                            nsINavHistoryService,
                            nsIBrowserHistory)

namespace {

static int64_t GetSimpleBookmarksQueryFolder(
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions);
static void ParseSearchTermsFromQueries(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                        nsTArray<nsTArray<nsString>*>* aTerms);

void GetTagsSqlFragment(int64_t aTagsFolder,
                        const nsACString& aRelation,
                        bool aHasSearchTerms,
                        nsACString& _sqlFragment) {
  if (!aHasSearchTerms)
    _sqlFragment.AssignLiteral("null");
  else {
    
    _sqlFragment.Assign(NS_LITERAL_CSTRING(
         "(SELECT GROUP_CONCAT(t_t.title, ',') "
           "FROM moz_bookmarks b_t "
           "JOIN moz_bookmarks t_t ON t_t.id = +b_t.parent  "
           "WHERE b_t.fk = ") + aRelation + NS_LITERAL_CSTRING(" "
           "AND t_t.parent = ") +
           nsPrintfCString("%lld", aTagsFolder) + NS_LITERAL_CSTRING(" "
         ")"));
  }

  _sqlFragment.AppendLiteral(" AS tags ");
}





class UpdateBatchScoper
{
public:
  explicit UpdateBatchScoper(nsNavHistory& aNavHistory) : mNavHistory(aNavHistory)
  {
    mNavHistory.BeginUpdateBatch();
  }
  ~UpdateBatchScoper()
  {
    mNavHistory.EndUpdateBatch();
  }
protected:
  nsNavHistory& mNavHistory;
};

} 




const int32_t nsNavHistory::kGetInfoIndex_PageID = 0;
const int32_t nsNavHistory::kGetInfoIndex_URL = 1;
const int32_t nsNavHistory::kGetInfoIndex_Title = 2;
const int32_t nsNavHistory::kGetInfoIndex_RevHost = 3;
const int32_t nsNavHistory::kGetInfoIndex_VisitCount = 4;
const int32_t nsNavHistory::kGetInfoIndex_VisitDate = 5;
const int32_t nsNavHistory::kGetInfoIndex_FaviconURL = 6;
const int32_t nsNavHistory::kGetInfoIndex_ItemId = 7;
const int32_t nsNavHistory::kGetInfoIndex_ItemDateAdded = 8;
const int32_t nsNavHistory::kGetInfoIndex_ItemLastModified = 9;
const int32_t nsNavHistory::kGetInfoIndex_ItemParentId = 10;
const int32_t nsNavHistory::kGetInfoIndex_ItemTags = 11;
const int32_t nsNavHistory::kGetInfoIndex_Frecency = 12;
const int32_t nsNavHistory::kGetInfoIndex_Hidden = 13;
const int32_t nsNavHistory::kGetInfoIndex_Guid = 14;

PLACES_FACTORY_SINGLETON_IMPLEMENTATION(nsNavHistory, gHistoryService)


nsNavHistory::nsNavHistory()
  : mBatchLevel(0)
  , mBatchDBTransaction(nullptr)
  , mCachedNow(0)
  , mRecentTyped(RECENT_EVENTS_INITIAL_CACHE_LENGTH)
  , mRecentLink(RECENT_EVENTS_INITIAL_CACHE_LENGTH)
  , mRecentBookmark(RECENT_EVENTS_INITIAL_CACHE_LENGTH)
  , mEmbedVisits(EMBED_VISITS_INITIAL_CACHE_LENGTH)
  , mHistoryEnabled(true)
  , mNumVisitsForFrecency(10)
  , mTagsFolder(-1)
  , mDaysOfHistory(-1)
  , mLastCachedStartOfDay(INT64_MAX)
  , mLastCachedEndOfDay(0)
  , mCanNotify(true)
  , mCacheObservers("history-observers")
{
  NS_ASSERTION(!gHistoryService,
               "Attempting to create two instances of the service!");
  gHistoryService = this;
}


nsNavHistory::~nsNavHistory()
{
  
  
  NS_ASSERTION(gHistoryService == this,
               "Deleting a non-singleton instance of the service");
  if (gHistoryService == this)
    gHistoryService = nullptr;
}


nsresult
nsNavHistory::Init()
{
  LoadPrefs();

  mDB = Database::GetDatabase();
  NS_ENSURE_STATE(mDB);

  







  
  Preferences::AddWeakObservers(this, kObservedPrefs);

  nsCOMPtr<nsIObserverService> obsSvc = services::GetObserverService();
  if (obsSvc) {
    (void)obsSvc->AddObserver(this, TOPIC_PLACES_CONNECTION_CLOSED, true);
    (void)obsSvc->AddObserver(this, TOPIC_IDLE_DAILY, true);
#ifdef MOZ_XUL
    (void)obsSvc->AddObserver(this, TOPIC_AUTOCOMPLETE_FEEDBACK_INCOMING, true);
#endif
  }

  
  

  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::GetDatabaseStatus(uint16_t *aDatabaseStatus)
{
  NS_ENSURE_ARG_POINTER(aDatabaseStatus);
  *aDatabaseStatus = mDB->GetDatabaseStatus();
  return NS_OK;
}

uint32_t
nsNavHistory::GetRecentFlags(nsIURI *aURI)
{
  uint32_t result = 0;
  nsAutoCString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to get aURI's spec");

  if (NS_SUCCEEDED(rv)) {
    if (CheckIsRecentEvent(&mRecentTyped, spec))
      result |= RECENT_TYPED;
    if (CheckIsRecentEvent(&mRecentLink, spec))
      result |= RECENT_ACTIVATED;
    if (CheckIsRecentEvent(&mRecentBookmark, spec))
      result |= RECENT_BOOKMARKED;
  }

  return result;
}

nsresult
nsNavHistory::GetIdForPage(nsIURI* aURI,
                           int64_t* _pageId,
                           nsCString& _GUID)
{
  *_pageId = 0;

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT id, url, title, rev_host, visit_count, guid "
    "FROM moz_places "
    "WHERE url = :page_url "
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasEntry = false;
  rv = stmt->ExecuteStep(&hasEntry);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasEntry) {
    rv = stmt->GetInt64(0, _pageId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->GetUTF8String(5, _GUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsNavHistory::GetOrCreateIdForPage(nsIURI* aURI,
                                   int64_t* _pageId,
                                   nsCString& _GUID)
{
  nsresult rv = GetIdForPage(aURI, _pageId, _GUID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (*_pageId != 0) {
    return NS_OK;
  }

  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "INSERT OR IGNORE INTO moz_places (url, rev_host, hidden, frecency, guid) "
    "VALUES (:page_url, :rev_host, :hidden, :frecency, GENERATE_GUID()) "
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsAutoString revHost;
  rv = GetReversedHostname(aURI, revHost);
  
  if (NS_SUCCEEDED(rv)) {
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("rev_host"), revHost);
  } else {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("rev_host"));
  }
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), 1);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoCString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("frecency"),
                             IsQueryURI(spec) ? 0 : -1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  {
    nsCOMPtr<mozIStorageStatement> getIdStmt = mDB->GetStatement(
      "SELECT id, guid FROM moz_places WHERE url = :page_url "
    );
    NS_ENSURE_STATE(getIdStmt);
    mozStorageStatementScoper getIdScoper(getIdStmt);

    rv = URIBinder::Bind(getIdStmt, NS_LITERAL_CSTRING("page_url"), aURI);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasResult = false;
    rv = getIdStmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(hasResult, "hasResult is false but the call succeeded?");
    *_pageId = getIdStmt->AsInt64(0);
    rv = getIdStmt->GetUTF8String(1, _GUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


void
nsNavHistory::LoadPrefs()
{
  
  mHistoryEnabled = Preferences::GetBool(PREF_HISTORY_ENABLED, true);

  
#define FRECENCY_PREF(_prop, _pref) \
  _prop = Preferences::GetInt(_pref, _pref##_DEF)

  FRECENCY_PREF(mNumVisitsForFrecency,     PREF_FREC_NUM_VISITS);
  FRECENCY_PREF(mFirstBucketCutoffInDays,  PREF_FREC_FIRST_BUCKET_CUTOFF);
  FRECENCY_PREF(mSecondBucketCutoffInDays, PREF_FREC_SECOND_BUCKET_CUTOFF);
  FRECENCY_PREF(mThirdBucketCutoffInDays,  PREF_FREC_THIRD_BUCKET_CUTOFF);
  FRECENCY_PREF(mFourthBucketCutoffInDays, PREF_FREC_FOURTH_BUCKET_CUTOFF);
  FRECENCY_PREF(mEmbedVisitBonus,          PREF_FREC_EMBED_VISIT_BONUS);
  FRECENCY_PREF(mFramedLinkVisitBonus,     PREF_FREC_FRAMED_LINK_VISIT_BONUS);
  FRECENCY_PREF(mLinkVisitBonus,           PREF_FREC_LINK_VISIT_BONUS);
  FRECENCY_PREF(mTypedVisitBonus,          PREF_FREC_TYPED_VISIT_BONUS);
  FRECENCY_PREF(mBookmarkVisitBonus,       PREF_FREC_BOOKMARK_VISIT_BONUS);
  FRECENCY_PREF(mDownloadVisitBonus,       PREF_FREC_DOWNLOAD_VISIT_BONUS);
  FRECENCY_PREF(mPermRedirectVisitBonus,   PREF_FREC_PERM_REDIRECT_VISIT_BONUS);
  FRECENCY_PREF(mTempRedirectVisitBonus,   PREF_FREC_TEMP_REDIRECT_VISIT_BONUS);
  FRECENCY_PREF(mDefaultVisitBonus,        PREF_FREC_DEFAULT_VISIT_BONUS);
  FRECENCY_PREF(mUnvisitedBookmarkBonus,   PREF_FREC_UNVISITED_BOOKMARK_BONUS);
  FRECENCY_PREF(mUnvisitedTypedBonus,      PREF_FREC_UNVISITED_TYPED_BONUS);
  FRECENCY_PREF(mFirstBucketWeight,        PREF_FREC_FIRST_BUCKET_WEIGHT);
  FRECENCY_PREF(mSecondBucketWeight,       PREF_FREC_SECOND_BUCKET_WEIGHT);
  FRECENCY_PREF(mThirdBucketWeight,        PREF_FREC_THIRD_BUCKET_WEIGHT);
  FRECENCY_PREF(mFourthBucketWeight,       PREF_FREC_FOURTH_BUCKET_WEIGHT);
  FRECENCY_PREF(mDefaultWeight,            PREF_FREC_DEFAULT_BUCKET_WEIGHT);

#undef FRECENCY_PREF
}


void
nsNavHistory::NotifyOnVisit(nsIURI* aURI,
                          int64_t aVisitID,
                          PRTime aTime,
                          int64_t referringVisitID,
                          int32_t aTransitionType,
                          const nsACString& aGUID,
                          bool aHidden)
{
  MOZ_ASSERT(!aGUID.IsEmpty());
  
  
  
  
  if (mDaysOfHistory == 0) {
    mDaysOfHistory = 1;
  } else if (aTime > mLastCachedEndOfDay || aTime < mLastCachedStartOfDay) {
    mDaysOfHistory = -1;
  }

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver,
                   OnVisit(aURI, aVisitID, aTime, 0,
                           referringVisitID, aTransitionType, aGUID, aHidden));
}

void
nsNavHistory::NotifyTitleChange(nsIURI* aURI,
                                const nsString& aTitle,
                                const nsACString& aGUID)
{
  MOZ_ASSERT(!aGUID.IsEmpty());
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver, OnTitleChanged(aURI, aTitle, aGUID));
}

void
nsNavHistory::NotifyFrecencyChanged(nsIURI* aURI,
                                    int32_t aNewFrecency,
                                    const nsACString& aGUID,
                                    bool aHidden,
                                    PRTime aLastVisitDate)
{
  MOZ_ASSERT(!aGUID.IsEmpty());
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver,
                   OnFrecencyChanged(aURI, aNewFrecency, aGUID, aHidden,
                                     aLastVisitDate));
}

void
nsNavHistory::NotifyManyFrecenciesChanged()
{
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver,
                   OnManyFrecenciesChanged());
}

namespace {

class FrecencyNotification : public nsRunnable
{
public:
  FrecencyNotification(const nsACString& aSpec,
                       int32_t aNewFrecency,
                       const nsACString& aGUID,
                       bool aHidden,
                       PRTime aLastVisitDate)
    : mSpec(aSpec)
    , mNewFrecency(aNewFrecency)
    , mGUID(aGUID)
    , mHidden(aHidden)
    , mLastVisitDate(aLastVisitDate)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread(), "Must be called on the main thread");
    nsNavHistory* navHistory = nsNavHistory::GetHistoryService();
    if (navHistory) {
      nsCOMPtr<nsIURI> uri;
      (void)NS_NewURI(getter_AddRefs(uri), mSpec);
      navHistory->NotifyFrecencyChanged(uri, mNewFrecency, mGUID, mHidden,
                                        mLastVisitDate);
    }
    return NS_OK;
  }

private:
  nsCString mSpec;
  int32_t mNewFrecency;
  nsCString mGUID;
  bool mHidden;
  PRTime mLastVisitDate;
};

} 

void
nsNavHistory::DispatchFrecencyChangedNotification(const nsACString& aSpec,
                                                  int32_t aNewFrecency,
                                                  const nsACString& aGUID,
                                                  bool aHidden,
                                                  PRTime aLastVisitDate) const
{
  nsCOMPtr<nsIRunnable> notif = new FrecencyNotification(aSpec, aNewFrecency,
                                                         aGUID, aHidden,
                                                         aLastVisitDate);
  (void)NS_DispatchToMainThread(notif);
}

int32_t
nsNavHistory::GetDaysOfHistory() {
  MOZ_ASSERT(NS_IsMainThread(), "This can only be called on the main thread");

  if (mDaysOfHistory != -1)
    return mDaysOfHistory;

  
  
  
  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT CAST(( "
        "strftime('%s','now','localtime','utc') - "
        "(SELECT MIN(visit_date)/1000000 FROM moz_historyvisits) "
      ") AS DOUBLE) "
    "/86400, "
    "strftime('%s','now','localtime','+1 day','start of day','utc') * 1000000"
  );
  NS_ENSURE_TRUE(stmt, 0);
  mozStorageStatementScoper scoper(stmt);

  bool hasResult;
  if (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    
    
    bool hasNoVisits;
    (void)stmt->GetIsNull(0, &hasNoVisits);
    mDaysOfHistory = hasNoVisits ?
      0 : std::max(1, static_cast<int32_t>(ceil(stmt->AsDouble(0))));
    mLastCachedStartOfDay =
      NormalizeTime(nsINavHistoryQuery::TIME_RELATIVE_TODAY, 0);
    mLastCachedEndOfDay = stmt->AsInt64(1) - 1; 
  }

  return mDaysOfHistory;
}

PRTime
nsNavHistory::GetNow()
{
  if (!mCachedNow) {
    mCachedNow = PR_Now();
    if (!mExpireNowTimer)
      mExpireNowTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mExpireNowTimer)
      mExpireNowTimer->InitWithFuncCallback(expireNowTimerCallback, this,
                                            RENEW_CACHED_NOW_TIMEOUT,
                                            nsITimer::TYPE_ONE_SHOT);
  }
  return mCachedNow;
}


void nsNavHistory::expireNowTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsNavHistory *history = static_cast<nsNavHistory *>(aClosure);
  if (history) {
    history->mCachedNow = 0;
    history->mExpireNowTimer = 0;
  }
}







static PRTime
NormalizeTimeRelativeToday(PRTime aTime)
{
  
  PRExplodedTime explodedTime;
  PR_ExplodeTime(aTime, PR_LocalTimeParameters, &explodedTime);

  
  explodedTime.tm_min =
    explodedTime.tm_hour =
    explodedTime.tm_sec =
    explodedTime.tm_usec = 0;

  return PR_ImplodeTime(&explodedTime);
}











PRTime 
nsNavHistory::NormalizeTime(uint32_t aRelative, PRTime aOffset)
{
  PRTime ref;
  switch (aRelative)
  {
    case nsINavHistoryQuery::TIME_RELATIVE_EPOCH:
      return aOffset;
    case nsINavHistoryQuery::TIME_RELATIVE_TODAY:
      ref = NormalizeTimeRelativeToday(PR_Now());
      break;
    case nsINavHistoryQuery::TIME_RELATIVE_NOW:
      ref = PR_Now();
      break;
    default:
      NS_NOTREACHED("Invalid relative time");
      return 0;
  }
  return ref + aOffset;
}























uint32_t
nsNavHistory::GetUpdateRequirements(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                    nsNavHistoryQueryOptions* aOptions,
                                    bool* aHasSearchTerms)
{
  NS_ASSERTION(aQueries.Count() > 0, "Must have at least one query");

  
  *aHasSearchTerms = false;
  int32_t i;
  for (i = 0; i < aQueries.Count(); i ++) {
    aQueries[i]->GetHasSearchTerms(aHasSearchTerms);
    if (*aHasSearchTerms)
      break;
  }

  bool nonTimeBasedItems = false;
  bool domainBasedItems = false;

  for (i = 0; i < aQueries.Count(); i ++) {
    nsNavHistoryQuery* query = aQueries[i];

    if (query->Folders().Length() > 0 ||
        query->OnlyBookmarked() ||
        query->Tags().Length() > 0) {
      return QUERYUPDATE_COMPLEX_WITH_BOOKMARKS;
    }

    
    
    if (!query->SearchTerms().IsEmpty() ||
        !query->Domain().IsVoid() ||
        query->Uri() != nullptr)
      nonTimeBasedItems = true;

    if (! query->Domain().IsVoid())
      domainBasedItems = true;
  }

  if (aOptions->ResultType() ==
      nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY)
    return QUERYUPDATE_COMPLEX_WITH_BOOKMARKS;

  
  
  
  
  if (aOptions->MaxResults() > 0)
    return QUERYUPDATE_COMPLEX;

  if (aQueries.Count() == 1 && domainBasedItems)
    return QUERYUPDATE_HOST;
  if (aQueries.Count() == 1 && !nonTimeBasedItems)
    return QUERYUPDATE_TIME;

  return QUERYUPDATE_SIMPLE;
}
















bool
nsNavHistory::EvaluateQueryForNode(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                   nsNavHistoryQueryOptions* aOptions,
                                   nsNavHistoryResultNode* aNode)
{
  
  nsCOMPtr<nsIURI> nodeUri;

  
  if (aNode->mHidden && !aOptions->IncludeHidden())
    return false;

  for (int32_t i = 0; i < aQueries.Count(); i ++) {
    bool hasIt;
    nsCOMPtr<nsNavHistoryQuery> query = aQueries[i];

    
    query->GetHasBeginTime(&hasIt);
    if (hasIt) {
      PRTime beginTime = NormalizeTime(query->BeginTimeReference(),
                                       query->BeginTime());
      if (aNode->mTime < beginTime)
        continue; 
    }

    
    query->GetHasEndTime(&hasIt);
    if (hasIt) {
      PRTime endTime = NormalizeTime(query->EndTimeReference(),
                                     query->EndTime());
      if (aNode->mTime > endTime)
        continue; 
    }

    
    if (! query->SearchTerms().IsEmpty()) {
      
      
      nsCOMArray<nsNavHistoryResultNode> inputSet;
      inputSet.AppendObject(aNode);
      nsCOMArray<nsNavHistoryQuery> queries;
      queries.AppendObject(query);
      nsCOMArray<nsNavHistoryResultNode> filteredSet;
      nsresult rv = FilterResultSet(nullptr, inputSet, &filteredSet, queries, aOptions);
      if (NS_FAILED(rv))
        continue;
      if (! filteredSet.Count())
        continue; 
    }

    
    query->GetHasDomain(&hasIt);
    if (hasIt) {
      if (! nodeUri) {
        
        if (NS_FAILED(NS_NewURI(getter_AddRefs(nodeUri), aNode->mURI)))
          continue;
      }
      nsAutoCString asciiRequest;
      if (NS_FAILED(AsciiHostNameFromHostString(query->Domain(), asciiRequest)))
        continue;

      if (query->DomainIsHost()) {
        nsAutoCString host;
        if (NS_FAILED(nodeUri->GetAsciiHost(host)))
          continue;

        if (! asciiRequest.Equals(host))
          continue; 
      }
      
      nsAutoCString domain;
      DomainNameFromURI(nodeUri, domain);
      if (! asciiRequest.Equals(domain))
        continue; 
    }

    
    if (query->Uri()) {
      if (! nodeUri) { 
        if (NS_FAILED(NS_NewURI(getter_AddRefs(nodeUri), aNode->mURI)))
          continue;
      }
      if (! query->UriIsPrefix()) {
        
        bool equals;
        nsresult rv = query->Uri()->Equals(nodeUri, &equals);
        NS_ENSURE_SUCCESS(rv, false);
        if (! equals)
          continue;
      } else {
        
        
        
        nsAutoCString nodeUriString;
        nodeUri->GetAsciiSpec(nodeUriString);
        nsAutoCString queryUriString;
        query->Uri()->GetAsciiSpec(queryUriString);
        if (queryUriString.Length() > nodeUriString.Length())
          continue; 
        nodeUriString.SetLength(queryUriString.Length());
        if (! nodeUriString.Equals(queryUriString))
          continue; 
      }
    }

    
    const nsTArray<uint32_t>& transitions = query->Transitions();
    if (aNode->mTransitionType > 0 &&
        transitions.Length() &&
        !transitions.Contains(aNode->mTransitionType)) {
      continue; 
    }

    
    
    
    return true;
  }

  
  return false;
}








nsresult 
nsNavHistory::AsciiHostNameFromHostString(const nsACString& aHostName,
                                          nsACString& aAscii)
{
  
  nsAutoCString fakeURL("http://");
  fakeURL.Append(aHostName);
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), fakeURL);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = uri->GetAsciiHost(aAscii);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}






void
nsNavHistory::DomainNameFromURI(nsIURI *aURI,
                                nsACString& aDomainName)
{
  
  if (!mTLDService)
    mTLDService = do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID);

  if (mTLDService) {
    
    
    nsresult rv = mTLDService->GetBaseDomain(aURI, 0, aDomainName);
    if (NS_SUCCEEDED(rv))
      return;
  }

  
  
  aURI->GetAsciiHost(aDomainName);
}


NS_IMETHODIMP
nsNavHistory::GetHasHistoryEntries(bool* aHasEntries)
{
  NS_ENSURE_ARG_POINTER(aHasEntries);
  *aHasEntries = GetDaysOfHistory() > 0;
  return NS_OK;
}


namespace {

class InvalidateAllFrecenciesCallback : public AsyncStatementCallback
{
public:
  InvalidateAllFrecenciesCallback()
  {
  }

  NS_IMETHOD HandleCompletion(uint16_t aReason)
  {
    if (aReason == REASON_FINISHED) {
      nsNavHistory *navHistory = nsNavHistory::GetHistoryService();
      NS_ENSURE_STATE(navHistory);
      navHistory->NotifyManyFrecenciesChanged();
    }
    return NS_OK;
  }
};

} 

nsresult
nsNavHistory::invalidateFrecencies(const nsCString& aPlaceIdsQueryString)
{
  
  nsCString invalidFrecenciesSQLFragment(
    "UPDATE moz_places SET frecency = "
  );
  if (!aPlaceIdsQueryString.IsEmpty())
    invalidFrecenciesSQLFragment.AppendLiteral("NOTIFY_FRECENCY(");
  invalidFrecenciesSQLFragment.AppendLiteral(
      "(CASE "
       "WHEN url BETWEEN 'place:' AND 'place;' "
       "THEN 0 "
       "ELSE -1 "
       "END) "
  );
  if (!aPlaceIdsQueryString.IsEmpty()) {
    invalidFrecenciesSQLFragment.AppendLiteral(
      ", url, guid, hidden, last_visit_date) "
    );
  }
  invalidFrecenciesSQLFragment.AppendLiteral(
    "WHERE frecency > 0 "
  );
  if (!aPlaceIdsQueryString.IsEmpty()) {
    invalidFrecenciesSQLFragment.AppendLiteral("AND id IN(");
    invalidFrecenciesSQLFragment.Append(aPlaceIdsQueryString);
    invalidFrecenciesSQLFragment.Append(')');
  }
  nsRefPtr<InvalidateAllFrecenciesCallback> cb =
    aPlaceIdsQueryString.IsEmpty() ? new InvalidateAllFrecenciesCallback()
                                   : nullptr;

  nsCOMPtr<mozIStorageAsyncStatement> stmt = mDB->GetAsyncStatement(
    invalidFrecenciesSQLFragment
  );
  NS_ENSURE_STATE(stmt);

  nsCOMPtr<mozIStoragePendingStatement> ps;
  nsresult rv = stmt->ExecuteAsync(cb, getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::MarkPageAsFollowedBookmark(nsIURI* aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
  if (IsHistoryDisabled())
    return NS_OK;

  nsAutoCString uriString;
  nsresult rv = aURI->GetSpec(uriString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int64_t unusedEventTime;
  if (mRecentBookmark.Get(uriString, &unusedEventTime))
    mRecentBookmark.Remove(uriString);

  if (mRecentBookmark.Count() > RECENT_EVENT_QUEUE_MAX_LENGTH)
    ExpireNonrecentEvents(&mRecentBookmark);

  mRecentBookmark.Put(uriString, GetNow());
  return NS_OK;
}










NS_IMETHODIMP
nsNavHistory::CanAddURI(nsIURI* aURI, bool* canAdd)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(canAdd);

  
  if (IsHistoryDisabled()) {
    *canAdd = false;
    return NS_OK;
  }

  nsAutoCString scheme;
  nsresult rv = aURI->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (scheme.EqualsLiteral("http")) {
    *canAdd = true;
    return NS_OK;
  }
  if (scheme.EqualsLiteral("https")) {
    *canAdd = true;
    return NS_OK;
  }

  
  if (scheme.EqualsLiteral("about") ||
      scheme.EqualsLiteral("imap") ||
      scheme.EqualsLiteral("news") ||
      scheme.EqualsLiteral("mailbox") ||
      scheme.EqualsLiteral("moz-anno") ||
      scheme.EqualsLiteral("view-source") ||
      scheme.EqualsLiteral("chrome") ||
      scheme.EqualsLiteral("resource") ||
      scheme.EqualsLiteral("data") ||
      scheme.EqualsLiteral("wyciwyg") ||
      scheme.EqualsLiteral("javascript") ||
      scheme.EqualsLiteral("blob")) {
    *canAdd = false;
    return NS_OK;
  }
  *canAdd = true;
  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::GetNewQuery(nsINavHistoryQuery **_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG_POINTER(_retval);

  nsRefPtr<nsNavHistoryQuery> query = new nsNavHistoryQuery();
  query.forget(_retval);
  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::GetNewQueryOptions(nsINavHistoryQueryOptions **_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG_POINTER(_retval);

  nsRefPtr<nsNavHistoryQueryOptions> queryOptions = new nsNavHistoryQueryOptions();
  queryOptions.forget(_retval);
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistory::ExecuteQuery(nsINavHistoryQuery *aQuery, nsINavHistoryQueryOptions *aOptions,
                           nsINavHistoryResult** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aQuery);
  NS_ENSURE_ARG(aOptions);
  NS_ENSURE_ARG_POINTER(_retval);

  return ExecuteQueries(&aQuery, 1, aOptions, _retval);
}














NS_IMETHODIMP
nsNavHistory::ExecuteQueries(nsINavHistoryQuery** aQueries, uint32_t aQueryCount,
                             nsINavHistoryQueryOptions *aOptions,
                             nsINavHistoryResult** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aQueries);
  NS_ENSURE_ARG(aOptions);
  NS_ENSURE_ARG(aQueryCount);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  
  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(aOptions);
  NS_ENSURE_TRUE(options, NS_ERROR_INVALID_ARG);

  
  nsCOMArray<nsNavHistoryQuery> queries;
  for (uint32_t i = 0; i < aQueryCount; i ++) {
    nsCOMPtr<nsNavHistoryQuery> query = do_QueryInterface(aQueries[i], &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    queries.AppendObject(query);
  }

  
  nsRefPtr<nsNavHistoryContainerResultNode> rootNode;
  int64_t folderId = GetSimpleBookmarksQueryFolder(queries, options);
  if (folderId) {
    
    
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
    nsRefPtr<nsNavHistoryResultNode> tempRootNode;
    rv = bookmarks->ResultNodeForContainer(folderId, options,
                                           getter_AddRefs(tempRootNode));
    if (NS_SUCCEEDED(rv)) {
      rootNode = tempRootNode->GetAsContainer();
    }
    else {
      NS_WARNING("Generating a generic empty node for a broken query!");
      
      options->SetExcludeItems(true);
    }
  }

  if (!rootNode) {
    
    
    rootNode = new nsNavHistoryQueryResultNode(EmptyCString(), EmptyCString(),
                                               queries, options);
  }

  
  nsRefPtr<nsNavHistoryResult> result;
  rv = nsNavHistoryResult::NewHistoryResult(aQueries, aQueryCount, options,
                                            rootNode, isBatching(),
                                            getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  result.forget(_retval);
  return NS_OK;
}









static
bool IsOptimizableHistoryQuery(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                 nsNavHistoryQueryOptions *aOptions,
                                 uint16_t aSortMode)
{
  if (aQueries.Count() != 1)
    return false;

  nsNavHistoryQuery *aQuery = aQueries[0];
 
  if (aOptions->QueryType() != nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY)
    return false;

  if (aOptions->ResultType() != nsINavHistoryQueryOptions::RESULTS_AS_URI)
    return false;

  if (aOptions->SortingMode() != aSortMode)
    return false;

  if (aOptions->MaxResults() <= 0)
    return false;

  if (aOptions->ExcludeItems())
    return false;

  if (aOptions->IncludeHidden())
    return false;

  if (aQuery->MinVisits() != -1 || aQuery->MaxVisits() != -1)
    return false;

  if (aQuery->BeginTime() || aQuery->BeginTimeReference()) 
    return false;

  if (aQuery->EndTime() || aQuery->EndTimeReference()) 
    return false;

  if (!aQuery->SearchTerms().IsEmpty()) 
    return false;

  if (aQuery->OnlyBookmarked()) 
    return false;

  if (aQuery->DomainIsHost() || !aQuery->Domain().IsEmpty())
    return false;

  if (aQuery->AnnotationIsNot() || !aQuery->Annotation().IsEmpty()) 
    return false;

  if (aQuery->UriIsPrefix() || aQuery->Uri()) 
    return false;

  if (aQuery->Folders().Length() > 0)
    return false;

  if (aQuery->Tags().Length() > 0)
    return false;

  if (aQuery->Transitions().Length() > 0)
    return false;

  return true;
}

static
bool NeedToFilterResultSet(const nsCOMArray<nsNavHistoryQuery>& aQueries, 
                             nsNavHistoryQueryOptions *aOptions)
{
  uint16_t resultType = aOptions->ResultType();
  return resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS;
}



class PlacesSQLQueryBuilder
{
public:
  PlacesSQLQueryBuilder(const nsCString& aConditions,
                        nsNavHistoryQueryOptions* aOptions,
                        bool aUseLimit,
                        nsNavHistory::StringHash& aAddParams,
                        bool aHasSearchTerms);

  nsresult GetQueryString(nsCString& aQueryString);

private:
  nsresult Select();

  nsresult SelectAsURI();
  nsresult SelectAsVisit();
  nsresult SelectAsDay();
  nsresult SelectAsSite();
  nsresult SelectAsTag();

  nsresult Where();
  nsresult GroupBy();
  nsresult OrderBy();
  nsresult Limit();

  void OrderByColumnIndexAsc(int32_t aIndex);
  void OrderByColumnIndexDesc(int32_t aIndex);
  
  void OrderByTextColumnIndexAsc(int32_t aIndex);
  void OrderByTextColumnIndexDesc(int32_t aIndex);

  const nsCString& mConditions;
  bool mUseLimit;
  bool mHasSearchTerms;

  uint16_t mResultType;
  uint16_t mQueryType;
  bool mIncludeHidden;
  uint16_t mSortingMode;
  uint32_t mMaxResults;

  nsCString mQueryString;
  nsCString mGroupBy;
  bool mHasDateColumns;
  bool mSkipOrderBy;
  nsNavHistory::StringHash& mAddParams;
};

PlacesSQLQueryBuilder::PlacesSQLQueryBuilder(
    const nsCString& aConditions, 
    nsNavHistoryQueryOptions* aOptions, 
    bool aUseLimit,
    nsNavHistory::StringHash& aAddParams,
    bool aHasSearchTerms)
: mConditions(aConditions)
, mUseLimit(aUseLimit)
, mHasSearchTerms(aHasSearchTerms)
, mResultType(aOptions->ResultType())
, mQueryType(aOptions->QueryType())
, mIncludeHidden(aOptions->IncludeHidden())
, mSortingMode(aOptions->SortingMode())
, mMaxResults(aOptions->MaxResults())
, mSkipOrderBy(false)
, mAddParams(aAddParams)
{
  mHasDateColumns = (mQueryType == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS);
}

nsresult
PlacesSQLQueryBuilder::GetQueryString(nsCString& aQueryString)
{
  nsresult rv = Select();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = Where();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = GroupBy();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = OrderBy();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = Limit();
  NS_ENSURE_SUCCESS(rv, rv);

  aQueryString = mQueryString;
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::Select()
{
  nsresult rv;

  switch (mResultType)
  {
    case nsINavHistoryQueryOptions::RESULTS_AS_URI:
    case nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS:
      rv = SelectAsURI();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case nsINavHistoryQueryOptions::RESULTS_AS_VISIT:
    case nsINavHistoryQueryOptions::RESULTS_AS_FULL_VISIT:
      rv = SelectAsVisit();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY:
    case nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY:
      rv = SelectAsDay();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY:
      rv = SelectAsSite();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY:
      rv = SelectAsTag();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    default:
      NS_NOTREACHED("Invalid result type");
  }
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsURI()
{
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  nsAutoCString tagsSqlFragment;

  switch (mQueryType) {
    case nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY:
      GetTagsSqlFragment(history->GetTagsFolder(),
                         NS_LITERAL_CSTRING("h.id"),
                         mHasSearchTerms,
                         tagsSqlFragment);

      mQueryString = NS_LITERAL_CSTRING(
        "SELECT h.id, h.url, h.title AS page_title, h.rev_host, h.visit_count, "
        "h.last_visit_date, f.url, null, null, null, null, ") +
        tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency, h.hidden, h.guid "
        "FROM moz_places h "
        "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
        
        "WHERE 1 "
          "{QUERY_OPTIONS_VISITS} {QUERY_OPTIONS_PLACES} "
          "{ADDITIONAL_CONDITIONS} ");
      break;

    case nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS:
      if (mResultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS) {
        
        
        
        
        mSkipOrderBy = true;

        GetTagsSqlFragment(history->GetTagsFolder(),
                           NS_LITERAL_CSTRING("b2.fk"),
                           mHasSearchTerms,
                           tagsSqlFragment);

        mQueryString = NS_LITERAL_CSTRING(
          "SELECT b2.fk, h.url, COALESCE(b2.title, h.title) AS page_title, "
            "h.rev_host, h.visit_count, h.last_visit_date, f.url, b2.id, "
            "b2.dateAdded, b2.lastModified, b2.parent, ") +
            tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency, h.hidden, h.guid, "
            "b2.guid, b2.position, b2.type, b2.fk "
          "FROM moz_bookmarks b2 "
          "JOIN (SELECT b.fk "
                "FROM moz_bookmarks b "
                
                "WHERE b.type = 1 {ADDITIONAL_CONDITIONS} "
                ") AS seed ON b2.fk = seed.fk "
          "JOIN moz_places h ON h.id = b2.fk "
          "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
          "WHERE NOT EXISTS ( "
            "SELECT id FROM moz_bookmarks WHERE id = b2.parent AND parent = ") +
                nsPrintfCString("%lld", history->GetTagsFolder()) +
          NS_LITERAL_CSTRING(") "
          "ORDER BY b2.fk DESC, b2.lastModified DESC");
      }
      else {
        GetTagsSqlFragment(history->GetTagsFolder(),
                           NS_LITERAL_CSTRING("b.fk"),
                           mHasSearchTerms,
                           tagsSqlFragment);
        mQueryString = NS_LITERAL_CSTRING(
          "SELECT b.fk, h.url, COALESCE(b.title, h.title) AS page_title, "
            "h.rev_host, h.visit_count, h.last_visit_date, f.url, b.id, "
            "b.dateAdded, b.lastModified, b.parent, ") +
            tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency, h.hidden, h.guid,"
            "b.guid, b.position, b.type, b.fk "
          "FROM moz_bookmarks b "
          "JOIN moz_places h ON b.fk = h.id "
          "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
          "WHERE NOT EXISTS "
              "(SELECT id FROM moz_bookmarks "
                "WHERE id = b.parent AND parent = ") +
                  nsPrintfCString("%lld", history->GetTagsFolder()) +
              NS_LITERAL_CSTRING(") "
            "{ADDITIONAL_CONDITIONS}");
      }
      break;

    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsVisit()
{
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  nsAutoCString tagsSqlFragment;
  GetTagsSqlFragment(history->GetTagsFolder(),
                     NS_LITERAL_CSTRING("h.id"),
                     mHasSearchTerms,
                     tagsSqlFragment);
  mQueryString = NS_LITERAL_CSTRING(
    "SELECT h.id, h.url, h.title AS page_title, h.rev_host, h.visit_count, "
      "v.visit_date, f.url, null, null, null, null, ") +
      tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency, h.hidden, h.guid "
    "FROM moz_places h "
    "JOIN moz_historyvisits v ON h.id = v.place_id "
    "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
    
    "WHERE 1 "
      "{QUERY_OPTIONS_VISITS} {QUERY_OPTIONS_PLACES} "
      "{ADDITIONAL_CONDITIONS} ");

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsDay()
{
  mSkipOrderBy = true;

  
  
  uint16_t sortingMode = nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING;
  if (mSortingMode != nsINavHistoryQueryOptions::SORT_BY_NONE &&
      mResultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY)
    sortingMode = mSortingMode;

  uint16_t resultType =
    mResultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ?
      (uint16_t)nsINavHistoryQueryOptions::RESULTS_AS_URI :
      (uint16_t)nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY;

  
  
  
  mQueryString = nsPrintfCString(
     "SELECT null, "
       "'place:type=%ld&sort=%ld&beginTime='||beginTime||'&endTime='||endTime, "
      "dayTitle, null, null, beginTime, null, null, null, null, null, null "
     "FROM (", 
     resultType,
     sortingMode);

  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  int32_t daysOfHistory = history->GetDaysOfHistory();
  for (int32_t i = 0; i <= HISTORY_DATE_CONT_NUM(daysOfHistory); i++) {
    nsAutoCString dateName;
    
    
    
    
    
    
    nsAutoCString sqlFragmentContainerBeginTime, sqlFragmentContainerEndTime;
    
    nsAutoCString sqlFragmentSearchBeginTime, sqlFragmentSearchEndTime;
    switch(i) {
       case 0:
        
         history->GetStringFromName(
          MOZ_UTF16("finduri-AgeInDays-is-0"), dateName);
        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','utc')*1000000)");
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','+1 day','utc')*1000000)");
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = sqlFragmentContainerEndTime;
         break;
       case 1:
        
         history->GetStringFromName(
          MOZ_UTF16("finduri-AgeInDays-is-1"), dateName);
        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','-1 day','utc')*1000000)");
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','utc')*1000000)");
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = sqlFragmentContainerEndTime;
        break;
      case 2:
        
        history->GetAgeInDaysString(7,
          MOZ_UTF16("finduri-AgeInDays-last-is"), dateName);
        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','-7 days','utc')*1000000)");
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','+1 day','utc')*1000000)");
        
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','-2 days','utc')*1000000)");
        break;
      case 3:
        
        history->GetStringFromName(
          MOZ_UTF16("finduri-AgeInMonths-is-0"), dateName);
        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of month','utc')*1000000)");
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','+1 day','utc')*1000000)");
        
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','-7 days','utc')*1000000)");
         break;
       default:
        if (i == HISTORY_ADDITIONAL_DATE_CONT_NUM + 6) {
          
          history->GetAgeInDaysString(6,
            MOZ_UTF16("finduri-AgeInMonths-isgreater"), dateName);
          
          sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
            "(datetime(0, 'unixepoch')*1000000)");
          
          sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
            "(strftime('%s','now','localtime','start of month','-5 months','utc')*1000000)");
          
          sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
          sqlFragmentSearchEndTime = sqlFragmentContainerEndTime;
          break;
        }
        int32_t MonthIndex = i - HISTORY_ADDITIONAL_DATE_CONT_NUM;
        
        
        PRExplodedTime tm;
        PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &tm);
        uint16_t currentYear = tm.tm_year;
        
        
        
        
        
        tm.tm_mday = 2;
        tm.tm_month -= MonthIndex;
        
        
        
        PR_NormalizeTime(&tm, PR_GMTParameters);
        
        
        
        if (tm.tm_year < currentYear) {
          history->GetMonthYear(tm.tm_month + 1, tm.tm_year, dateName);
        }
        else {
          history->GetMonthName(tm.tm_month + 1, dateName);
        }

        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of month','-");
        sqlFragmentContainerBeginTime.AppendInt(MonthIndex);
        sqlFragmentContainerBeginTime.Append(NS_LITERAL_CSTRING(
            " months','utc')*1000000)"));
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of month','-");
        sqlFragmentContainerEndTime.AppendInt(MonthIndex - 1);
        sqlFragmentContainerEndTime.Append(NS_LITERAL_CSTRING(
            " months','utc')*1000000)"));
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = sqlFragmentContainerEndTime;
        break;
    }

    nsPrintfCString dateParam("dayTitle%d", i);
    mAddParams.Put(dateParam, dateName);

    nsPrintfCString dayRange(
      "SELECT :%s AS dayTitle, "
             "%s AS beginTime, "
             "%s AS endTime "
       "WHERE EXISTS ( "
        "SELECT id FROM moz_historyvisits "
        "WHERE visit_date >= %s "
          "AND visit_date < %s "
           "AND visit_type NOT IN (0,%d,%d) "
           "{QUERY_OPTIONS_VISITS} "
         "LIMIT 1 "
      ") ",
      dateParam.get(),
      sqlFragmentContainerBeginTime.get(),
      sqlFragmentContainerEndTime.get(),
      sqlFragmentSearchBeginTime.get(),
      sqlFragmentSearchEndTime.get(),
      nsINavHistoryService::TRANSITION_EMBED,
      nsINavHistoryService::TRANSITION_FRAMED_LINK
    );

    mQueryString.Append(dayRange);

    if (i < HISTORY_DATE_CONT_NUM(daysOfHistory))
      mQueryString.AppendLiteral(" UNION ALL ");
  }

  mQueryString.AppendLiteral(") "); 

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsSite()
{
  nsAutoCString localFiles;

  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  history->GetStringFromName(MOZ_UTF16("localhost"), localFiles);
  mAddParams.Put(NS_LITERAL_CSTRING("localhost"), localFiles);

  
  nsAutoCString visitsJoin;
  nsAutoCString additionalConditions;
  nsAutoCString timeConstraints;
  if (!mConditions.IsEmpty()) {
    visitsJoin.AssignLiteral("JOIN moz_historyvisits v ON v.place_id = h.id ");
    additionalConditions.AssignLiteral("{QUERY_OPTIONS_VISITS} "
                                       "{QUERY_OPTIONS_PLACES} "
                                       "{ADDITIONAL_CONDITIONS} ");
    timeConstraints.AssignLiteral("||'&beginTime='||:begin_time||"
                                    "'&endTime='||:end_time");
  }

  mQueryString = nsPrintfCString(
    "SELECT null, 'place:type=%ld&sort=%ld&domain=&domainIsHost=true'%s, "
           ":localhost, :localhost, null, null, null, null, null, null, null "
    "WHERE EXISTS ( "
      "SELECT h.id FROM moz_places h "
      "%s "
      "WHERE h.hidden = 0 "
        "AND h.visit_count > 0 "
        "AND h.url BETWEEN 'file://' AND 'file:/~' "
      "%s "
      "LIMIT 1 "
    ") "
    "UNION ALL "
    "SELECT null, "
           "'place:type=%ld&sort=%ld&domain='||host||'&domainIsHost=true'%s, "
           "host, host, null, null, null, null, null, null, null "
    "FROM ( "
      "SELECT get_unreversed_host(h.rev_host) AS host "
      "FROM moz_places h "
      "%s "
      "WHERE h.hidden = 0 "
        "AND h.rev_host <> '.' "
        "AND h.visit_count > 0 "
        "%s "
      "GROUP BY h.rev_host "
      "ORDER BY host ASC "
    ") ",
    nsINavHistoryQueryOptions::RESULTS_AS_URI,
    mSortingMode,
    timeConstraints.get(),
    visitsJoin.get(),
    additionalConditions.get(),
    nsINavHistoryQueryOptions::RESULTS_AS_URI,
    mSortingMode,
    timeConstraints.get(),
    visitsJoin.get(),
    additionalConditions.get()
  );

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsTag()
{
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  
  
  mHasDateColumns = true; 

  mQueryString = nsPrintfCString(
    "SELECT null, 'place:folder=' || id || '&queryType=%d&type=%ld', "
           "title, null, null, null, null, null, dateAdded, "
           "lastModified, null, null, null "
    "FROM moz_bookmarks "
    "WHERE parent = %lld",
    nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS,
    nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS,
    history->GetTagsFolder()
  );

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::Where()
{

  
  nsAutoCString additionalVisitsConditions;
  nsAutoCString additionalPlacesConditions;

  if (!mIncludeHidden) {
    additionalPlacesConditions += NS_LITERAL_CSTRING("AND hidden = 0 ");
  }

  if (mQueryType == nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY) {
    
    
    additionalPlacesConditions += NS_LITERAL_CSTRING(
      "AND last_visit_date NOTNULL "
    );
  }

  if (mResultType == nsINavHistoryQueryOptions::RESULTS_AS_URI &&
      !additionalVisitsConditions.IsEmpty()) {
    
    nsAutoCString tmp = additionalVisitsConditions;
    additionalVisitsConditions = "AND EXISTS (SELECT 1 FROM moz_historyvisits WHERE place_id = h.id ";
    additionalVisitsConditions.Append(tmp);
    additionalVisitsConditions.AppendLiteral("LIMIT 1)");
  }

  mQueryString.ReplaceSubstring("{QUERY_OPTIONS_VISITS}",
                                additionalVisitsConditions.get());
  mQueryString.ReplaceSubstring("{QUERY_OPTIONS_PLACES}",
                                additionalPlacesConditions.get());

  
  
  if (mQueryString.Find("{ADDITIONAL_CONDITIONS}", 0) != kNotFound) {
    nsAutoCString innerCondition;
    
    if (!mConditions.IsEmpty()) {
      innerCondition = " AND (";
      innerCondition += mConditions;
      innerCondition += ")";
    }
    mQueryString.ReplaceSubstring("{ADDITIONAL_CONDITIONS}",
                                  innerCondition.get());

  } else if (!mConditions.IsEmpty()) {

    mQueryString += "WHERE ";
    mQueryString += mConditions;

  }
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::GroupBy()
{
  mQueryString += mGroupBy;
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::OrderBy()
{
  if (mSkipOrderBy)
    return NS_OK;

  
  
  
  switch(mSortingMode)
  {
    case nsINavHistoryQueryOptions::SORT_BY_NONE:
      
      if (mResultType == nsINavHistoryQueryOptions::RESULTS_AS_URI) {
        if (mQueryType == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS)
          mQueryString += NS_LITERAL_CSTRING(" ORDER BY b.id ASC ");
        else if (mQueryType == nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY)
          mQueryString += NS_LITERAL_CSTRING(" ORDER BY h.id ASC ");
      }
      break;
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING:
      
      
      
      
      
      
      if (mMaxResults > 0)
        OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_VisitDate);
      else if (mSortingMode == nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING)
        OrderByTextColumnIndexAsc(nsNavHistory::kGetInfoIndex_Title);
      else
        OrderByTextColumnIndexDesc(nsNavHistory::kGetInfoIndex_Title);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING:
      OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_VisitDate);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING:
      OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_VisitDate);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_URI_ASCENDING:
      OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_URL);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_URI_DESCENDING:
      OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_URL);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING:
      OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_VisitCount);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING:
      OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_VisitCount);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATEADDED_ASCENDING:
      if (mHasDateColumns)
        OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_ItemDateAdded);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATEADDED_DESCENDING:
      if (mHasDateColumns)
        OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_ItemDateAdded);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_LASTMODIFIED_ASCENDING:
      if (mHasDateColumns)
        OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_ItemLastModified);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_LASTMODIFIED_DESCENDING:
      if (mHasDateColumns)
        OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_ItemLastModified);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_TAGS_ASCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_TAGS_DESCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_ASCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_DESCENDING:
      break; 
    case nsINavHistoryQueryOptions::SORT_BY_FRECENCY_ASCENDING:
        OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_Frecency);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_FRECENCY_DESCENDING:
        OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_Frecency);
      break;
    default:
      NS_NOTREACHED("Invalid sorting mode");
  }
  return NS_OK;
}

void PlacesSQLQueryBuilder::OrderByColumnIndexAsc(int32_t aIndex)
{
  mQueryString += nsPrintfCString(" ORDER BY %d ASC", aIndex+1);
}

void PlacesSQLQueryBuilder::OrderByColumnIndexDesc(int32_t aIndex)
{
  mQueryString += nsPrintfCString(" ORDER BY %d DESC", aIndex+1);
}

void PlacesSQLQueryBuilder::OrderByTextColumnIndexAsc(int32_t aIndex)
{
  mQueryString += nsPrintfCString(" ORDER BY %d COLLATE NOCASE ASC",
                                  aIndex+1);
}

void PlacesSQLQueryBuilder::OrderByTextColumnIndexDesc(int32_t aIndex)
{
  mQueryString += nsPrintfCString(" ORDER BY %d COLLATE NOCASE DESC",
                                  aIndex+1);
}

nsresult
PlacesSQLQueryBuilder::Limit()
{
  if (mUseLimit && mMaxResults > 0) {
    mQueryString += NS_LITERAL_CSTRING(" LIMIT ");
    mQueryString.AppendInt(mMaxResults);
    mQueryString.Append(' ');
  }
  return NS_OK;
}

nsresult
nsNavHistory::ConstructQueryString(
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions, 
    nsCString& queryString, 
    bool& aParamsPresent,
    nsNavHistory::StringHash& aAddParams)
{
  
  
  
  
  
  nsresult rv;
  aParamsPresent = false;

  int32_t sortingMode = aOptions->SortingMode();
  NS_ASSERTION(sortingMode >= nsINavHistoryQueryOptions::SORT_BY_NONE &&
               sortingMode <= nsINavHistoryQueryOptions::SORT_BY_FRECENCY_DESCENDING,
               "Invalid sortingMode found while building query!");

  bool hasSearchTerms = false;
  for (int32_t i = 0; i < aQueries.Count() && !hasSearchTerms; i++) {
    aQueries[i]->GetHasSearchTerms(&hasSearchTerms);
  }

  nsAutoCString tagsSqlFragment;
  GetTagsSqlFragment(GetTagsFolder(),
                     NS_LITERAL_CSTRING("h.id"),
                     hasSearchTerms,
                     tagsSqlFragment);

  if (IsOptimizableHistoryQuery(aQueries, aOptions,
        nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING) ||
      IsOptimizableHistoryQuery(aQueries, aOptions,
        nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING)) {
    
    
    queryString = NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title AS page_title, h.rev_host, h.visit_count, h.last_visit_date, "
          "f.url, null, null, null, null, ") +
          tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency, h.hidden, h.guid "
        "FROM moz_places h "
        "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE h.hidden = 0 "
          "AND EXISTS (SELECT id FROM moz_historyvisits WHERE place_id = h.id "
                       "AND visit_type NOT IN ") +
                       nsPrintfCString("(0,%d,%d) ",
                                       nsINavHistoryService::TRANSITION_EMBED,
                                       nsINavHistoryService::TRANSITION_FRAMED_LINK) +
                       NS_LITERAL_CSTRING("LIMIT 1) "
          "{QUERY_OPTIONS} "
        );

    queryString.AppendLiteral("ORDER BY ");
    if (sortingMode == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING)
      queryString.AppendLiteral("last_visit_date DESC ");
    else
      queryString.AppendLiteral("visit_count DESC ");

    queryString.AppendLiteral("LIMIT ");
    queryString.AppendInt(aOptions->MaxResults());

    nsAutoCString additionalQueryOptions;

    queryString.ReplaceSubstring("{QUERY_OPTIONS}",
                                  additionalQueryOptions.get());
    return NS_OK;
  }

  nsAutoCString conditions;
  for (int32_t i = 0; i < aQueries.Count(); i++) {
    nsCString queryClause;
    rv = QueryToSelectClause(aQueries[i], aOptions, i, &queryClause);
    NS_ENSURE_SUCCESS(rv, rv);
    if (! queryClause.IsEmpty()) {
      aParamsPresent = true;
      if (! conditions.IsEmpty()) 
        conditions += NS_LITERAL_CSTRING(" OR ");
      conditions += NS_LITERAL_CSTRING("(") + queryClause +
        NS_LITERAL_CSTRING(")");
    }
  }

  
  
  
  bool useLimitClause = !NeedToFilterResultSet(aQueries, aOptions);

  PlacesSQLQueryBuilder queryStringBuilder(conditions, aOptions,
                                           useLimitClause, aAddParams,
                                           hasSearchTerms);
  rv = queryStringBuilder.GetQueryString(queryString);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PLDHashOperator BindAdditionalParameter(nsNavHistory::StringHash::KeyType aParamName, 
                                        nsCString aParamValue,
                                        void* aStatement)
{
  mozIStorageStatement* stmt = static_cast<mozIStorageStatement*>(aStatement);

  nsresult rv = stmt->BindUTF8StringByName(aParamName, aParamValue);
  if (NS_FAILED(rv))
    return PL_DHASH_STOP;

  return PL_DHASH_NEXT;
}















nsresult
nsNavHistory::GetQueryResults(nsNavHistoryQueryResultNode *aResultNode,
                              const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions *aOptions,
                              nsCOMArray<nsNavHistoryResultNode>* aResults)
{
  NS_ENSURE_ARG_POINTER(aOptions);
  NS_ASSERTION(aResults->Count() == 0, "Initial result array must be empty");
  if (! aQueries.Count())
    return NS_ERROR_INVALID_ARG;

  nsCString queryString;
  bool paramsPresent = false;
  nsNavHistory::StringHash addParams(HISTORY_DATE_CONT_LENGTH);
  nsresult rv = ConstructQueryString(aQueries, aOptions, queryString,
                                     paramsPresent, addParams);
  NS_ENSURE_SUCCESS(rv,rv);

  
  nsCOMPtr<mozIStorageStatement> statement = mDB->GetStatement(queryString);
#ifdef DEBUG
  if (!statement) {
    nsAutoCString lastErrorString;
    (void)mDB->MainConn()->GetLastErrorString(lastErrorString);
    int32_t lastError = 0;
    (void)mDB->MainConn()->GetLastError(&lastError);
    printf("Places failed to create a statement from this query:\n%s\nStorage error (%d): %s\n",
           queryString.get(), lastError, lastErrorString.get());
  }
#endif
  NS_ENSURE_STATE(statement);
  mozStorageStatementScoper scoper(statement);

  if (paramsPresent) {
    
    int32_t i;
    for (i = 0; i < aQueries.Count(); i++) {
      rv = BindQueryClauseParameters(statement, i, aQueries[i], aOptions);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  addParams.EnumerateRead(BindAdditionalParameter, statement.get());

  
  if (NeedToFilterResultSet(aQueries, aOptions)) {
    
    nsCOMArray<nsNavHistoryResultNode> toplevel;
    rv = ResultsAsList(statement, aOptions, &toplevel);
    NS_ENSURE_SUCCESS(rv, rv);

    FilterResultSet(aResultNode, toplevel, aResults, aQueries, aOptions);
  } else {
    rv = ResultsAsList(statement, aOptions, aResults);
    NS_ENSURE_SUCCESS(rv, rv);
  } 

  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::AddObserver(nsINavHistoryObserver* aObserver, bool aOwnsWeak)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aObserver);

  return mObservers.AppendWeakElement(aObserver, aOwnsWeak);
}

NS_IMETHODIMP
nsNavHistory::RemoveObserver(nsINavHistoryObserver* aObserver)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aObserver);

  return mObservers.RemoveWeakElement(aObserver);
}

NS_IMETHODIMP
nsNavHistory::GetObservers(uint32_t* _count,
                           nsINavHistoryObserver*** _observers)
{
  NS_ENSURE_ARG_POINTER(_count);
  NS_ENSURE_ARG_POINTER(_observers);

  *_count = 0;
  *_observers = nullptr;

  
  
  mDaysOfHistory = -1;

  if (!mCanNotify)
    return NS_OK;

  nsCOMArray<nsINavHistoryObserver> observers;

  
  mCacheObservers.GetEntries(observers);

  
  for (uint32_t i = 0; i < mObservers.Length(); ++i) {
    const nsCOMPtr<nsINavHistoryObserver> &observer = mObservers.ElementAt(i);
    
    if (observer)
      observers.AppendElement(observer);
  }

  if (observers.Count() == 0)
    return NS_OK;

  *_observers = static_cast<nsINavHistoryObserver**>
    (moz_xmalloc(observers.Count() * sizeof(nsINavHistoryObserver*)));
  NS_ENSURE_TRUE(*_observers, NS_ERROR_OUT_OF_MEMORY);

  *_count = observers.Count();
  for (uint32_t i = 0; i < *_count; ++i) {
    NS_ADDREF((*_observers)[i] = observers[i]);
  }

  return NS_OK;
}


nsresult
nsNavHistory::BeginUpdateBatch()
{
  if (mBatchLevel++ == 0) {
    mBatchDBTransaction = new mozStorageTransaction(mDB->MainConn(), false,
                                                    mozIStorageConnection::TRANSACTION_DEFERRED,
                                                    true);

    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavHistoryObserver, OnBeginUpdateBatch());
  }
  return NS_OK;
}


nsresult
nsNavHistory::EndUpdateBatch()
{
  if (--mBatchLevel == 0) {
    if (mBatchDBTransaction) {
      DebugOnly<nsresult> rv = mBatchDBTransaction->Commit();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Batch failed to commit transaction");
      delete mBatchDBTransaction;
      mBatchDBTransaction = nullptr;
    }

    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavHistoryObserver, OnEndUpdateBatch());
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::RunInBatchMode(nsINavHistoryBatchCallback* aCallback,
                             nsISupports* aUserData)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aCallback);

  UpdateBatchScoper batch(*this);
  return aCallback->RunBatched(aUserData);
}

NS_IMETHODIMP
nsNavHistory::GetHistoryDisabled(bool *_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = IsHistoryDisabled();
  return NS_OK;
}












nsresult
nsNavHistory::RemovePagesInternal(const nsCString& aPlaceIdsQueryString)
{
  
  if (aPlaceIdsQueryString.IsEmpty())
    return NS_OK;

  mozStorageTransaction transaction(mDB->MainConn(), false,
                                    mozIStorageConnection::TRANSACTION_DEFERRED,
                                    true);

  
  nsresult rv = mDB->MainConn()->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits WHERE place_id IN (") +
        aPlaceIdsQueryString +
        NS_LITERAL_CSTRING(")")
  );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CleanupPlacesOnVisitsDelete(aPlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mDaysOfHistory = -1;

  return transaction.Commit();
}













nsresult
nsNavHistory::CleanupPlacesOnVisitsDelete(const nsCString& aPlaceIdsQueryString)
{
  
  if (aPlaceIdsQueryString.IsEmpty())
    return NS_OK;

  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(NS_LITERAL_CSTRING(
    "SELECT h.id, h.url, h.guid, "
           "(SUBSTR(h.url, 1, 6) <> 'place:' "
           " AND NOT EXISTS (SELECT b.id FROM moz_bookmarks b "
                            "WHERE b.fk = h.id LIMIT 1)) as whole_entry "
    "FROM moz_places h "
    "WHERE h.id IN ( ") + aPlaceIdsQueryString + NS_LITERAL_CSTRING(")")
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsCString filteredPlaceIds;
  nsCOMArray<nsIURI> URIs;
  nsTArray<nsCString> GUIDs;
  bool hasMore;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
    int64_t placeId;
    nsresult rv = stmt->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoCString URLString;
    rv = stmt->GetUTF8String(1, URLString);
    nsCString guid;
    rv = stmt->GetUTF8String(2, guid);
    int32_t wholeEntry;
    rv = stmt->GetInt32(3, &wholeEntry);
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), URLString);
    NS_ENSURE_SUCCESS(rv, rv);
    if (wholeEntry) {
      if (!filteredPlaceIds.IsEmpty()) {
        filteredPlaceIds.Append(',');
      }
      filteredPlaceIds.AppendInt(placeId);
      URIs.AppendObject(uri);
      GUIDs.AppendElement(guid);
    }
    else {
      
      
      NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                       nsINavHistoryObserver,
                       OnDeleteVisits(uri, 0, guid, nsINavHistoryObserver::REASON_DELETED, 0));
    }
  }

  
  
  
  
  nsresult rv = mDB->MainConn()->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING(
      "DELETE FROM moz_places WHERE id IN ( "
        ) + filteredPlaceIds + NS_LITERAL_CSTRING(
      ") "
    )
  );
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = invalidateFrecencies(aPlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  for (int32_t i = 0; i < URIs.Count(); ++i) {
    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavHistoryObserver,
                     OnDeleteURI(URIs[i], GUIDs[i], nsINavHistoryObserver::REASON_DELETED));
  }

  return NS_OK;
}








NS_IMETHODIMP
nsNavHistory::RemovePages(nsIURI **aURIs, uint32_t aLength)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURIs);

  nsresult rv;
  
  nsCString deletePlaceIdsQueryString;
  for (uint32_t i = 0; i < aLength; i++) {
    int64_t placeId;
    nsAutoCString guid;
    rv = GetIdForPage(aURIs[i], &placeId, guid);
    NS_ENSURE_SUCCESS(rv, rv);
    if (placeId != 0) {
      if (!deletePlaceIdsQueryString.IsEmpty())
        deletePlaceIdsQueryString.Append(',');
      deletePlaceIdsQueryString.AppendInt(placeId);
    }
  }

  UpdateBatchScoper batch(*this); 

  rv = RemovePagesInternal(deletePlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  clearEmbedVisits();

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::RemovePage(nsIURI *aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
  int64_t placeId;
  nsAutoCString guid;
  nsresult rv = GetIdForPage(aURI, &placeId, guid);
  NS_ENSURE_SUCCESS(rv, rv);
  if (placeId == 0) {
    return NS_OK;
  }
  nsAutoCString deletePlaceIdQueryString;
  deletePlaceIdQueryString.AppendInt(placeId);

  rv = RemovePagesInternal(deletePlaceIdQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  clearEmbedVisits();

  return NS_OK;
}















NS_IMETHODIMP
nsNavHistory::RemovePagesFromHost(const nsACString& aHost, bool aEntireDomain)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsresult rv;
  
  
  if (aHost.IsEmpty())
    aEntireDomain = false;

  
  
  nsCString localFiles;
  TitleForDomain(EmptyCString(), localFiles);
  nsAutoString host16;
  if (!aHost.Equals(localFiles))
    CopyUTF8toUTF16(aHost, host16);

  
  nsAutoString revHostDot;
  GetReversedHostname(host16, revHostDot);
  NS_ASSERTION(revHostDot[revHostDot.Length() - 1] == '.', "Invalid rev. host");
  nsAutoString revHostSlash(revHostDot);
  revHostSlash.Truncate(revHostSlash.Length() - 1);
  revHostSlash.Append('/');

  
  nsAutoCString conditionString;
  if (aEntireDomain)
    conditionString.AssignLiteral("rev_host >= ?1 AND rev_host < ?2 ");
  else
    conditionString.AssignLiteral("rev_host = ?1 ");

  
  nsCOMPtr<mozIStorageStatement> statement = mDB->GetStatement(
    NS_LITERAL_CSTRING("SELECT id FROM moz_places WHERE ") + conditionString
  );
  NS_ENSURE_STATE(statement);
  mozStorageStatementScoper scoper(statement);

  rv = statement->BindStringByIndex(0, revHostDot);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aEntireDomain) {
    rv = statement->BindStringByIndex(1, revHostSlash);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCString hostPlaceIds;
  bool hasMore = false;
  while (NS_SUCCEEDED(statement->ExecuteStep(&hasMore)) && hasMore) {
    if (!hostPlaceIds.IsEmpty())
      hostPlaceIds.Append(',');
    int64_t placeId;
    rv = statement->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    hostPlaceIds.AppendInt(placeId);
  }

  
  UpdateBatchScoper batch(*this); 

  rv = RemovePagesInternal(hostPlaceIds);
  NS_ENSURE_SUCCESS(rv, rv);

  
  clearEmbedVisits();

  return NS_OK;
}










NS_IMETHODIMP
nsNavHistory::RemovePagesByTimeframe(PRTime aBeginTime, PRTime aEndTime)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsresult rv;
  
  nsCString deletePlaceIdsQueryString;

  
  
  nsCOMPtr<mozIStorageStatement> selectByTime = mDB->GetStatement(
    "SELECT h.id FROM moz_places h WHERE "
      "EXISTS "
        "(SELECT id FROM moz_historyvisits v WHERE v.place_id = h.id "
          "AND v.visit_date >= :from_date AND v.visit_date <= :to_date LIMIT 1)"
  );
  NS_ENSURE_STATE(selectByTime);
  mozStorageStatementScoper selectByTimeScoper(selectByTime);

  rv = selectByTime->BindInt64ByName(NS_LITERAL_CSTRING("from_date"), aBeginTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = selectByTime->BindInt64ByName(NS_LITERAL_CSTRING("to_date"), aEndTime);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore = false;
  while (NS_SUCCEEDED(selectByTime->ExecuteStep(&hasMore)) && hasMore) {
    int64_t placeId;
    rv = selectByTime->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    if (placeId != 0) {
      if (!deletePlaceIdsQueryString.IsEmpty())
        deletePlaceIdsQueryString.Append(',');
      deletePlaceIdsQueryString.AppendInt(placeId);
    }
  }

  
  UpdateBatchScoper batch(*this); 

  rv = RemovePagesInternal(deletePlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  clearEmbedVisits();

  return NS_OK;
}
















NS_IMETHODIMP
nsNavHistory::RemoveVisitsByTimeframe(PRTime aBeginTime, PRTime aEndTime)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsresult rv;

  
  
  
  nsCString deletePlaceIdsQueryString;
  {
    nsCOMPtr<mozIStorageStatement> selectByTime = mDB->GetStatement(
      "SELECT place_id "
      "FROM moz_historyvisits "
      "WHERE :from_date <= visit_date AND visit_date <= :to_date "
      "EXCEPT "
      "SELECT place_id "
      "FROM moz_historyvisits "
      "WHERE visit_date < :from_date OR :to_date < visit_date"
    );
    NS_ENSURE_STATE(selectByTime);
    mozStorageStatementScoper selectByTimeScoper(selectByTime);
    rv = selectByTime->BindInt64ByName(NS_LITERAL_CSTRING("from_date"), aBeginTime);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = selectByTime->BindInt64ByName(NS_LITERAL_CSTRING("to_date"), aEndTime);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore = false;
    while (NS_SUCCEEDED(selectByTime->ExecuteStep(&hasMore)) && hasMore) {
      int64_t placeId;
      rv = selectByTime->GetInt64(0, &placeId);
      NS_ENSURE_SUCCESS(rv, rv);
      
      if (placeId > 0) {
        if (!deletePlaceIdsQueryString.IsEmpty())
          deletePlaceIdsQueryString.Append(',');
        deletePlaceIdsQueryString.AppendInt(placeId);
      }
    }
  }

  
  UpdateBatchScoper batch(*this); 

  mozStorageTransaction transaction(mDB->MainConn(), false,
                                    mozIStorageConnection::TRANSACTION_DEFERRED,
                                    true);

  
  nsCOMPtr<mozIStorageStatement> deleteVisitsStmt = mDB->GetStatement(
    "DELETE FROM moz_historyvisits "
    "WHERE :from_date <= visit_date AND visit_date <= :to_date"
  );
  NS_ENSURE_STATE(deleteVisitsStmt);
  mozStorageStatementScoper deletevisitsScoper(deleteVisitsStmt);

  rv = deleteVisitsStmt->BindInt64ByName(NS_LITERAL_CSTRING("from_date"), aBeginTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteVisitsStmt->BindInt64ByName(NS_LITERAL_CSTRING("to_date"), aEndTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteVisitsStmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CleanupPlacesOnVisitsDelete(deletePlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  clearEmbedVisits();

  
  mDaysOfHistory = -1;

  return NS_OK;
}






NS_IMETHODIMP
nsNavHistory::RemoveAllPages()
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsresult rv = mDB->MainConn()->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_historyvisits"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  clearEmbedVisits();

  
  mDaysOfHistory = 0;

  
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver, OnClearHistory());

  
  
  rv = invalidateFrecencies(EmptyCString());
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to fix invalid frecencies");

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::MarkPageAsTyped(nsIURI *aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
  if (IsHistoryDisabled())
    return NS_OK;

  nsAutoCString uriString;
  nsresult rv = aURI->GetSpec(uriString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int64_t unusedEventTime;
  if (mRecentTyped.Get(uriString, &unusedEventTime))
    mRecentTyped.Remove(uriString);

  if (mRecentTyped.Count() > RECENT_EVENT_QUEUE_MAX_LENGTH)
    ExpireNonrecentEvents(&mRecentTyped);

  mRecentTyped.Put(uriString, GetNow());
  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::MarkPageAsFollowedLink(nsIURI *aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
  if (IsHistoryDisabled())
    return NS_OK;

  nsAutoCString uriString;
  nsresult rv = aURI->GetSpec(uriString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int64_t unusedEventTime;
  if (mRecentLink.Get(uriString, &unusedEventTime))
    mRecentLink.Remove(uriString);

  if (mRecentLink.Count() > RECENT_EVENT_QUEUE_MAX_LENGTH)
    ExpireNonrecentEvents(&mRecentLink);

  mRecentLink.Put(uriString, GetNow());
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistory::GetPageTitle(nsIURI* aURI, nsAString& aTitle)
{
  PLACES_WARN_DEPRECATED();

  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  aTitle.Truncate(0);

  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(
    "SELECT id, url, title, rev_host, visit_count, guid "
    "FROM moz_places "
    "WHERE url = :page_url "
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasResults = false;
  rv = stmt->ExecuteStep(&hasResults);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasResults) {
    aTitle.SetIsVoid(true);
    return NS_OK; 
  }

  rv = stmt->GetString(2, aTitle);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





NS_IMETHODIMP
nsNavHistory::GetDatabaseConnection(mozIStorageConnection** _DBConnection)
{
  return GetDBConnection(_DBConnection);
}


NS_IMETHODIMP
nsNavHistory::GetExpectedDatabasePageSize(int32_t* _expectedPageSize)
{
  NS_ENSURE_STATE(mDB);
  NS_ENSURE_STATE(mDB->MainConn());
  return mDB->MainConn()->GetDefaultPageSize(_expectedPageSize);
}


NS_IMETHODIMP
nsNavHistory::OnBeginVacuum(bool* _vacuumGranted)
{
  
  
  *_vacuumGranted = true;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistory::OnEndVacuum(bool aSucceeded)
{
  NS_WARN_IF_FALSE(aSucceeded, "Places.sqlite vacuum failed.");
  return NS_OK;
}





NS_IMETHODIMP
nsNavHistory::GetDBConnection(mozIStorageConnection **_DBConnection)
{
  NS_ENSURE_ARG_POINTER(_DBConnection);
  nsRefPtr<mozIStorageConnection> connection = mDB->MainConn();
  connection.forget(_DBConnection);

  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::AsyncExecuteLegacyQueries(nsINavHistoryQuery** aQueries,
                                        uint32_t aQueryCount,
                                        nsINavHistoryQueryOptions* aOptions,
                                        mozIStorageStatementCallback* aCallback,
                                        mozIStoragePendingStatement** _stmt)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aQueries);
  NS_ENSURE_ARG(aOptions);
  NS_ENSURE_ARG(aCallback);
  NS_ENSURE_ARG_POINTER(_stmt);

  nsCOMArray<nsNavHistoryQuery> queries;
  for (uint32_t i = 0; i < aQueryCount; i ++) {
    nsCOMPtr<nsNavHistoryQuery> query = do_QueryInterface(aQueries[i]);
    NS_ENSURE_STATE(query);
    queries.AppendObject(query);
  }
  NS_ENSURE_ARG_MIN(queries.Count(), 1);

  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(aOptions);
  NS_ENSURE_ARG(options);

  nsCString queryString;
  bool paramsPresent = false;
  nsNavHistory::StringHash addParams(HISTORY_DATE_CONT_LENGTH);
  nsresult rv = ConstructQueryString(queries, options, queryString,
                                     paramsPresent, addParams);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<mozIStorageAsyncStatement> statement =
    mDB->GetAsyncStatement(queryString);
  NS_ENSURE_STATE(statement);

#ifdef DEBUG
  if (NS_FAILED(rv)) {
    nsAutoCString lastErrorString;
    (void)mDB->MainConn()->GetLastErrorString(lastErrorString);
    int32_t lastError = 0;
    (void)mDB->MainConn()->GetLastError(&lastError);
    printf("Places failed to create a statement from this query:\n%s\nStorage error (%d): %s\n",
           queryString.get(), lastError, lastErrorString.get());
  }
#endif
  NS_ENSURE_SUCCESS(rv, rv);

  if (paramsPresent) {
    
    int32_t i;
    for (i = 0; i < queries.Count(); i++) {
      rv = BindQueryClauseParameters(statement, i, queries[i], options);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  addParams.EnumerateRead(BindAdditionalParameter, statement.get());

  rv = statement->ExecuteAsync(aCallback, _stmt);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsNavHistory::NotifyOnPageExpired(nsIURI *aURI, PRTime aVisitTime,
                                  bool aWholeEntry, const nsACString& aGUID,
                                  uint16_t aReason, uint32_t aTransitionType)
{
  
  mDaysOfHistory = -1;

  MOZ_ASSERT(!aGUID.IsEmpty());
  if (aWholeEntry) {
    
    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavHistoryObserver, OnDeleteURI(aURI, aGUID, aReason));
  }
  else {
    
    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavHistoryObserver,
                     OnDeleteVisits(aURI, aVisitTime, aGUID, aReason,
                                    aTransitionType));
  }

  return NS_OK;
}




NS_IMETHODIMP
nsNavHistory::Observe(nsISupports *aSubject, const char *aTopic,
                    const char16_t *aData)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  if (strcmp(aTopic, TOPIC_PROFILE_TEARDOWN) == 0 ||
      strcmp(aTopic, TOPIC_PROFILE_CHANGE) == 0) {
    
    
    mDB->Observe(aSubject, aTopic, aData);
  }

  else if (strcmp(aTopic, TOPIC_PLACES_CONNECTION_CLOSED) == 0) {
      
      
      mCanNotify = false;
  }

#ifdef MOZ_XUL
  else if (strcmp(aTopic, TOPIC_AUTOCOMPLETE_FEEDBACK_INCOMING) == 0) {
    nsCOMPtr<nsIAutoCompleteInput> input = do_QueryInterface(aSubject);
    if (!input)
      return NS_OK;

    
    bool isPrivate;
    nsresult rv = input->GetInPrivateContext(&isPrivate);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isPrivate)
      return NS_OK;

    nsCOMPtr<nsIAutoCompletePopup> popup;
    input->GetPopup(getter_AddRefs(popup));
    if (!popup)
      return NS_OK;

    nsCOMPtr<nsIAutoCompleteController> controller;
    input->GetController(getter_AddRefs(controller));
    if (!controller)
      return NS_OK;

    
    bool open;
    rv = popup->GetPopupOpen(&open);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!open)
      return NS_OK;

    
    int32_t selectedIndex;
    rv = popup->GetSelectedIndex(&selectedIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    if (selectedIndex == -1)
      return NS_OK;

    rv = AutoCompleteFeedback(selectedIndex, controller);
    NS_ENSURE_SUCCESS(rv, rv);
  }

#endif
  else if (strcmp(aTopic, TOPIC_PREF_CHANGED) == 0) {
    LoadPrefs();
  }

  else if (strcmp(aTopic, TOPIC_IDLE_DAILY) == 0) {
    (void)DecayFrecency();
  }

  return NS_OK;
}


namespace {

class DecayFrecencyCallback : public AsyncStatementTelemetryTimer
{
public:
  DecayFrecencyCallback()
    : AsyncStatementTelemetryTimer(Telemetry::PLACES_IDLE_FRECENCY_DECAY_TIME_MS)
  {
  }

  NS_IMETHOD HandleCompletion(uint16_t aReason)
  {
    (void)AsyncStatementTelemetryTimer::HandleCompletion(aReason);
    if (aReason == REASON_FINISHED) {
      nsNavHistory *navHistory = nsNavHistory::GetHistoryService();
      NS_ENSURE_STATE(navHistory);
      navHistory->NotifyManyFrecenciesChanged();
    }
    return NS_OK;
  }
};

} 

nsresult
nsNavHistory::DecayFrecency()
{
  nsresult rv = FixInvalidFrecencies();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  nsCOMPtr<mozIStorageAsyncStatement> decayFrecency = mDB->GetAsyncStatement(
    "UPDATE moz_places SET frecency = ROUND(frecency * .975) "
    "WHERE frecency > 0"
  );
  NS_ENSURE_STATE(decayFrecency);

  
  
  nsCOMPtr<mozIStorageAsyncStatement> decayAdaptive = mDB->GetAsyncStatement(
    "UPDATE moz_inputhistory SET use_count = use_count * .975"
  );
  NS_ENSURE_STATE(decayAdaptive);

  
  nsCOMPtr<mozIStorageAsyncStatement> deleteAdaptive = mDB->GetAsyncStatement(
    "DELETE FROM moz_inputhistory WHERE use_count < .01"
  );
  NS_ENSURE_STATE(deleteAdaptive);

  mozIStorageBaseStatement *stmts[] = {
    decayFrecency.get(),
    decayAdaptive.get(),
    deleteAdaptive.get()
  };
  nsCOMPtr<mozIStoragePendingStatement> ps;
  nsRefPtr<DecayFrecencyCallback> cb = new DecayFrecencyCallback();
  rv = mDB->MainConn()->ExecuteAsync(stmts, ArrayLength(stmts), cb,
                                     getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}











class ConditionBuilder
{
public:

  explicit ConditionBuilder(int32_t aQueryIndex): mQueryIndex(aQueryIndex)
  { }

  ConditionBuilder& Condition(const char* aStr)
  {
    if (!mClause.IsEmpty())
      mClause.AppendLiteral(" AND ");
    Str(aStr);
    return *this;
  }

  ConditionBuilder& Str(const char* aStr)
  {
    mClause.Append(' ');
    mClause.Append(aStr);
    mClause.Append(' ');
    return *this;
  }

  ConditionBuilder& Param(const char* aParam)
  {
    mClause.Append(' ');
    if (!mQueryIndex)
      mClause.Append(aParam);
    else
      mClause += nsPrintfCString("%s%d", aParam, mQueryIndex);

    mClause.Append(' ');
    return *this;
  }

  void GetClauseString(nsCString& aResult) 
  {
    aResult = mClause;
  }

private:

  int32_t mQueryIndex;
  nsCString mClause;
};









nsresult
nsNavHistory::QueryToSelectClause(nsNavHistoryQuery* aQuery, 
                                  nsNavHistoryQueryOptions* aOptions,
                                  int32_t aQueryIndex,
                                  nsCString* aClause)
{
  bool hasIt;
  bool excludeQueries = aOptions->ExcludeQueries();

  ConditionBuilder clause(aQueryIndex);

  if ((NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) ||
    (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt)) {
    clause.Condition("EXISTS (SELECT 1 FROM moz_historyvisits "
                              "WHERE place_id = h.id");
    
    if (NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) 
      clause.Condition("visit_date >=").Param(":begin_time");
    
    if (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt)
      clause.Condition("visit_date <=").Param(":end_time");
    clause.Str(" LIMIT 1)");
  }

  
  bool hasSearchTerms;
  int32_t searchBehavior = mozIPlacesAutoComplete::BEHAVIOR_HISTORY |
                           mozIPlacesAutoComplete::BEHAVIOR_BOOKMARK;
  if (NS_SUCCEEDED(aQuery->GetHasSearchTerms(&hasSearchTerms)) && hasSearchTerms) {
    
    
    
    clause.Condition("AUTOCOMPLETE_MATCH(").Param(":search_string")
          .Str(", h.url, page_title, tags, ")
          .Str(nsPrintfCString("1, 1, 1, 1, %d, %d)",
                               mozIPlacesAutoComplete::MATCH_ANYWHERE_UNMODIFIED,
                               searchBehavior).get());
    
    excludeQueries = true;
  }

  
  if (aQuery->MinVisits() >= 0)
    clause.Condition("h.visit_count >=").Param(":min_visits");

  if (aQuery->MaxVisits() >= 0)
    clause.Condition("h.visit_count <=").Param(":max_visits");
  
  
  if (aOptions->QueryType() != nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS &&
      aQuery->OnlyBookmarked())
    clause.Condition("EXISTS (SELECT b.fk FROM moz_bookmarks b WHERE b.type = ")
          .Str(nsPrintfCString("%d", nsNavBookmarks::TYPE_BOOKMARK).get())
          .Str("AND b.fk = h.id)");

  
  if (NS_SUCCEEDED(aQuery->GetHasDomain(&hasIt)) && hasIt) {
    bool domainIsHost = false;
    aQuery->GetDomainIsHost(&domainIsHost);
    if (domainIsHost)
      clause.Condition("h.rev_host =").Param(":domain_lower");
    else
      
      clause.Condition("h.rev_host >=").Param(":domain_lower")
            .Condition("h.rev_host <").Param(":domain_upper");
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasUri(&hasIt)) && hasIt) {
    if (aQuery->UriIsPrefix()) {
      clause.Condition("h.url >= ").Param(":uri")
            .Condition("h.url <= ").Param(":uri_upper");
    }
    else
      clause.Condition("h.url =").Param(":uri");
  }

  
  aQuery->GetHasAnnotation(&hasIt);
  if (hasIt) {
    clause.Condition("");
    if (aQuery->AnnotationIsNot())
      clause.Str("NOT");
    clause.Str(
      "EXISTS "
        "(SELECT h.id "
         "FROM moz_annos anno "
         "JOIN moz_anno_attributes annoname "
           "ON anno.anno_attribute_id = annoname.id "
         "WHERE anno.place_id = h.id "
           "AND annoname.name = ").Param(":anno").Str(")");
    
    
  }

  
  const nsTArray<nsString> &tags = aQuery->Tags();
  if (tags.Length() > 0) {
    clause.Condition("h.id");
    if (aQuery->TagsAreNot())
      clause.Str("NOT");
    clause.Str(
      "IN "
        "(SELECT bms.fk "
         "FROM moz_bookmarks bms "
         "JOIN moz_bookmarks tags ON bms.parent = tags.id "
         "WHERE tags.parent =").
           Param(":tags_folder").
           Str("AND tags.title IN (");
    for (uint32_t i = 0; i < tags.Length(); ++i) {
      nsPrintfCString param(":tag%d_", i);
      clause.Param(param.get());
      if (i < tags.Length() - 1)
        clause.Str(",");
    }
    clause.Str(")");
    if (!aQuery->TagsAreNot())
      clause.Str("GROUP BY bms.fk HAVING count(*) >=").Param(":tag_count");
    clause.Str(")");
  }

  
  const nsTArray<uint32_t>& transitions = aQuery->Transitions();
  for (uint32_t i = 0; i < transitions.Length(); ++i) {
    nsPrintfCString param(":transition%d_", i);
    clause.Condition("h.id IN (SELECT place_id FROM moz_historyvisits "
                             "WHERE visit_type = ")
          .Param(param.get())
          .Str(")");
  }

  
  const nsTArray<int64_t>& folders = aQuery->Folders();
  if (folders.Length() > 0) {
    aOptions->SetQueryType(nsNavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS);

    nsTArray<int64_t> includeFolders;
    includeFolders.AppendElements(folders);

    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_STATE(bookmarks);

    for (nsTArray<int64_t>::size_type i = 0; i < folders.Length(); ++i) {
      nsTArray<int64_t> subFolders;
      if (NS_FAILED(bookmarks->GetDescendantFolders(folders[i], subFolders)))
        continue;
      includeFolders.AppendElements(subFolders);
    }

    clause.Condition("b.parent IN(");
    for (nsTArray<int64_t>::size_type i = 0; i < includeFolders.Length(); ++i) {
      clause.Str(nsPrintfCString("%lld", includeFolders[i]).get());
      if (i < includeFolders.Length() - 1) {
        clause.Str(",");
      }
    }
    clause.Str(")");
  }

  if (excludeQueries) {
    
    clause.Condition("NOT h.url BETWEEN 'place:' AND 'place;'");
  }

  clause.GetClauseString(*aClause);
  return NS_OK;
}






nsresult
nsNavHistory::BindQueryClauseParameters(mozIStorageBaseStatement* statement,
                                        int32_t aQueryIndex,
                                        nsNavHistoryQuery* aQuery, 
                                        nsNavHistoryQueryOptions* aOptions)
{
  nsresult rv;

  bool hasIt;
  
  
  
  nsAutoCString qIndex;
  if (aQueryIndex > 0)
    qIndex.AppendInt(aQueryIndex);

  
  if (NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) {
    PRTime time = NormalizeTime(aQuery->BeginTimeReference(),
                                aQuery->BeginTime());
    rv = statement->BindInt64ByName(
      NS_LITERAL_CSTRING("begin_time") + qIndex, time);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt) {
    PRTime time = NormalizeTime(aQuery->EndTimeReference(),
                                aQuery->EndTime());
    rv = statement->BindInt64ByName(
      NS_LITERAL_CSTRING("end_time") + qIndex, time
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasSearchTerms(&hasIt)) && hasIt) {
    rv = statement->BindStringByName(
      NS_LITERAL_CSTRING("search_string") + qIndex,
      aQuery->SearchTerms()
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  int32_t visits = aQuery->MinVisits();
  if (visits >= 0) {
    rv = statement->BindInt32ByName(
      NS_LITERAL_CSTRING("min_visits") + qIndex, visits
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  visits = aQuery->MaxVisits();
  if (visits >= 0) {
    rv = statement->BindInt32ByName(
      NS_LITERAL_CSTRING("max_visits") + qIndex, visits
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasDomain(&hasIt)) && hasIt) {
    nsString revDomain;
    GetReversedHostname(NS_ConvertUTF8toUTF16(aQuery->Domain()), revDomain);

    if (aQuery->DomainIsHost()) {
      rv = statement->BindStringByName(
        NS_LITERAL_CSTRING("domain_lower") + qIndex, revDomain
      );
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      
      
      
      NS_ASSERTION(revDomain[revDomain.Length() - 1] == '.', "Invalid rev. host");
      rv = statement->BindStringByName(
        NS_LITERAL_CSTRING("domain_lower") + qIndex, revDomain
      );
      NS_ENSURE_SUCCESS(rv, rv);
      revDomain.Truncate(revDomain.Length() - 1);
      revDomain.Append(char16_t('/'));
      rv = statement->BindStringByName(
        NS_LITERAL_CSTRING("domain_upper") + qIndex, revDomain
      );
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  if (aQuery->Uri()) {
    rv = URIBinder::Bind(
      statement, NS_LITERAL_CSTRING("uri") + qIndex, aQuery->Uri()
    );
    NS_ENSURE_SUCCESS(rv, rv);
    if (aQuery->UriIsPrefix()) {
      nsAutoCString uriString;
      aQuery->Uri()->GetSpec(uriString);
      uriString.Append(char(0x7F)); 
      rv = URIBinder::Bind(
        statement, NS_LITERAL_CSTRING("uri_upper") + qIndex, uriString
      );
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  if (!aQuery->Annotation().IsEmpty()) {
    rv = statement->BindUTF8StringByName(
      NS_LITERAL_CSTRING("anno") + qIndex, aQuery->Annotation()
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  const nsTArray<nsString> &tags = aQuery->Tags();
  if (tags.Length() > 0) {
    for (uint32_t i = 0; i < tags.Length(); ++i) {
      nsPrintfCString paramName("tag%d_", i);
      NS_ConvertUTF16toUTF8 tag(tags[i]);
      rv = statement->BindUTF8StringByName(paramName + qIndex, tag);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    int64_t tagsFolder = GetTagsFolder();
    rv = statement->BindInt64ByName(
      NS_LITERAL_CSTRING("tags_folder") + qIndex, tagsFolder
    );
    NS_ENSURE_SUCCESS(rv, rv);
    if (!aQuery->TagsAreNot()) {
      rv = statement->BindInt32ByName(
        NS_LITERAL_CSTRING("tag_count") + qIndex, tags.Length()
      );
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  const nsTArray<uint32_t>& transitions = aQuery->Transitions();
  if (transitions.Length() > 0) {
    for (uint32_t i = 0; i < transitions.Length(); ++i) {
      nsPrintfCString paramName("transition%d_", i);
      rv = statement->BindInt64ByName(paramName + qIndex, transitions[i]);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}





nsresult
nsNavHistory::ResultsAsList(mozIStorageStatement* statement,
                            nsNavHistoryQueryOptions* aOptions,
                            nsCOMArray<nsNavHistoryResultNode>* aResults)
{
  nsresult rv;
  nsCOMPtr<mozIStorageValueArray> row = do_QueryInterface(statement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore = false;
  while (NS_SUCCEEDED(statement->ExecuteStep(&hasMore)) && hasMore) {
    nsRefPtr<nsNavHistoryResultNode> result;
    rv = RowToResult(row, aOptions, getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);
    aResults->AppendObject(result);
  }
  return NS_OK;
}

const int64_t UNDEFINED_URN_VALUE = -1;




nsresult
CreatePlacesPersistURN(nsNavHistoryQueryResultNode *aResultNode, 
                       int64_t aValue, const nsCString& aTitle, nsCString& aURN)
{
  nsAutoCString uri;
  nsresult rv = aResultNode->GetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);

  aURN.AssignLiteral("urn:places-persist:");
  aURN.Append(uri);

  aURN.Append(',');
  if (aValue != UNDEFINED_URN_VALUE)
    aURN.AppendInt(aValue);

  aURN.Append(',');
  if (!aTitle.IsEmpty()) {
    nsAutoCString escapedTitle;
    bool success = NS_Escape(aTitle, escapedTitle, url_XAlphas);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    aURN.Append(escapedTitle);
  }

  return NS_OK;
}

int64_t
nsNavHistory::GetTagsFolder()
{
  
  
  
  if (mTagsFolder == -1) {
    nsNavBookmarks *bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, -1);
    
    nsresult rv = bookmarks->GetTagsFolder(&mTagsFolder);
    NS_ENSURE_SUCCESS(rv, -1);
  }
  return mTagsFolder;
}










nsresult
nsNavHistory::FilterResultSet(nsNavHistoryQueryResultNode* aQueryNode,
                              const nsCOMArray<nsNavHistoryResultNode>& aSet,
                              nsCOMArray<nsNavHistoryResultNode>* aFiltered,
                              const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions *aOptions)
{
  
  nsNavBookmarks *bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  
  nsTArray<nsTArray<nsString>*> terms;
  ParseSearchTermsFromQueries(aQueries, &terms);

  uint16_t resultType = aOptions->ResultType();
  for (int32_t nodeIndex = 0; nodeIndex < aSet.Count(); nodeIndex++) {
    
    
    if (!aSet[nodeIndex]->IsURI())
      continue;

    
    
    
    if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS &&
        nodeIndex > 0 && aSet[nodeIndex]->mURI == aSet[nodeIndex-1]->mURI)
      continue;

    if (aSet[nodeIndex]->mItemId != -1 && aQueryNode &&
        aQueryNode->mItemId == aSet[nodeIndex]->mItemId) {
      continue;
    }

    
    bool appendNode = false;
    for (int32_t queryIndex = 0;
         queryIndex < aQueries.Count() && !appendNode; queryIndex++) {

      if (terms[queryIndex]->Length()) {
        
        
        NS_ConvertUTF8toUTF16 nodeTitle(aSet[nodeIndex]->mTitle);
        
        nsAutoCString cNodeURL(aSet[nodeIndex]->mURI);
        NS_ConvertUTF8toUTF16 nodeURL(NS_UnescapeURL(cNodeURL));

        
        
        bool matchAll = true;
        for (int32_t termIndex = terms[queryIndex]->Length() - 1;
             termIndex >= 0 && matchAll;
             termIndex--) {
          nsString& term = terms[queryIndex]->ElementAt(termIndex);

          
          matchAll = CaseInsensitiveFindInReadable(term, nodeTitle) ||
                     CaseInsensitiveFindInReadable(term, nodeURL) ||
                     CaseInsensitiveFindInReadable(term, aSet[nodeIndex]->mTags);
        }

        
        if (!matchAll)
          continue;
      }

      
      appendNode = true;
    }

    if (appendNode)
      aFiltered->AppendObject(aSet[nodeIndex]);

    
    if (aOptions->MaxResults() > 0 &&
        (uint32_t)aFiltered->Count() >= aOptions->MaxResults())
      break;
  }

  
  for (int32_t i = 0; i < aQueries.Count(); i++) {
    delete terms[i];
  }

  return NS_OK;
}

void
nsNavHistory::registerEmbedVisit(nsIURI* aURI,
                                 int64_t aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  VisitHashKey* visit = mEmbedVisits.PutEntry(aURI);
  if (!visit) {
    NS_WARNING("Unable to register a EMBED visit.");
    return;
  }
  visit->visitTime = aTime;
}

bool
nsNavHistory::hasEmbedVisit(nsIURI* aURI) {
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  return !!mEmbedVisits.GetEntry(aURI);
}

void
nsNavHistory::clearEmbedVisits() {
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  mEmbedVisits.Clear();
}

NS_IMETHODIMP
nsNavHistory::ClearEmbedVisits() {
  clearEmbedVisits();
  return NS_OK;
}









bool
nsNavHistory::CheckIsRecentEvent(RecentEventHash* hashTable,
                                 const nsACString& url)
{
  PRTime eventTime;
  if (hashTable->Get(url, reinterpret_cast<int64_t*>(&eventTime))) {
    hashTable->Remove(url);
    if (eventTime > GetNow() - RECENT_EVENT_THRESHOLD)
      return true;
    return false;
  }
  return false;
}






static PLDHashOperator
ExpireNonrecentEventsCallback(nsCStringHashKey::KeyType aKey,
                              int64_t& aData,
                              void* userArg)
{
  int64_t* threshold = reinterpret_cast<int64_t*>(userArg);
  if (aData < *threshold)
    return PL_DHASH_REMOVE;
  return PL_DHASH_NEXT;
}
void
nsNavHistory::ExpireNonrecentEvents(RecentEventHash* hashTable)
{
  int64_t threshold = GetNow() - RECENT_EVENT_THRESHOLD;
  hashTable->Enumerate(ExpireNonrecentEventsCallback,
                       reinterpret_cast<void*>(&threshold));
}







nsresult
nsNavHistory::RowToResult(mozIStorageValueArray* aRow,
                          nsNavHistoryQueryOptions* aOptions,
                          nsNavHistoryResultNode** aResult)
{
  NS_ASSERTION(aRow && aOptions && aResult, "Null pointer in RowToResult");

  
  nsAutoCString url;
  nsresult rv = aRow->GetUTF8String(kGetInfoIndex_URL, url);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoCString title;
  rv = aRow->GetUTF8String(kGetInfoIndex_Title, title);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t accessCount = aRow->AsInt32(kGetInfoIndex_VisitCount);
  PRTime time = aRow->AsInt64(kGetInfoIndex_VisitDate);

  
  nsAutoCString favicon;
  rv = aRow->GetUTF8String(kGetInfoIndex_FaviconURL, favicon);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int64_t itemId = aRow->AsInt64(kGetInfoIndex_ItemId);
  int64_t parentId = -1;
  if (itemId == 0) {
    
    
    itemId = -1;
  }
  else {
    
    int64_t itemParentId = aRow->AsInt64(kGetInfoIndex_ItemParentId);
    if (itemParentId > 0) {
      
      
      parentId = itemParentId;
    }
  }

  if (IsQueryURI(url)) {
    
    if (itemId != -1) {
      
      
      
      
      
      nsNavBookmarks *bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

      rv = bookmarks->GetItemTitle(itemId, title);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsAutoCString guid;
    if (itemId != -1) {
      rv = aRow->GetUTF8String(nsNavBookmarks::kGetChildrenIndex_Guid, guid);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsRefPtr<nsNavHistoryResultNode> resultNode;
    rv = QueryRowToResult(itemId, guid, url, title, accessCount, time, favicon,
                          getter_AddRefs(resultNode));
    NS_ENSURE_SUCCESS(rv, rv);

    if (itemId != -1 ||
        aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_TAG_QUERY) {
      
      resultNode->mDateAdded = aRow->AsInt64(kGetInfoIndex_ItemDateAdded);
      resultNode->mLastModified = aRow->AsInt64(kGetInfoIndex_ItemLastModified);
      if (resultNode->IsFolder()) {
        
        
        
        resultNode->GetAsContainer()->mOptions = aOptions;
      }
    }

    resultNode.forget(aResult);
    return rv;
  } else if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_URI ||
             aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS) {
    nsRefPtr<nsNavHistoryResultNode> resultNode =
      new nsNavHistoryResultNode(url, title, accessCount, time, favicon);

    if (itemId != -1) {
      resultNode->mItemId = itemId;
      resultNode->mFolderId = parentId;
      resultNode->mDateAdded = aRow->AsInt64(kGetInfoIndex_ItemDateAdded);
      resultNode->mLastModified = aRow->AsInt64(kGetInfoIndex_ItemLastModified);

      rv = aRow->GetUTF8String(nsNavBookmarks::kGetChildrenIndex_Guid,
                               resultNode->mBookmarkGuid);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    resultNode->mFrecency = aRow->AsInt32(kGetInfoIndex_Frecency);
    resultNode->mHidden = !!aRow->AsInt32(kGetInfoIndex_Hidden);

    nsAutoString tags;
    rv = aRow->GetString(kGetInfoIndex_ItemTags, tags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!tags.IsVoid()) {
      resultNode->mTags.Assign(tags);
    }

    rv = aRow->GetUTF8String(kGetInfoIndex_Guid, resultNode->mPageGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    resultNode.forget(aResult);
    return NS_OK;
  }

  if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_VISIT) {
    nsRefPtr<nsNavHistoryResultNode> resultNode =
      new nsNavHistoryResultNode(url, title, accessCount, time, favicon);

    nsAutoString tags;
    rv = aRow->GetString(kGetInfoIndex_ItemTags, tags);
    if (!tags.IsVoid())
      resultNode->mTags.Assign(tags);

    rv = aRow->GetUTF8String(kGetInfoIndex_Guid, resultNode->mPageGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    resultNode.forget(aResult);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}







nsresult
nsNavHistory::QueryRowToResult(int64_t itemId,
                               const nsACString& aBookmarkGuid,
                               const nsACString& aURI,
                               const nsACString& aTitle,
                               uint32_t aAccessCount, PRTime aTime,
                               const nsACString& aFavicon,
                               nsNavHistoryResultNode** aNode)
{
  MOZ_ASSERT((itemId != -1 && !aBookmarkGuid.IsEmpty()) ||
             (itemId == -1 && aBookmarkGuid.IsEmpty()));

  nsCOMArray<nsNavHistoryQuery> queries;
  nsCOMPtr<nsNavHistoryQueryOptions> options;
  nsresult rv = QueryStringToQueryArray(aURI, &queries,
                                        getter_AddRefs(options));

  nsRefPtr<nsNavHistoryResultNode> resultNode;
  
  
  if (NS_SUCCEEDED(rv)) {
    
    int64_t targetFolderId = GetSimpleBookmarksQueryFolder(queries, options);
    if (targetFolderId) {
      nsNavBookmarks *bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

      rv = bookmarks->ResultNodeForContainer(targetFolderId, options,
                                             getter_AddRefs(resultNode));
      
      
      if (NS_SUCCEEDED(rv)) {
        
        
        resultNode->GetAsFolder()->mTargetFolderItemId = targetFolderId;
        resultNode->mItemId = itemId;
        nsAutoCString targetFolderGuid(resultNode->GetAsFolder()->mBookmarkGuid);
        resultNode->mBookmarkGuid = aBookmarkGuid;
        resultNode->GetAsFolder()->mTargetFolderGuid = targetFolderGuid;

        
        
        if (!aTitle.IsVoid()) {
          resultNode->mTitle = aTitle;
        }
      }
    }
    else {
      
      resultNode = new nsNavHistoryQueryResultNode(aTitle, EmptyCString(),
                                                   aTime, queries, options);
      resultNode->mItemId = itemId;
    }
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("Generating a generic empty node for a broken query!");
    
    
    
    resultNode = new nsNavHistoryQueryResultNode(aTitle, aFavicon, aURI);
    resultNode->mItemId = itemId;
    
    resultNode->GetAsQuery()->Options()->SetExcludeItems(true);
  }

  resultNode.forget(aNode);
  return NS_OK;
}







nsresult
nsNavHistory::VisitIdToResultNode(int64_t visitId,
                                  nsNavHistoryQueryOptions* aOptions,
                                  nsNavHistoryResultNode** aResult)
{
  nsAutoCString tagsFragment;
  GetTagsSqlFragment(GetTagsFolder(), NS_LITERAL_CSTRING("h.id"),
                     true, tagsFragment);

  nsCOMPtr<mozIStorageStatement> statement;
  switch (aOptions->ResultType())
  {
    case nsNavHistoryQueryOptions::RESULTS_AS_VISIT:
    case nsNavHistoryQueryOptions::RESULTS_AS_FULL_VISIT:
      
      
      statement = mDB->GetStatement(NS_LITERAL_CSTRING(
        "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
               "v.visit_date, f.url, null, null, null, null, "
               ) + tagsFragment + NS_LITERAL_CSTRING(", h.frecency, h.hidden, h.guid "
        "FROM moz_places h "
        "JOIN moz_historyvisits v ON h.id = v.place_id "
        "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE v.id = :visit_id ")
      );
      break;

    case nsNavHistoryQueryOptions::RESULTS_AS_URI:
      
      
      statement = mDB->GetStatement(NS_LITERAL_CSTRING(
        "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
               "h.last_visit_date, f.url, null, null, null, null, "
               ) + tagsFragment + NS_LITERAL_CSTRING(", h.frecency, h.hidden, h.guid "
        "FROM moz_places h "
        "JOIN moz_historyvisits v ON h.id = v.place_id "
        "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE v.id = :visit_id ")
      );
      break;

    default:
      
      
      return NS_OK;
  }
  NS_ENSURE_STATE(statement);
  mozStorageStatementScoper scoper(statement);

  nsresult rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("visit_id"),
                                           visitId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore = false;
  rv = statement->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! hasMore) {
    NS_NOTREACHED("Trying to get a result node for an invalid visit");
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<mozIStorageValueArray> row = do_QueryInterface(statement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return RowToResult(row, aOptions, aResult);
}

nsresult
nsNavHistory::BookmarkIdToResultNode(int64_t aBookmarkId, nsNavHistoryQueryOptions* aOptions,
                                     nsNavHistoryResultNode** aResult)
{
  nsAutoCString tagsFragment;
  GetTagsSqlFragment(GetTagsFolder(), NS_LITERAL_CSTRING("h.id"),
                     true, tagsFragment);
  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(NS_LITERAL_CSTRING(
      "SELECT b.fk, h.url, COALESCE(b.title, h.title), "
             "h.rev_host, h.visit_count, h.last_visit_date, f.url, b.id, "
             "b.dateAdded, b.lastModified, b.parent, "
             ) + tagsFragment + NS_LITERAL_CSTRING(", h.frecency, h.hidden, h.guid, "
             "b.guid, b.position, b.type, b.fk "
      "FROM moz_bookmarks b "
      "JOIN moz_places h ON b.fk = h.id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE b.id = :item_id ")
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"),
                                      aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore = false;
  rv = stmt->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasMore) {
    NS_NOTREACHED("Trying to get a result node for an invalid bookmark identifier");
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<mozIStorageValueArray> row = do_QueryInterface(stmt, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return RowToResult(row, aOptions, aResult);
}

nsresult
nsNavHistory::URIToResultNode(nsIURI* aURI,
                              nsNavHistoryQueryOptions* aOptions,
                              nsNavHistoryResultNode** aResult)
{
  nsAutoCString tagsFragment;
  GetTagsSqlFragment(GetTagsFolder(), NS_LITERAL_CSTRING("h.id"),
                     true, tagsFragment);
  
  nsCOMPtr<mozIStorageStatement> stmt = mDB->GetStatement(NS_LITERAL_CSTRING(
    "SELECT h.id, :page_url, COALESCE(b.title, h.title), "
           "h.rev_host, h.visit_count, h.last_visit_date, f.url, "
           "b.id, b.dateAdded, b.lastModified, b.parent, "
           ) + tagsFragment + NS_LITERAL_CSTRING(", h.frecency, h.hidden, h.guid, "
           "b.guid, b.position, b.type, b.fk "
    "FROM moz_places h "
    "LEFT JOIN moz_bookmarks b ON b.fk = h.id "
    "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE h.url = :page_url ")
  );
  NS_ENSURE_STATE(stmt);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore = false;
  rv = stmt->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasMore) {
    NS_NOTREACHED("Trying to get a result node for an invalid url");
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<mozIStorageValueArray> row = do_QueryInterface(stmt, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return RowToResult(row, aOptions, aResult);
}

void
nsNavHistory::SendPageChangedNotification(nsIURI* aURI,
                                          uint32_t aChangedAttribute,
                                          const nsAString& aNewValue,
                                          const nsACString& aGUID)
{
  MOZ_ASSERT(!aGUID.IsEmpty());
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver,
                   OnPageChanged(aURI, aChangedAttribute, aNewValue, aGUID));
}







void
nsNavHistory::TitleForDomain(const nsCString& domain, nsACString& aTitle)
{
  if (! domain.IsEmpty()) {
    aTitle = domain;
    return;
  }

  
  GetStringFromName(MOZ_UTF16("localhost"), aTitle);
}

void
nsNavHistory::GetAgeInDaysString(int32_t aInt, const char16_t *aName,
                                 nsACString& aResult)
{
  nsIStringBundle *bundle = GetBundle();
  if (bundle) {
    nsAutoString intString;
    intString.AppendInt(aInt);
    const char16_t* strings[1] = { intString.get() };
    nsXPIDLString value;
    nsresult rv = bundle->FormatStringFromName(aName, strings,
                                               1, getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
      CopyUTF16toUTF8(value, aResult);
      return;
    }
  }
  CopyUTF16toUTF8(nsDependentString(aName), aResult);
}

void
nsNavHistory::GetStringFromName(const char16_t *aName, nsACString& aResult)
{
  nsIStringBundle *bundle = GetBundle();
  if (bundle) {
    nsXPIDLString value;
    nsresult rv = bundle->GetStringFromName(aName, getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
      CopyUTF16toUTF8(value, aResult);
      return;
    }
  }
  CopyUTF16toUTF8(nsDependentString(aName), aResult);
}

void
nsNavHistory::GetMonthName(int32_t aIndex, nsACString& aResult)
{
  nsIStringBundle *bundle = GetDateFormatBundle();
  if (bundle) {
    nsCString name = nsPrintfCString("month.%d.name", aIndex);
    nsXPIDLString value;
    nsresult rv = bundle->GetStringFromName(NS_ConvertUTF8toUTF16(name).get(),
                                            getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
      CopyUTF16toUTF8(value, aResult);
      return;
    }
  }
  aResult = nsPrintfCString("[%d]", aIndex);
}

void
nsNavHistory::GetMonthYear(int32_t aMonth, int32_t aYear, nsACString& aResult)
{
  nsIStringBundle *bundle = GetBundle();
  if (bundle) {
    nsAutoCString monthName;
    GetMonthName(aMonth, monthName);
    nsAutoString yearString;
    yearString.AppendInt(aYear);
    const char16_t* strings[2] = {
      NS_ConvertUTF8toUTF16(monthName).get()
    , yearString.get()
    };
    nsXPIDLString value;
    if (NS_SUCCEEDED(bundle->FormatStringFromName(
          MOZ_UTF16("finduri-MonthYear"), strings, 2,
          getter_Copies(value)
        ))) {
      CopyUTF16toUTF8(value, aResult);
      return;
    }
  }
  aResult.AppendLiteral("finduri-MonthYear");
}


namespace {











static int64_t
GetSimpleBookmarksQueryFolder(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions* aOptions)
{
  if (aQueries.Count() != 1)
    return 0;

  nsNavHistoryQuery* query = aQueries[0];
  if (query->Folders().Length() != 1)
    return 0;

  bool hasIt;
  query->GetHasBeginTime(&hasIt);
  if (hasIt)
    return 0;
  query->GetHasEndTime(&hasIt);
  if (hasIt)
    return 0;
  query->GetHasDomain(&hasIt);
  if (hasIt)
    return 0;
  query->GetHasUri(&hasIt);
  if (hasIt)
    return 0;
  (void)query->GetHasSearchTerms(&hasIt);
  if (hasIt)
    return 0;
  if (query->Tags().Length() > 0)
    return 0;
  if (aOptions->MaxResults() > 0)
    return 0;

  
  
  if(aOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS)
    return 0;

  
  

  return query->Folders()[0];
}













inline bool isQueryWhitespace(char16_t ch)
{
  return ch == ' ';
}

void ParseSearchTermsFromQueries(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                 nsTArray<nsTArray<nsString>*>* aTerms)
{
  int32_t lastBegin = -1;
  for (int32_t i = 0; i < aQueries.Count(); i++) {
    nsTArray<nsString> *queryTerms = new nsTArray<nsString>();
    bool hasSearchTerms;
    if (NS_SUCCEEDED(aQueries[i]->GetHasSearchTerms(&hasSearchTerms)) &&
        hasSearchTerms) {
      const nsString& searchTerms = aQueries[i]->SearchTerms();
      for (uint32_t j = 0; j < searchTerms.Length(); j++) {
        if (isQueryWhitespace(searchTerms[j]) ||
            searchTerms[j] == '"') {
          if (lastBegin >= 0) {
            
            queryTerms->AppendElement(Substring(searchTerms, lastBegin,
                                               j - lastBegin));
            lastBegin = -1;
          }
        } else {
          if (lastBegin < 0) {
            
            lastBegin = j;
          }
        }
      }
      
      if (lastBegin >= 0)
        queryTerms->AppendElement(Substring(searchTerms, lastBegin));
    }
    aTerms->AppendElement(queryTerms);
  }
}

} 


nsresult
nsNavHistory::UpdateFrecency(int64_t aPlaceId)
{
  nsCOMPtr<mozIStorageAsyncStatement> updateFrecencyStmt = mDB->GetAsyncStatement(
    "UPDATE moz_places "
    "SET frecency = NOTIFY_FRECENCY("
      "CALCULATE_FRECENCY(:page_id), url, guid, hidden, last_visit_date"
    ") "
    "WHERE id = :page_id"
  );
  NS_ENSURE_STATE(updateFrecencyStmt);
  nsresult rv = updateFrecencyStmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"),
                                                    aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<mozIStorageAsyncStatement> updateHiddenStmt = mDB->GetAsyncStatement(
    "UPDATE moz_places "
    "SET hidden = 0 "
    "WHERE id = :page_id AND frecency <> 0"
  );
  NS_ENSURE_STATE(updateHiddenStmt);
  rv = updateHiddenStmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"),
                                         aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  mozIStorageBaseStatement *stmts[] = {
    updateFrecencyStmt.get()
  , updateHiddenStmt.get()
  };

  nsRefPtr<AsyncStatementCallbackNotifier> cb =
    new AsyncStatementCallbackNotifier(TOPIC_FRECENCY_UPDATED);
  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = mDB->MainConn()->ExecuteAsync(stmts, ArrayLength(stmts), cb,
                                     getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


namespace {

class FixInvalidFrecenciesCallback : public AsyncStatementCallbackNotifier
{
public:
  FixInvalidFrecenciesCallback()
    : AsyncStatementCallbackNotifier(TOPIC_FRECENCY_UPDATED)
  {
  }

  NS_IMETHOD HandleCompletion(uint16_t aReason)
  {
    nsresult rv = AsyncStatementCallbackNotifier::HandleCompletion(aReason);
    NS_ENSURE_SUCCESS(rv, rv);
    if (aReason == REASON_FINISHED) {
      nsNavHistory *navHistory = nsNavHistory::GetHistoryService();
      NS_ENSURE_STATE(navHistory);
      navHistory->NotifyManyFrecenciesChanged();
    }
    return NS_OK;
  }
};

} 

nsresult
nsNavHistory::FixInvalidFrecencies()
{
  nsCOMPtr<mozIStorageAsyncStatement> stmt = mDB->GetAsyncStatement(
    "UPDATE moz_places "
    "SET frecency = CALCULATE_FRECENCY(id) "
    "WHERE frecency < 0"
  );
  NS_ENSURE_STATE(stmt);

  nsRefPtr<FixInvalidFrecenciesCallback> callback =
    new FixInvalidFrecenciesCallback();
  nsCOMPtr<mozIStoragePendingStatement> ps;
  (void)stmt->ExecuteAsync(callback, getter_AddRefs(ps));

  return NS_OK;
}


#ifdef MOZ_XUL

nsresult
nsNavHistory::AutoCompleteFeedback(int32_t aIndex,
                                   nsIAutoCompleteController *aController)
{
  nsCOMPtr<mozIStorageAsyncStatement> stmt = mDB->GetAsyncStatement(
    "INSERT OR REPLACE INTO moz_inputhistory "
    
    "SELECT h.id, IFNULL(i.input, :input_text), IFNULL(i.use_count, 0) * .9 + 1 "
    "FROM moz_places h "
    "LEFT JOIN moz_inputhistory i ON i.place_id = h.id AND i.input = :input_text "
    "WHERE url = :page_url "
  );
  NS_ENSURE_STATE(stmt);

  nsAutoString input;
  nsresult rv = aController->GetSearchString(input);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("input_text"), input);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString url;
  rv = aController->GetValueAt(aIndex, url);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                       NS_ConvertUTF16toUTF8(url));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsRefPtr<AsyncStatementCallbackNotifier> callback =
    new AsyncStatementCallbackNotifier(TOPIC_AUTOCOMPLETE_FEEDBACK_UPDATED);
  nsCOMPtr<mozIStoragePendingStatement> canceler;
  rv = stmt->ExecuteAsync(callback, getter_AddRefs(canceler));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

#endif


nsICollation *
nsNavHistory::GetCollation()
{
  if (mCollation)
    return mCollation;

  
  nsCOMPtr<nsILocale> locale;
  nsCOMPtr<nsILocaleService> ls(do_GetService(NS_LOCALESERVICE_CONTRACTID));
  NS_ENSURE_TRUE(ls, nullptr);
  nsresult rv = ls->GetApplicationLocale(getter_AddRefs(locale));
  NS_ENSURE_SUCCESS(rv, nullptr);

  
  nsCOMPtr<nsICollationFactory> cfact =
    do_CreateInstance(NS_COLLATIONFACTORY_CONTRACTID);
  NS_ENSURE_TRUE(cfact, nullptr);
  rv = cfact->CreateCollation(locale, getter_AddRefs(mCollation));
  NS_ENSURE_SUCCESS(rv, nullptr);

  return mCollation;
}

nsIStringBundle *
nsNavHistory::GetBundle()
{
  if (!mBundle) {
    nsCOMPtr<nsIStringBundleService> bundleService =
      services::GetStringBundleService();
    NS_ENSURE_TRUE(bundleService, nullptr);
    nsresult rv = bundleService->CreateBundle(
        "chrome://places/locale/places.properties",
        getter_AddRefs(mBundle));
    NS_ENSURE_SUCCESS(rv, nullptr);
  }
  return mBundle;
}

nsIStringBundle *
nsNavHistory::GetDateFormatBundle()
{
  if (!mDateFormatBundle) {
    nsCOMPtr<nsIStringBundleService> bundleService =
      services::GetStringBundleService();
    NS_ENSURE_TRUE(bundleService, nullptr);
    nsresult rv = bundleService->CreateBundle(
        "chrome://global/locale/dateFormat.properties",
        getter_AddRefs(mDateFormatBundle));
    NS_ENSURE_SUCCESS(rv, nullptr);
  }
  return mDateFormatBundle;
}
