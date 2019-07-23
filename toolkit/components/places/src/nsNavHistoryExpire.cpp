











































#include "nsNavHistory.h"
#include "mozStorageHelper.h"
#include "nsNetUtil.h"
#include "nsIAnnotationService.h"
#include "nsPrintfCString.h"

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






#define EXPIRATION_COUNT_PER_RUN 6


#define EXPIRATION_COUNT_PER_RUN_LARGE 50





#define PARTIAL_EXPIRATION_TIMEOUT (3.5 * PR_MSEC_PER_SEC)


#define SUBSEQUENT_EXPIRATION_TIMEOUT (20 * PR_MSEC_PER_SEC)







#define MAX_SEQUENTIAL_RUNS 1


#define PREF_SANITIZE_ON_SHUTDOWN   "privacy.sanitize.sanitizeOnShutdown"
#define PREF_SANITIZE_ITEM_HISTORY  "privacy.item.history"


const PRTime EXPIRATION_POLICY_DAYS = ((PRTime)7 * 86400 * PR_USEC_PER_SEC);
const PRTime EXPIRATION_POLICY_WEEKS = ((PRTime)30 * 86400 * PR_USEC_PER_SEC);
const PRTime EXPIRATION_POLICY_MONTHS = ((PRTime)180 * 86400 * PR_USEC_PER_SEC);


#define EXPIRATION_CAP_PLACES 500


#define PREF_BRANCH_BASE                        "browser."
#define PREF_BROWSER_HISTORY_EXPIRE_DAYS        "history_expire_days"







nsNavHistoryExpire::nsNavHistoryExpire(nsNavHistory* aHistory) :
    mHistory(aHistory),
    mTimerSet(PR_FALSE),
    mAnyEmptyRuns(PR_FALSE),
    mNextExpirationTime(0),
    mAddCount(0),
    mExpiredItems(0)
{

}




nsNavHistoryExpire::~nsNavHistoryExpire()
{

}











void
nsNavHistoryExpire::OnAddURI(PRTime aNow)
{
  mAddCount ++;

  if (mTimer && mTimerSet) {
    mTimer->Cancel();
    mTimerSet = PR_FALSE;
  }

  if (mNextExpirationTime != 0 && aNow < mNextExpirationTime)
    return; 

  StartTimer(PARTIAL_EXPIRATION_TIMEOUT);
}






void
nsNavHistoryExpire::OnDeleteURI()
{
  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  if (!connection) {
    NS_NOTREACHED("No connection");
    return;
  }
  nsresult rv = ExpireAnnotations(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireAnnotations failed.");
}





void
nsNavHistoryExpire::OnQuit()
{
  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  if (!connection) {
    NS_NOTREACHED("No connection");
    return;
  }

  
  if (mTimer)
    mTimer->Cancel();

  
  nsresult rv = ExpireForDegenerateRuns();
  if (NS_FAILED(rv))
    NS_WARNING("ExpireForDegenerateRuns failed.");

  
  
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService("@mozilla.org/preferences-service;1"));
  PRBool sanitizeOnShutdown = PR_FALSE;
  PRBool sanitizeHistory = PR_FALSE;
  (void)prefs->GetBoolPref(PREF_SANITIZE_ON_SHUTDOWN, &sanitizeOnShutdown);
  (void)prefs->GetBoolPref(PREF_SANITIZE_ITEM_HISTORY, &sanitizeHistory);
  if (sanitizeHistory && sanitizeOnShutdown)
    return;

  
  rv = ExpireOrphans(EXPIRATION_CAP_PLACES);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireOrphans failed.");
}








nsresult
nsNavHistoryExpire::ClearHistory()
{
  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  NS_ENSURE_TRUE(connection, NS_ERROR_OUT_OF_MEMORY);

  mozStorageTransaction transaction(connection, PR_FALSE);

  
  
  
  
  nsresult rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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

  
  rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits_view"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ExpireHistoryParanoid(connection, -1);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireHistoryParanoid failed.");

  rv = ExpireFaviconsParanoid(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireFaviconsParanoid failed.");

  rv = ExpireAnnotationsParanoid(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireAnnotationsParanoid failed.");

  rv = ExpireInputHistoryParanoid(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireInputHistoryParanoid failed.");

  
  
  
  
  rv = mHistory->FixInvalidFrecenciesForExcludedPlaces();
  if (NS_FAILED(rv))
    NS_WARNING("failed to fix invalid frecencies");

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  

  ENUMERATE_WEAKARRAY(mHistory->mObservers, nsINavHistoryObserver,
                      OnClearHistory())

  return NS_OK;
}








void
nsNavHistoryExpire::OnExpirationChanged()
{
  mNextExpirationTime = 0;
  
  (void)OnAddURI(PR_Now());
}




nsresult
nsNavHistoryExpire::DoPartialExpiration()
{
  
  PRBool keepGoing;
  nsresult rv = ExpireItems(EXPIRATION_COUNT_PER_RUN, &keepGoing);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireItems failed.");
  else if (keepGoing)
    StartTimer(SUBSEQUENT_EXPIRATION_TIMEOUT);
  return NS_OK;
}












nsresult
nsNavHistoryExpire::ExpireItems(PRUint32 aNumToExpire, PRBool* aKeepGoing)
{
  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  NS_ENSURE_TRUE(connection, NS_ERROR_OUT_OF_MEMORY);

  
  
  
  mozStorageTransaction transaction(connection, PR_FALSE);

  *aKeepGoing = PR_TRUE;

  PRInt64 expireTime;
  if (aNumToExpire == 0) {
    
    expireTime = 0;
  } else {
    expireTime = PR_Now() - GetExpirationTimeAgo(mHistory->mExpireDaysMax);
  }

  
  nsTArray<nsNavHistoryExpireRecord> expiredVisits;
  nsresult rv = FindVisits(expireTime, aNumToExpire, connection,
                           expiredVisits);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (expiredVisits.Length() < aNumToExpire) {
    *aKeepGoing = PR_FALSE;
    ComputeNextExpirationTime(connection);

    if (expiredVisits.Length() == 0) {
      
      
      mAnyEmptyRuns = PR_TRUE;
      return NS_OK;
    }
  }
  mExpiredItems += expiredVisits.Length();

  rv = EraseVisits(connection, expiredVisits);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EraseHistory(connection, expiredVisits);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIURI> uri;
  for (PRUint32 i = 0; i < expiredVisits.Length(); i ++) {
    rv = NS_NewURI(getter_AddRefs(uri), expiredVisits[i].uri);
    if (NS_FAILED(rv)) continue;

    
    if (expiredVisits[i].hidden) continue;

    ENUMERATE_WEAKARRAY(mHistory->mObservers, nsINavHistoryObserver,
                        OnPageExpired(uri, expiredVisits[i].visitDate,
                                      expiredVisits[i].erased));
  }

  
  rv = EraseFavicons(connection, expiredVisits);
  if (NS_FAILED(rv))
    NS_WARNING("EraseFavicons failed.");

  rv = EraseAnnotations(connection, expiredVisits);
  if (NS_FAILED(rv))
    NS_WARNING("EraseAnnotations failed.");

  
  rv = ExpireAnnotations(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireAnnotations failed.");

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}







nsresult
nsNavHistoryExpire::ExpireOrphans(PRUint32 aNumToExpire)
{
  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  NS_ENSURE_TRUE(connection, NS_ERROR_OUT_OF_MEMORY);

  mozStorageTransaction transaction(connection, PR_FALSE);

  nsresult rv = ExpireHistoryParanoid(connection, aNumToExpire);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ExpireFaviconsParanoid(connection);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ExpireAnnotationsParanoid(connection);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ExpireInputHistoryParanoid(connection);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
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
                               mozIStorageConnection* aConnection,
                               nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCOMPtr<mozIStorageStatement> selectStatement;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
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
    rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
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
nsNavHistoryExpire::EraseVisits(mozIStorageConnection* aConnection,
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

  
  
  
  
  
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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

  rv = aConnection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DELETE FROM moz_historyvisits_view WHERE id IN (") +
    deletedVisitIds +
    NS_LITERAL_CSTRING(")"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}











nsresult
nsNavHistoryExpire::EraseHistory(mozIStorageConnection* aConnection,
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

  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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
nsNavHistoryExpire::EraseFavicons(mozIStorageConnection* aConnection,
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

  
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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
nsNavHistoryExpire::EraseAnnotations(mozIStorageConnection* aConnection,
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
    
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_annos WHERE place_id in (") +
      placeIds + NS_LITERAL_CSTRING(") AND expiration != ") +
      nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}









nsresult
nsNavHistoryExpire::ExpireAnnotations(mozIStorageConnection* aConnection)
{
  mozStorageTransaction transaction(aConnection, PR_FALSE);

  
  
  PRTime now = PR_Now();
  nsCOMPtr<mozIStorageStatement> expirePagesStatement;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_annos "
      "WHERE expiration = ?1 AND "
        "(?2 > MAX(COALESCE(lastModified, 0), dateAdded))"),
    getter_AddRefs(expirePagesStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<mozIStorageStatement> expireItemsStatement;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
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

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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
nsNavHistoryExpire::ExpireHistoryParanoid(mozIStorageConnection* aConnection,
                                          PRInt32 aMaxRecords)
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
  nsresult rv = aConnection->ExecuteSimpleSQL(query);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}






nsresult
nsNavHistoryExpire::ExpireFaviconsParanoid(mozIStorageConnection* aConnection)
{
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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
nsNavHistoryExpire::ExpireAnnotationsParanoid(mozIStorageConnection* aConnection)
{
  
  nsCAutoString session_query = NS_LITERAL_CSTRING(
    "DELETE FROM moz_annos WHERE expiration = ") +
    nsPrintfCString("%d", nsIAnnotationService::EXPIRE_SESSION);
  nsresult rv = aConnection->ExecuteSimpleSQL(session_query);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_items_annos WHERE id IN "
      "(SELECT a.id FROM moz_items_annos a "
      "LEFT OUTER JOIN moz_bookmarks b ON a.item_id = b.id "
      "WHERE b.id IS NULL)"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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
nsNavHistoryExpire::ExpireInputHistoryParanoid(mozIStorageConnection* aConnection)
{
  
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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











PRBool
nsNavHistoryExpire::ExpireForDegenerateRuns()
{
  
  
  if (mAnyEmptyRuns)
    return PR_FALSE;

  
  PRBool keepGoing;
  nsresult rv = ExpireItems(EXPIRATION_COUNT_PER_RUN_LARGE, &keepGoing);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireItems failed.");
  return PR_TRUE;
}







void
nsNavHistoryExpire::ComputeNextExpirationTime(
    mozIStorageConnection* aConnection)
{
  mNextExpirationTime = 0;

  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
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




nsresult
nsNavHistoryExpire::StartTimer(PRUint32 aMilleseconds)
{
  if (!mTimer)
    mTimer = do_CreateInstance("@mozilla.org/timer;1");
  NS_ENSURE_STATE(mTimer); 
  nsresult rv = mTimer->InitWithFuncCallback(TimerCallback, this,
                                             aMilleseconds,
                                             nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}




void 
nsNavHistoryExpire::TimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsNavHistoryExpire* that = static_cast<nsNavHistoryExpire*>(aClosure);
  that->mTimerSet = PR_FALSE;
  that->DoPartialExpiration();
}




PRTime
nsNavHistoryExpire::GetExpirationTimeAgo(PRInt32 aExpireDays)
{
  
  
  const PRInt32 maxDays = 106751991;
  if (aExpireDays > maxDays)
    aExpireDays = maxDays;

  
  
  return (PRTime)aExpireDays * 86400 * PR_USEC_PER_SEC;
}
