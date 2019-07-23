











































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





#define PARTIAL_EXPIRATION_TIMEOUT 3500


#define SUBSEQUENT_EXPIRATION_TIMEOUT 20000







#define MAX_SEQUENTIAL_RUNS 1


#define PREF_SANITIZE_ON_SHUTDOWN   "privacy.sanitize.sanitizeOnShutdown"
#define PREF_SANITIZE_ITEM_HISTORY  "privacy.item.history"


const PRTime EXPIRATION_POLICY_DAYS = ((PRTime)7 * 86400 * PR_USEC_PER_SEC);
const PRTime EXPIRATION_POLICY_WEEKS = ((PRTime)30 * 86400 * PR_USEC_PER_SEC);
const PRTime EXPIRATION_POLICY_MONTHS = ((PRTime)180 * 86400 * PR_USEC_PER_SEC);


const PRTime EMBEDDED_LINK_LIFETIME = ((PRTime)1 * 86400 * PR_USEC_PER_SEC);


#define EXPIRATION_CAP_EMBEDDED 500


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

  
  
  
  rv = ExpireEmbeddedLinks(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireEmbeddedLinks failed.");

  
  
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService("@mozilla.org/preferences-service;1"));
  PRBool sanitizeOnShutdown, sanitizeHistory;
  prefs->GetBoolPref(PREF_SANITIZE_ON_SHUTDOWN, &sanitizeOnShutdown);
  prefs->GetBoolPref(PREF_SANITIZE_ITEM_HISTORY, &sanitizeHistory);
  if (sanitizeHistory && sanitizeOnShutdown)
    return;

  
  rv = ExpireHistoryParanoid(connection, EXPIRATION_CAP_PLACES);
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
}








nsresult
nsNavHistoryExpire::ClearHistory()
{
  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  NS_ENSURE_TRUE(connection, NS_ERROR_OUT_OF_MEMORY);

  
  nsresult rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits"));
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

  
  
  
  rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_places SET frecency = -1"));
  if (NS_FAILED(rv))
    NS_WARNING("failed to recent frecency");

  
  
  
  
  rv = mHistory->FixInvalidFrecenciesForExcludedPlaces();
  if (NS_FAILED(rv))
    NS_WARNING("failed to fix invalid frecencies");

  
  
  

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
      "SELECT v.id, v.place_id, v.visit_date, h.url, h.favicon_id, h.hidden, "
        "(SELECT fk FROM moz_bookmarks WHERE fk = h.id) "
      "FROM moz_places h JOIN moz_historyvisits v ON h.id = v.place_id "
      "WHERE v.visit_date < ?1 "
      "ORDER BY v.visit_date ASC LIMIT ?2"),
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
        "SELECT count(*) FROM moz_places WHERE visit_count > 0"),
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

  nsresult rv = aConnection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DELETE FROM moz_historyvisits WHERE id IN (") +
    deletedVisitIds +
    NS_LITERAL_CSTRING(")"));
  NS_ENSURE_SUCCESS(rv, rv);

  if (placeIds.IsEmpty())
    return NS_OK;

  
  
  
  
  rv = aConnection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING(
      "UPDATE moz_places "
      "SET frecency = -1 "
      "WHERE id IN ("
        "SELECT h.id FROM moz_places h "
        "LEFT OUTER JOIN moz_historyvisits v ON v.place_id = h.id "
        "LEFT OUTER JOIN moz_bookmarks b ON b.fk = h.id "
        "WHERE v.id IS NULL AND b.id IS NULL AND h.id IN (") +
    placeIds +
    NS_LITERAL_CSTRING("))"));
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

  return aConnection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DELETE FROM moz_places WHERE id IN( "
      "SELECT h.id "
      "FROM moz_places h "
      "LEFT OUTER JOIN moz_historyvisits v ON v.place_id = h.id "
      "LEFT OUTER JOIN moz_annos a ON a.place_id = h.id "
      "WHERE h.id IN(") + deletedPlaceIds +
      NS_LITERAL_CSTRING(") AND v.place_id IS NULL AND (a.expiration <> ") +
      nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER) +
      NS_LITERAL_CSTRING(" OR a.expiration IS NULL))"));
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

  
  return aConnection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DELETE FROM moz_favicons WHERE id IN ( "
      "SELECT f.id FROM moz_favicons f "
      "LEFT OUTER JOIN moz_places h ON f.id = h.favicon_id "
      "WHERE f.id IN (") + deletedFaviconIds +
      NS_LITERAL_CSTRING(") AND h.favicon_id IS NULL)"));
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
      "DELETE FROM moz_annos WHERE expiration = ?1 AND (?2 > MAX(COALESCE(lastModified, 0), dateAdded))"),
    getter_AddRefs(expirePagesStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<mozIStorageStatement> expireItemsStatement;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_items_annos WHERE expiration = ?1 AND (?2 > MAX(COALESCE(lastModified, 0), dateAdded))"),
    getter_AddRefs(expireItemsStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = expirePagesStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_DAYS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_DAYS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  rv = expireItemsStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_DAYS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_DAYS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = expirePagesStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_WEEKS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_WEEKS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expirePagesStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = expireItemsStatement->BindInt32Parameter(0, nsIAnnotationService::EXPIRE_WEEKS);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->BindInt64Parameter(1, (now - EXPIRATION_POLICY_WEEKS));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireItemsStatement->Execute();
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

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




nsresult
nsNavHistoryExpire::ExpireEmbeddedLinks(mozIStorageConnection* aConnection)
{
  PRTime maxEmbeddedAge = PR_Now() - EMBEDDED_LINK_LIFETIME;
  nsCOMPtr<mozIStorageStatement> expireEmbeddedLinksStatement;
  
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_historyvisits WHERE id IN ("
      "SELECT id FROM moz_historyvisits WHERE visit_date < ?1 "
      "AND (visit_type = ?2 OR visit_type = 0) LIMIT ?3)"),
    getter_AddRefs(expireEmbeddedLinksStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireEmbeddedLinksStatement->BindInt64Parameter(0, maxEmbeddedAge);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireEmbeddedLinksStatement->BindInt32Parameter(1, nsINavHistoryService::TRANSITION_EMBED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireEmbeddedLinksStatement->BindInt32Parameter(2, EXPIRATION_CAP_EMBEDDED);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = expireEmbeddedLinksStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}










nsresult
nsNavHistoryExpire::ExpireHistoryParanoid(mozIStorageConnection* aConnection,
                                          PRInt32 aMaxRecords)
{
  nsCAutoString query = NS_LITERAL_CSTRING(
    "DELETE FROM moz_places WHERE id IN ("
      "SELECT h.id FROM moz_places h "
        "LEFT OUTER JOIN moz_historyvisits v ON h.id = v.place_id "
        "LEFT OUTER JOIN moz_bookmarks b ON h.id = b.fk "
        "LEFT OUTER JOIN moz_annos a ON h.id = a.place_id "
      "WHERE v.id IS NULL AND b.id IS NULL AND (a.expiration != ") +
      nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER) +
      NS_LITERAL_CSTRING(" OR a.id IS NULL) AND SUBSTR(h.url,0,6) <> 'place:'");
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
    "DELETE FROM moz_favicons WHERE id IN "
    "(SELECT f.id FROM moz_favicons f "
     "LEFT OUTER JOIN moz_places h ON f.id = h.favicon_id "
     "WHERE h.favicon_id IS NULL)"));
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
    "DELETE FROM moz_annos WHERE id IN "
      "(SELECT a.id FROM moz_annos a "
      "LEFT OUTER JOIN moz_places p ON a.place_id = p.id "
      "LEFT OUTER JOIN moz_historyvisits v ON a.place_id = v.place_id "
      "WHERE p.id IS NULL "
      "OR (v.id IS NULL AND a.expiration != ") +
      nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER) +
      NS_LITERAL_CSTRING("))"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  nsCAutoString charsetAnno("URIProperties/characterSet");

  
  
  
  
  nsCOMPtr<mozIStorageStatement> migrateStatement;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
        "INSERT INTO moz_items_annos "
        "SELECT null, b.id, a.anno_attribute_id, a.mime_type, a.content, "
        " a.flags, a.expiration, a.type, a.dateAdded, a.lastModified "
        "FROM moz_annos a "
        "JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id "
        "JOIN moz_bookmarks b ON b.fk = a.place_id "
        "WHERE b.id IS NOT NULL AND n.name = ?1 AND a.expiration = ") +
        nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER),
      getter_AddRefs(migrateStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = migrateStatement->BindUTF8StringParameter(0, charsetAnno);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = migrateStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<mozIStorageStatement> cleanupStatement;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
        "DELETE FROM moz_annos WHERE id IN "
        "(SELECT a.id FROM moz_annos a "
        "JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id "
        "JOIN moz_bookmarks b ON b.fk = a.place_id "
        "WHERE b.id IS NOT NULL AND n.name = ?1 AND a.expiration = ") +
        nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER) +
        NS_LITERAL_CSTRING(")"),
      getter_AddRefs(cleanupStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cleanupStatement->BindUTF8StringParameter(0, charsetAnno);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cleanupStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_items_annos WHERE id IN "
      "(SELECT a.id FROM moz_items_annos a "
      "LEFT OUTER JOIN moz_bookmarks b ON a.item_id = b.id "
      "WHERE b.id IS NULL)"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_anno_attributes WHERE " 
    "id NOT IN (SELECT DISTINCT a.id FROM moz_anno_attributes a "
      "JOIN moz_annos b ON b.anno_attribute_id = a.id "
      "JOIN moz_places p ON b.place_id = p.id) "
    "AND "
    "id NOT IN (SELECT DISTINCT a.id FROM moz_anno_attributes a "
      "JOIN moz_items_annos c ON c.anno_attribute_id = a.id "
      "JOIN moz_bookmarks p ON c.item_id = p.id)"));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}






nsresult
nsNavHistoryExpire::ExpireInputHistoryParanoid(mozIStorageConnection* aConnection)
{
  
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_inputhistory WHERE place_id IN "
    "(SELECT i.place_id FROM moz_inputhistory i "
      "LEFT OUTER JOIN moz_places h ON i.place_id = h.id "
      "WHERE h.id IS NULL)"));
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
