













































#include "nsNavHistory.h"
#include "mozStorageHelper.h"
#include "nsNetUtil.h"
#include "nsIAnnotationService.h"
#include "nsPrintfCString.h"
#include "nsPlacesMacros.h"
#include "nsIIdleService.h"

struct nsNavHistoryExpireRecord {
  nsNavHistoryExpireRecord(mozIStorageStatement* statement);

  PRInt64 visitID;
  PRInt64 placeID;
  PRTime visitDate;
  nsCString uri;
  PRInt64 faviconID;
  PRBool hidden;
  PRBool bookmarked;
  PRBool erased; 
};



#define EXPIRATION_PARTIAL_TIMEOUT 500


#define EXPIRATION_PARTIAL_SUBSEQUENT_TIMEOUT ((PRUint32)10 * PR_MSEC_PER_SEC)



#define EXPIRATION_PAGES_PER_RUN 6




#define EXPIRATION_IDLE_TIMEOUT ((PRUint32)5 * 60 * PR_MSEC_PER_SEC)


#define EXPIRATION_IDLE_LONG_TIMEOUT EXPIRATION_IDLE_TIMEOUT * 10


#define EXPIRATION_MAX_PAGES_AT_IDLE 100




#define EXPIRATION_MAX_PAGES_AT_SHUTDOWN 100


const PRTime EXPIRATION_POLICY_DAYS = ((PRTime)7 * 86400 * PR_USEC_PER_SEC);
const PRTime EXPIRATION_POLICY_WEEKS = ((PRTime)30 * 86400 * PR_USEC_PER_SEC);
const PRTime EXPIRATION_POLICY_MONTHS = ((PRTime)180 * 86400 * PR_USEC_PER_SEC);


#define PREF_BRANCH_BASE                        "browser."
#define PREF_BROWSER_HISTORY_EXPIRE_DAYS        "history_expire_days"


#define PREF_SANITIZE_ON_SHUTDOWN   "privacy.sanitize.sanitizeOnShutdown"
#define PREF_SANITIZE_ITEM_HISTORY  "privacy.item.history"

nsNavHistoryExpire::nsNavHistoryExpire() :
    mNextExpirationTime(0)
{
  mHistory = nsNavHistory::GetHistoryService();
  NS_ASSERTION(mHistory, "History service should exist at this point.");
  mDBConn = mHistory->GetStorageConnection();
  NS_ASSERTION(mDBConn, "History service should have a valid database connection");

  
  InitializeIdleTimer(EXPIRATION_IDLE_TIMEOUT);
}

nsNavHistoryExpire::~nsNavHistoryExpire()
{
  
  if (mPartialExpirationTimer) {
    mPartialExpirationTimer->Cancel();
    mPartialExpirationTimer = 0;
  }
  if (mIdleTimer) {
    mIdleTimer->Cancel();
    mIdleTimer = 0;
  }
}

void
nsNavHistoryExpire::InitializeIdleTimer(PRUint32 aTimeInMs)
{
  if (mIdleTimer) {
    mIdleTimer->Cancel();
    mIdleTimer = 0;
  }

  mIdleTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mIdleTimer) {
    (void)mIdleTimer->InitWithFuncCallback(IdleTimerCallback, this, aTimeInMs,
                                           nsITimer::TYPE_ONE_SHOT);
  }
}

void 
nsNavHistoryExpire::IdleTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsNavHistoryExpire* expire = static_cast<nsNavHistoryExpire*>(aClosure);
  expire->mIdleTimer = 0;
  expire->OnIdle();
}

void
nsNavHistoryExpire::OnIdle()
{
  PRUint32 idleTime = 0;
  nsCOMPtr<nsIIdleService> idleService =
    do_GetService("@mozilla.org/widget/idleservice;1");
  if (idleService)
    (void)idleService->GetIdleTime(&idleTime);

  
  
  if (idleTime < EXPIRATION_IDLE_TIMEOUT)
    return;

  mozStorageTransaction transaction(mDBConn, PR_TRUE);

  bool keepGoing = ExpireItems(EXPIRATION_MAX_PAGES_AT_IDLE);
  ExpireOrphans(EXPIRATION_MAX_PAGES_AT_IDLE);

  if (!keepGoing) {
    
    
    InitializeIdleTimer(EXPIRATION_IDLE_LONG_TIMEOUT);
  }
  else
    InitializeIdleTimer(EXPIRATION_IDLE_TIMEOUT);
}

void
nsNavHistoryExpire::OnDeleteVisits()
{
  (void)ExpireAnnotations();
}

void
nsNavHistoryExpire::OnQuit()
{
  
  if (mPartialExpirationTimer) {
    mPartialExpirationTimer->Cancel();
    mPartialExpirationTimer = 0;
  }
  if (mIdleTimer) {
    mIdleTimer->Cancel();
    mIdleTimer = 0;
  }

  nsCOMPtr<nsIPrefBranch> prefs =
    do_GetService("@mozilla.org/preferences-service;1");
  if (prefs) {
    
    
    PRBool sanitizeOnShutdown = PR_FALSE;
    (void)prefs->GetBoolPref(PREF_SANITIZE_ON_SHUTDOWN, &sanitizeOnShutdown);
    PRBool sanitizeHistory = PR_FALSE;
    (void)prefs->GetBoolPref(PREF_SANITIZE_ITEM_HISTORY, &sanitizeHistory);

    if (sanitizeHistory && sanitizeOnShutdown)
      return;
  }

  
  ExpireOrphans(EXPIRATION_MAX_PAGES_AT_SHUTDOWN);
}

nsresult
nsNavHistoryExpire::ClearHistory()
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  
  
  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_places_view SET frecency = -MAX(visit_count, 1) "
    "WHERE id IN("
      "SELECT h.id FROM moz_places_temp h "
      "WHERE "
        "EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) "
      "UNION ALL "
      "SELECT h.id FROM moz_places h "
      "WHERE "
        "EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) "
    ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits_view"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  ExpireOrphans(-1);

  
  
  
  
  rv = mHistory->FixInvalidFrecenciesForExcludedPlaces();
  if (NS_FAILED(rv))
    NS_WARNING("failed to fix invalid frecencies");

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  ENUMERATE_OBSERVERS(mHistory->canNotify(), mHistory->mCacheObservers,
                      mHistory->mObservers, nsINavHistoryObserver,
                      OnClearHistory())

  return NS_OK;
}

void
nsNavHistoryExpire::OnExpirationChanged()
{
  
  
  StartPartialExpirationTimer(EXPIRATION_PARTIAL_TIMEOUT);
}

nsresult
nsNavHistoryExpire::DoPartialExpiration()
{
  bool keepGoing = ExpireItems(EXPIRATION_PAGES_PER_RUN);
  if (keepGoing)
    StartPartialExpirationTimer(EXPIRATION_PARTIAL_SUBSEQUENT_TIMEOUT);

  return NS_OK;
}

bool
nsNavHistoryExpire::ExpireItems(PRUint32 aNumToExpire)
{
  
  bool keepGoing = true;

  
  
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  PRInt64 expireTime;
  if (!aNumToExpire) {
    
    expireTime = 0;
  }
  else {
    expireTime = PR_Now() - GetExpirationTimeAgo(mHistory->mExpireDaysMax);
  }

  
  nsTArray<nsNavHistoryExpireRecord> expiredVisits;
  nsresult rv = FindVisits(expireTime, aNumToExpire, expiredVisits);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "FindVisits Failed");

  
  
  if (expiredVisits.Length() < aNumToExpire) {
    keepGoing = false;
    ComputeNextExpirationTime();
  }

  rv = EraseVisits(expiredVisits);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "EraseVisits Failed");

  rv = EraseHistory(expiredVisits);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "EraseHistory Failed");

  
  nsCOMPtr<nsIURI> uri;
  for (PRUint32 i = 0; i < expiredVisits.Length(); i ++) {
    rv = NS_NewURI(getter_AddRefs(uri), expiredVisits[i].uri);
    if (NS_FAILED(rv)) {
      NS_WARNING("Trying to expire a corrupt uri?!");
      continue;
    }

    
    if (expiredVisits[i].hidden) continue;

    ENUMERATE_OBSERVERS(mHistory->canNotify(), mHistory->mCacheObservers,
                        mHistory->mObservers, nsINavHistoryObserver,
                        OnPageExpired(uri, expiredVisits[i].visitDate,
                                      expiredVisits[i].erased));
  }

  
  rv = EraseFavicons(expiredVisits);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "EraseFavicons Failed");
  rv = EraseAnnotations(expiredVisits);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "EraseAnnotations Failed");
  rv = ExpireAnnotations();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "ExpireAnnotarions Failed");

  rv = transaction.Commit();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Committing transaction Failed");

  return keepGoing;
}

void
nsNavHistoryExpire::ExpireOrphans(PRUint32 aNumToExpire)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  nsresult rv = ExpireHistoryParanoid(aNumToExpire);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "ExpireHistoryParanoid Failed");

  rv = ExpireFaviconsParanoid();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "ExpireFaviconsParanoid Failed");

  rv = ExpireAnnotationsParanoid();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "ExpireAnnotationsParanoid Failed");

  rv = ExpireInputHistoryParanoid();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "ExpireInputHistoryParanoid Failed");

  rv = transaction.Commit();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Commit Transaction Failed");
}







nsNavHistoryExpireRecord::nsNavHistoryExpireRecord(
  mozIStorageStatement* statement)
{
  visitID = statement->AsInt64(0);
  placeID = statement->AsInt64(1);
  visitDate = statement->AsInt64(2);
  statement->GetUTF8String(3, uri);
  faviconID = statement->AsInt64(4);
  hidden = (statement->AsInt32(5) > 0);
  bookmarked = (statement->AsInt32(6) > 0);
  erased = PR_FALSE;
}

nsresult
nsNavHistoryExpire::FindVisits(PRTime aExpireThreshold, PRUint32 aNumToExpire,
                               nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCOMPtr<mozIStorageStatement> selectStatement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT v.id, v.place_id, v.visit_date, IFNULL(h_t.url, h.url), "
             "IFNULL(h_t.favicon_id, h.favicon_id), "
             "IFNULL(h_t.hidden, h.hidden), b.fk "
      "FROM moz_historyvisits_temp v "
      "LEFT JOIN moz_places_temp AS h_t ON h_t.id = v.place_id "
      "LEFT JOIN moz_places AS h ON h.id = v.place_id "
      "LEFT JOIN moz_bookmarks b ON b.fk = v.place_id "
      "WHERE visit_date < ?1 "
      "UNION ALL "
      "SELECT v.id, v.place_id, v.visit_date, IFNULL(h_t.url, h.url), "
             "IFNULL(h_t.favicon_id, h.favicon_id), "
             "IFNULL(h_t.hidden, h.hidden), b.fk "
      "FROM moz_historyvisits v "
      "LEFT JOIN moz_places_temp AS h_t ON h_t.id = v.place_id "
      "LEFT JOIN moz_places AS h ON h.id = v.place_id "
      "LEFT JOIN moz_bookmarks b ON b.fk = v.place_id "
      "WHERE visit_date < ?1 "
      "ORDER BY v.visit_date ASC "
      "LIMIT ?2 "),
    getter_AddRefs(selectStatement));
    NS_ENSURE_SUCCESS(rv, rv);

  
  PRTime expireMaxTime = aExpireThreshold ? aExpireThreshold : LL_MAXINT;
  rv = selectStatement->BindInt64Parameter(0, expireMaxTime);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt32 numToExpire = aNumToExpire ? aNumToExpire : -1;
  rv = selectStatement->BindInt64Parameter(1, numToExpire);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(selectStatement->ExecuteStep(&hasMore)) && hasMore) {
    nsNavHistoryExpireRecord record(selectStatement);
    aRecords.AppendElement(record);
  }

  
  
  if (aRecords.Length() < aNumToExpire) {
    
    nsCOMPtr<mozIStorageStatement> countStatement;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT "
          "(SELECT count(*) FROM moz_places_temp WHERE visit_count > 0) + "
          "(SELECT count(*) FROM moz_places WHERE visit_count > 0 AND "
            "id NOT IN (SELECT id FROM moz_places_temp))"),
      getter_AddRefs(countStatement));
    NS_ENSURE_SUCCESS(rv, rv);

    hasMore = PR_FALSE;
    
    PRInt32 pageCount = mHistory->mExpireSites;
    if (NS_SUCCEEDED(countStatement->ExecuteStep(&hasMore)) && hasMore) {
      rv = countStatement->GetInt32(0, &pageCount);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    if (pageCount <= mHistory->mExpireSites)
        return NS_OK;

    rv = selectStatement->Reset();
    NS_ENSURE_SUCCESS(rv, rv);

    
    PRTime expireMinTime = PR_Now() -
                           GetExpirationTimeAgo(mHistory->mExpireDaysMin);
    rv = selectStatement->BindInt64Parameter(0, expireMinTime);
    NS_ENSURE_SUCCESS(rv, rv);
    
    numToExpire = aNumToExpire - aRecords.Length();
    rv = selectStatement->BindInt64Parameter(1, numToExpire);
    NS_ENSURE_SUCCESS(rv, rv);

    hasMore = PR_FALSE;
    while (NS_SUCCEEDED(selectStatement->ExecuteStep(&hasMore)) && hasMore) {
      nsNavHistoryExpireRecord record(selectStatement);
      aRecords.AppendElement(record);
    }
  }

  return NS_OK;
}

nsresult
nsNavHistoryExpire::EraseVisits(
    const nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  
  nsCString deletedVisitIds;
  nsCString placeIds;
  nsTArray<PRInt64> deletedPlaceIdsArray, deletedVisitIdsArray;
  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    
    if (deletedVisitIdsArray.IndexOf(aRecords[i].visitID) == -1) {
      if (!deletedVisitIds.IsEmpty())
        deletedVisitIds.AppendLiteral(",");  
      deletedVisitIds.AppendInt(aRecords[i].visitID);
    }

    
    if (deletedPlaceIdsArray.IndexOf(aRecords[i].placeID) == -1) {
      if (!placeIds.IsEmpty())
        placeIds.AppendLiteral(",");
      placeIds.AppendInt(aRecords[i].placeID);
    }
  }

  if (deletedVisitIds.IsEmpty())
    return NS_OK;

  
  
  
  
  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_places_view "
    "SET frecency = -MAX(visit_count, 1) "
    "WHERE id IN ( "
      "SELECT h.id FROM moz_places_temp h "
      "WHERE "
        "NOT EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) AND "
        "NOT EXISTS ( "
          "SELECT v.id FROM moz_historyvisits_temp v "
          "WHERE v.place_id = h.id "
          "AND v.id NOT IN (") + deletedVisitIds + NS_LITERAL_CSTRING(") "
        ") AND "
        "NOT EXISTS ( "
          "SELECT v.id FROM moz_historyvisits v "
          "WHERE v.place_id = h.id "
          "AND v.id NOT IN (") + deletedVisitIds + NS_LITERAL_CSTRING(") "
        ") AND "
        "h.id IN (") + placeIds + NS_LITERAL_CSTRING(") "
      "UNION ALL "
      "SELECT h.id FROM moz_places h "
      "WHERE "
        "NOT EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) AND "
        "NOT EXISTS ( "
          "SELECT v.id FROM moz_historyvisits_temp v "
          "WHERE v.place_id = h.id "
          "AND v.id NOT IN (") + deletedVisitIds + NS_LITERAL_CSTRING(") "
        ") AND "
        "NOT EXISTS ( "
          "SELECT v.id FROM moz_historyvisits v "
          "WHERE v.place_id = h.id "
          "AND v.id NOT IN (") + deletedVisitIds + NS_LITERAL_CSTRING(") "
        ") AND "
        "h.id IN (") + placeIds + NS_LITERAL_CSTRING(") "
    ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DELETE FROM moz_historyvisits_view WHERE id IN (") +
    deletedVisitIds +
    NS_LITERAL_CSTRING(")"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistoryExpire::EraseHistory(
    nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCString deletedPlaceIds;
  nsTArray<PRInt64> deletedPlaceIdsArray;
  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    
    if (aRecords[i].bookmarked ||
        StringBeginsWith(aRecords[i].uri, NS_LITERAL_CSTRING("place:")))
      continue;
    
    if (deletedPlaceIdsArray.IndexOf(aRecords[i].placeID) == -1) {
      
      if (!deletedPlaceIds.IsEmpty())
        deletedPlaceIds.AppendLiteral(",");
      deletedPlaceIdsArray.AppendElement(aRecords[i].placeID);
      deletedPlaceIds.AppendInt(aRecords[i].placeID);
    }
    aRecords[i].erased = PR_TRUE;
  }

  if (deletedPlaceIds.IsEmpty())
    return NS_OK;

  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_places_view WHERE id IN( "
        "SELECT h.id "
        "FROM moz_places h "
        "WHERE h.id IN(") + deletedPlaceIds + NS_LITERAL_CSTRING(") "
          "AND NOT EXISTS "
            "(SELECT id FROM moz_historyvisits WHERE place_id = h.id LIMIT 1) "
          "AND NOT EXISTS "
            "(SELECT id FROM moz_historyvisits_temp WHERE place_id = h.id LIMIT 1) "
          "AND NOT EXISTS "
            "(SELECT id FROM moz_bookmarks WHERE fk = h.id LIMIT 1) "
          "AND SUBSTR(h.url, 1, 6) <> 'place:' "
        "UNION ALL "
        "SELECT h.id "
        "FROM moz_places_temp h "
        "WHERE h.id IN(") + deletedPlaceIds + NS_LITERAL_CSTRING(") "
          "AND NOT EXISTS "
            "(SELECT id FROM moz_historyvisits WHERE place_id = h.id LIMIT 1) "
          "AND NOT EXISTS "
            "(SELECT id FROM moz_historyvisits_temp WHERE place_id = h.id LIMIT 1) "
          "AND NOT EXISTS "
            "(SELECT id FROM moz_bookmarks WHERE fk = h.id LIMIT 1) "
          "AND SUBSTR(h.url, 1, 6) <> 'place:' "
      ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistoryExpire::EraseFavicons(
    const nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCString deletedFaviconIds;
  nsTArray<PRInt64> deletedFaviconIdsArray;  
  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    
    if (!aRecords[i].erased || aRecords[i].faviconID == 0)
      continue;
    
    if (deletedFaviconIdsArray.IndexOf(aRecords[i].faviconID) == -1) {
      
      if (!deletedFaviconIds.IsEmpty())
        deletedFaviconIds.AppendLiteral(",");
      deletedFaviconIdsArray.AppendElement(aRecords[i].faviconID);
      deletedFaviconIds.AppendInt(aRecords[i].faviconID);
    }
  }

  if (deletedFaviconIds.IsEmpty())
    return NS_OK;

  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_favicons WHERE id IN ( "
        "SELECT f.id FROM moz_favicons f "
        "LEFT JOIN moz_places h ON f.id = h.favicon_id "
        "LEFT JOIN moz_places_temp h_t ON f.id = h_t.favicon_id "
        "WHERE f.id IN (") + deletedFaviconIds + NS_LITERAL_CSTRING(") "
        "AND h.favicon_id IS NULL "
        "AND h_t.favicon_id IS NULL "
      ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistoryExpire::EraseAnnotations(
    const nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCString placeIds;
  nsTArray<PRInt64> deletedPlaceIdsArray;
  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    
    if (deletedPlaceIdsArray.IndexOf(aRecords[i].placeID) == -1) {
      
      if (!placeIds.IsEmpty())
        placeIds.AppendLiteral(",");
      deletedPlaceIdsArray.AppendElement(aRecords[i].placeID);
      placeIds.AppendInt(aRecords[i].placeID);
    }
  }
  
  if (placeIds.IsEmpty())
    return NS_OK;
    
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_annos WHERE place_id in (") +
      placeIds + NS_LITERAL_CSTRING(") AND expiration != ") +
      nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsNavHistoryExpire::ExpireAnnotations()
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  
  PRTime now = PR_Now();
  nsCOMPtr<mozIStorageStatement> expirePagesStatement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_annos "
      "WHERE expiration = ?1 AND "
        "(?2 > MAX(COALESCE(lastModified, 0), dateAdded))"),
    getter_AddRefs(expirePagesStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<mozIStorageStatement> expireItemsStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_items_annos "
      "WHERE expiration = ?1 AND "
        "(?2 > MAX(COALESCE(lastModified, 0), dateAdded))"),
    getter_AddRefs(expireItemsStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = expirePagesStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_DAYS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_DAYS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = expireItemsStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_DAYS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_DAYS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = expirePagesStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_WEEKS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_WEEKS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = expireItemsStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_WEEKS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_WEEKS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = expirePagesStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_MONTHS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_MONTHS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = expireItemsStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_MONTHS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_MONTHS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_annos WHERE expiration = ") +
        nsPrintfCString("%d", nsIAnnotationService::EXPIRE_WITH_HISTORY) +
        NS_LITERAL_CSTRING(" AND NOT EXISTS "
          "(SELECT id FROM moz_historyvisits_temp "
          "WHERE place_id = moz_annos.place_id LIMIT 1) "
        "AND NOT EXISTS "
          "(SELECT id FROM moz_historyvisits "
          "WHERE place_id = moz_annos.place_id LIMIT 1)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistoryExpire::ExpireHistoryParanoid(PRInt32 aMaxRecords)
{
  nsCAutoString query(
    "DELETE FROM moz_places_view WHERE id IN ("
      "SELECT h.id FROM moz_places h "
      "LEFT JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id "
      "LEFT JOIN moz_bookmarks b ON h.id = b.fk "
      "WHERE v.id IS NULL "
        "AND v_t.id IS NULL "
        "AND b.id IS NULL "
        "AND SUBSTR(h.url, 1, 6) <> 'place:' "
      "UNION ALL "
      "SELECT h.id FROM moz_places_temp h "
      "LEFT JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id "
      "LEFT JOIN moz_bookmarks b ON h.id = b.fk "
      "WHERE v.id IS NULL "
        "AND v_t.id IS NULL "
        "AND b.id IS NULL "
        "AND SUBSTR(h.url, 1, 6) <> 'place:'");
  if (aMaxRecords != -1) {
    query.AppendLiteral(" LIMIT ");
    query.AppendInt(aMaxRecords);
  }
  query.AppendLiteral(")");
  nsresult rv = mDBConn->ExecuteSimpleSQL(query);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistoryExpire::ExpireFaviconsParanoid()
{
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_favicons WHERE id IN ("
        "SELECT f.id FROM moz_favicons f "
        "LEFT JOIN moz_places h ON f.id = h.favicon_id "
        "LEFT JOIN moz_places_temp h_t ON f.id = h_t.favicon_id "
        "WHERE h.favicon_id IS NULL "
          "AND h_t.favicon_id IS NULL "
      ")"));
  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

nsresult
nsNavHistoryExpire::ExpireAnnotationsParanoid()
{
  
  nsCAutoString session_query = NS_LITERAL_CSTRING(
    "DELETE FROM moz_annos WHERE expiration = ") +
    nsPrintfCString("%d", nsIAnnotationService::EXPIRE_SESSION);
  nsresult rv = mDBConn->ExecuteSimpleSQL(session_query);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_annos WHERE id IN ("
        "SELECT a.id FROM moz_annos a "
        "LEFT JOIN moz_places h ON a.place_id = h.id "
        "LEFT JOIN moz_places_temp h_t ON a.place_id = h_t.id "
        "LEFT JOIN moz_historyvisits v ON a.place_id = v.place_id "
        "LEFT JOIN moz_historyvisits_temp v_t ON a.place_id = v_t.place_id "
        "WHERE (h.id IS NULL AND h_t.id IS NULL) "
          "OR (v.id IS NULL AND v_t.id IS NULL AND a.expiration != ") +
            nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER) +
          NS_LITERAL_CSTRING(")"
      ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_items_annos WHERE id IN "
      "(SELECT a.id FROM moz_items_annos a "
      "LEFT OUTER JOIN moz_bookmarks b ON a.item_id = b.id "
      "WHERE b.id IS NULL)"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_anno_attributes WHERE id IN (" 
        "SELECT n.id FROM moz_anno_attributes n "
        "LEFT JOIN moz_annos a ON n.id = a.anno_attribute_id "
        "LEFT JOIN moz_items_annos t ON n.id = t.anno_attribute_id "
        "WHERE a.anno_attribute_id IS NULL "
          "AND t.anno_attribute_id IS NULL "
      ")"));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsNavHistoryExpire::ExpireInputHistoryParanoid()
{
  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_inputhistory WHERE place_id IN ( "
        "SELECT place_id FROM moz_inputhistory "
        "LEFT JOIN moz_places h ON h.id = place_id "
        "LEFT JOIN moz_places_temp h_t ON h_t.id = place_id "
        "WHERE h.id IS NULL "
          "AND h_t.id IS NULL "
      ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsNavHistoryExpire::ComputeNextExpirationTime()
{
  mNextExpirationTime = 0;

  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT MIN(visit_date) FROM moz_historyvisits"),
    getter_AddRefs(statement));
  NS_ASSERTION(NS_SUCCEEDED(rv), "Could not create statement");
  if (NS_FAILED(rv)) return;

  PRBool hasMore;
  rv = statement->ExecuteStep(&hasMore);
  if (NS_FAILED(rv) || !hasMore)
    return; 
            

  PRTime minTime = statement->AsInt64(0);
  mNextExpirationTime = minTime + GetExpirationTimeAgo(mHistory->mExpireDaysMax);
}

void
nsNavHistoryExpire::StartPartialExpirationTimer(PRUint32 aMilleseconds)
{
  if (mPartialExpirationTimer) {
    mPartialExpirationTimer->Cancel();
    mPartialExpirationTimer = 0;
  }

  mPartialExpirationTimer = do_CreateInstance("@mozilla.org/timer;1");
  if(mPartialExpirationTimer) {
    (void)mPartialExpirationTimer->InitWithFuncCallback(
      PartialExpirationTimerCallback, this, aMilleseconds,
      nsITimer::TYPE_ONE_SHOT);
  }
}

void 
nsNavHistoryExpire::PartialExpirationTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsNavHistoryExpire* expire = static_cast<nsNavHistoryExpire*>(aClosure);
  expire->mPartialExpirationTimer = 0;
  expire->DoPartialExpiration();
}

PRTime
nsNavHistoryExpire::GetExpirationTimeAgo(PRInt32 aExpireDays)
{
  
  
  const PRInt32 maxDays = 106751991;
  if (aExpireDays > maxDays)
    aExpireDays = maxDays;

  
  
  return (PRTime)aExpireDays * 86400 * PR_USEC_PER_SEC;
}
