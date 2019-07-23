











































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





#define PARTIAL_EXPIRATION_TIMEOUT 3500


#define SUBSEQUENT_EXIPRATION_TIMEOUT 20000







#define MAX_SEQUENTIAL_RUNS 1


const PRTime EXPIRATION_POLICY_DAYS = ((PRTime)7 * 86400 * PR_USEC_PER_SEC);
const PRTime EXPIRATION_POLICY_WEEKS = ((PRTime)30 * 86400 * PR_USEC_PER_SEC);
const PRTime EXPIRATION_POLICY_MONTHS = ((PRTime)180 * 86400 * PR_USEC_PER_SEC);







nsNavHistoryExpire::nsNavHistoryExpire(nsNavHistory* aHistory) :
    mHistory(aHistory),
    mSequentialRuns(0),
    mTimerSet(PR_FALSE),
    mAnyEmptyRuns(PR_FALSE),
    mNextExpirationTime(0),
    mAddCount(0),
    mExpiredItems(0),
    mExpireRuns(0)
{

}




nsNavHistoryExpire::~nsNavHistoryExpire()
{

}











void
nsNavHistoryExpire::OnAddURI(PRTime aNow)
{
  mAddCount ++;
  mSequentialRuns = 0;

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

  
  rv = ExpireHistoryParanoid(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireHistoryParanoid failed.");
  rv = ExpireFaviconsParanoid(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireFaviconsParanoid failed.");
  rv = ExpireAnnotationsParanoid(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireAnnotationsParanoid failed.");
}








nsresult
nsNavHistoryExpire::ClearHistory()
{
  PRBool keepGoing;

  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  NS_ENSURE_TRUE(connection, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = ExpireItems(0, &keepGoing);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireItems failed.");

  rv = ExpireHistoryParanoid(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireHistoryParanoid failed.");

  rv = ExpireFaviconsParanoid(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireFaviconsParanoid failed.");

  rv = ExpireAnnotationsParanoid(connection);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireAnnotationsParanoid failed.");

  ENUMERATE_WEAKARRAY(mHistory->mObservers, nsINavHistoryObserver,
                      OnClearHistory())

  return NS_OK;
}








void
nsNavHistoryExpire::OnExpirationChanged()
{
  mNextExpirationTime = 0;
}




nsresult
nsNavHistoryExpire::DoPartialExpiration()
{
  mSequentialRuns ++;

  
  PRBool keepGoing;
  nsresult rv = ExpireItems(EXPIRATION_COUNT_PER_RUN, &keepGoing);
  if (NS_FAILED(rv))
    NS_WARNING("ExpireItems failed.");

  if (keepGoing && mSequentialRuns < MAX_SEQUENTIAL_RUNS)
    StartTimer(SUBSEQUENT_EXIPRATION_TIMEOUT);
  return NS_OK;
}












nsresult
nsNavHistoryExpire::ExpireItems(PRUint32 aNumToExpire, PRBool* aKeepGoing)
{
  
  mExpireRuns ++;

  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  NS_ENSURE_TRUE(connection, NS_ERROR_OUT_OF_MEMORY);

  
  
  
  mozStorageTransaction transaction(connection, PR_TRUE);

  *aKeepGoing = PR_TRUE;

  PRInt64 expireTime;
  if (aNumToExpire == 0) {
    
    expireTime = 0;
  } else {
    expireTime = PR_Now() - GetExpirationTimeAgo();
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
  nsresult rv;

  
  
  nsCOMPtr<mozIStorageStatement> selectStatement;
  nsCString sql;
  sql.AssignLiteral("SELECT "
      "v.id, v.place_id, v.visit_date, h.url, h.favicon_id, h.hidden, b.fk "
      "FROM moz_historyvisits v LEFT JOIN moz_places h ON v.place_id = h.id "
      "LEFT OUTER JOIN moz_bookmarks b on v.place_id = b.fk AND b.type = ?1 ");
  if (aExpireThreshold != 0)
    sql.AppendLiteral(" WHERE visit_date < ?2");
  rv = aConnection->CreateStatement(sql, getter_AddRefs(selectStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = selectStatement->BindInt32Parameter(0, nsINavBookmarksService::TYPE_BOOKMARK);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aExpireThreshold != 0) {
    rv = selectStatement->BindInt64Parameter(1, aExpireThreshold);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(selectStatement->ExecuteStep(&hasMore)) && hasMore &&
         (aNumToExpire == 0 || aRecords.Length() < aNumToExpire)) {
    nsNavHistoryExpireRecord record(selectStatement);
    aRecords.AppendElement(record);
  }
  return NS_OK;
}




nsresult
nsNavHistoryExpire::EraseVisits(mozIStorageConnection* aConnection,
    const nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCString deletedVisitIds;
  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    
    if (! deletedVisitIds.IsEmpty())
      deletedVisitIds.AppendLiteral(", ");
    deletedVisitIds.AppendInt(aRecords[i].visitID);
  }

  if (deletedVisitIds.IsEmpty())
    return NS_OK;

  return aConnection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DELETE FROM moz_historyvisits WHERE id IN (") +
    deletedVisitIds +
    NS_LITERAL_CSTRING(")"));
}











nsresult
nsNavHistoryExpire::EraseHistory(mozIStorageConnection* aConnection,
    nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCString deletedPlaceIds;
  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    
    if (aRecords[i].bookmarked ||
        StringBeginsWith(aRecords[i].uri, NS_LITERAL_CSTRING("place:")))
      continue;
    
    if (! deletedPlaceIds.IsEmpty())
      deletedPlaceIds.AppendLiteral(", ");
    deletedPlaceIds.AppendInt(aRecords[i].placeID);
    aRecords[i].erased = PR_TRUE;
  }

  if (deletedPlaceIds.IsEmpty())
    return NS_OK;

  return aConnection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DELETE FROM moz_places WHERE id IN( ") +
      deletedPlaceIds +
      NS_LITERAL_CSTRING(") AND id IN( "    
      "SELECT h.id FROM moz_places h "
      "WHERE (SELECT id from moz_historyvisits WHERE place_id=h.id) IS NULL "
      ") AND id NOT IN( " 
      "SELECT DISTINCT place_id FROM moz_annos WHERE expiration = ") +
      nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER) +
      NS_LITERAL_CSTRING(")"));              
}




nsresult
nsNavHistoryExpire::EraseFavicons(mozIStorageConnection* aConnection,
    const nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCString deletedFaviconIds;
  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    
    if (! aRecords[i].erased || aRecords[i].faviconID == 0)
      continue;
    
    if (! deletedFaviconIds.IsEmpty())
      deletedFaviconIds.AppendLiteral(", ");
    deletedFaviconIds.AppendInt(aRecords[i].faviconID);
  }

  if (deletedFaviconIds.IsEmpty())
    return NS_OK;

  
  return aConnection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DELETE FROM moz_favicons WHERE id IN (") +
    deletedFaviconIds +
    NS_LITERAL_CSTRING(") AND id IN "
      "(SELECT f.id FROM moz_favicons f "
      "LEFT OUTER JOIN moz_places h ON f.id = h.favicon_id "
      "WHERE h.favicon_id IS NULL)"));
}




nsresult
nsNavHistoryExpire::EraseAnnotations(mozIStorageConnection* aConnection,
    const nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCString placeIds;
  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    if (!placeIds.IsEmpty())
      placeIds.AppendLiteral(", ");
    placeIds.AppendInt(aRecords[i].placeID);
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
  mozStorageTransaction transaction(aConnection, PR_TRUE);

  
  
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
nsNavHistoryExpire::ExpireHistoryParanoid(mozIStorageConnection* aConnection)
{
  
  
  nsresult rv = aConnection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DELETE FROM moz_places "
      "WHERE id IN (SELECT h.id FROM moz_places h "
      "LEFT OUTER JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT OUTER JOIN moz_bookmarks b ON h.id = b.fk "
      "LEFT OUTER JOIN moz_annos a ON h.id = a.place_id "
      "WHERE v.id IS NULL "
      "AND b.id IS NULL "
      "AND a.expiration = ") +
      nsPrintfCString("%d", nsIAnnotationService::EXPIRE_NEVER) +
      NS_LITERAL_CSTRING(" AND a.id IS NULL "
      "AND SUBSTR(h.url,0,6) <> 'place:')"));
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
      "WHERE p.id IS NULL)"));
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











PRBool
nsNavHistoryExpire::ExpireForDegenerateRuns()
{
  
  
  if (mAnyEmptyRuns)
    return PR_FALSE;

  
  
  if (mAddCount < 10 || mAddCount < mExpiredItems)
    return PR_FALSE;

  
  
  PRBool keepGoing;
  nsresult rv = ExpireItems(mAddCount - mExpiredItems, &keepGoing);
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
  if (NS_FAILED(rv) || ! hasMore)
    return; 
            

  PRTime minTime = statement->AsInt64(0);
  mNextExpirationTime = minTime + GetExpirationTimeAgo();
}




nsresult
nsNavHistoryExpire::StartTimer(PRUint32 aMilleseconds)
{
  if (! mTimer)
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
nsNavHistoryExpire::GetExpirationTimeAgo()
{
  PRInt64 expireDays = mHistory->mExpireDays;

  
  
  const PRInt64 maxDays = 106751991;
  if (expireDays > maxDays)
    expireDays = maxDays;

  
  const PRInt64 secsPerDay = 24*60*60;
  const PRInt64 usecsPerSec = 1000000;
  const PRInt64 usecsPerDay = secsPerDay * usecsPerSec;
  return expireDays * usecsPerDay;
}
