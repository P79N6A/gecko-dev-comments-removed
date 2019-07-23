











































#include "nsNavHistory.h"
#include "mozStorageHelper.h"
#include "nsNetUtil.h"

struct nsNavHistoryExpireRecord {
  nsNavHistoryExpireRecord(mozIStorageStatement* statement);

  PRInt64 visitID;
  PRInt64 pageID;
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
nsNavHistoryExpire::OnQuit()
{
  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  if (! connection) {
    NS_NOTREACHED("No connection");
    return;
  }

  
  if (mTimer)
    mTimer->Cancel();

  
  ExpireForDegenerateRuns();

  
  ExpireHistoryParanoid(connection);
  ExpireFaviconsParanoid(connection);
  ExpireAnnotationsParanoid(connection);
}








nsresult
nsNavHistoryExpire::ClearHistory()
{
  PRBool keepGoing;

  mozIStorageConnection* connection = mHistory->GetStorageConnection();
  NS_ENSURE_TRUE(connection, NS_ERROR_OUT_OF_MEMORY);

  ExpireItems(0, &keepGoing);

  ExpireHistoryParanoid(connection);
  ExpireFaviconsParanoid(connection);
  ExpireAnnotationsParanoid(connection);

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
  ExpireItems(EXPIRATION_COUNT_PER_RUN, &keepGoing);

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

  
  EraseFavicons(connection, expiredVisits);
  EraseAnnotations(connection, expiredVisits);

  return NS_OK;
}







nsNavHistoryExpireRecord::nsNavHistoryExpireRecord(
                                              mozIStorageStatement* statement)
{
  visitID = statement->AsInt64(0);
  pageID = statement->AsInt64(1);
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
      "v.id, v.place_id, v.visit_date, h.url, h.favicon_id, h.hidden, b.item_child "
      "FROM moz_historyvisits v LEFT JOIN moz_places h ON v.place_id = h.id "
      "LEFT OUTER JOIN moz_bookmarks b on v.place_id = b.item_child");
  if (aExpireThreshold != 0)
    sql.AppendLiteral(" WHERE visit_date < ?1");
  rv = aConnection->CreateStatement(sql, getter_AddRefs(selectStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  if (aExpireThreshold != 0) {
    rv = selectStatement->BindInt64Parameter(0, aExpireThreshold);
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
  nsCOMPtr<mozIStorageStatement> deleteStatement;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits WHERE id = ?1"),
    getter_AddRefs(deleteStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  PRUint32 i;
  for (i = 0; i < aRecords.Length(); i ++) {
    deleteStatement->BindInt64Parameter(0, aRecords[i].visitID);
    rv = deleteStatement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}










nsresult
nsNavHistoryExpire::EraseHistory(mozIStorageConnection* aConnection,
    nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  nsCOMPtr<mozIStorageStatement> deleteStatement;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_places WHERE id = ?1"),
    getter_AddRefs(deleteStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> selectStatement;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT place_id FROM moz_historyvisits WHERE place_id = ?1"),
    getter_AddRefs(selectStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    if (aRecords[i].bookmarked)
      continue; 
    if (StringBeginsWith(aRecords[i].uri, NS_LITERAL_CSTRING("place:")))
      continue; 

    
    rv = selectStatement->BindInt64Parameter(0, aRecords[i].pageID);
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool hasVisit = PR_FALSE;
    rv = selectStatement->ExecuteStep(&hasVisit);
    selectStatement->Reset();
    if (hasVisit) continue;

    aRecords[i].erased = PR_TRUE;
    rv = deleteStatement->BindInt64Parameter(0, aRecords[i].pageID);
    rv = deleteStatement->Execute();
  }
  return NS_OK;
}




nsresult
nsNavHistoryExpire::EraseFavicons(mozIStorageConnection* aConnection,
    const nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  nsCOMPtr<mozIStorageStatement> selectStatement;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id FROM moz_places where favicon_id = ?1"),
    getter_AddRefs(selectStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> deleteStatement;
  rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_favicons WHERE id = ?1"),
    getter_AddRefs(deleteStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < aRecords.Length(); i ++) {
    if (! aRecords[i].erased)
      continue; 
    if (aRecords[i].faviconID == 0)
      continue; 
    selectStatement->BindInt64Parameter(0, aRecords[i].faviconID);

    
    PRBool hasEntry;
    if (NS_SUCCEEDED(selectStatement->ExecuteStep(&hasEntry)) && hasEntry) {
      selectStatement->Reset();
      continue; 
    }
    selectStatement->Reset();

    
    
    deleteStatement->BindInt64Parameter(0, aRecords[i].faviconID);
    deleteStatement->Execute();
  }
  return NS_OK;
}




nsresult
nsNavHistoryExpire::EraseAnnotations(mozIStorageConnection* aConnection,
    const nsTArray<nsNavHistoryExpireRecord>& aRecords)
{
  
  return NS_OK;
}







nsresult
nsNavHistoryExpire::ExpireHistoryParanoid(mozIStorageConnection* aConnection)
{
  
  
  nsresult rv = aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_places WHERE id IN (SELECT id FROM moz_places h "
      "LEFT OUTER JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT OUTER JOIN moz_bookmarks b ON h.id = b.item_child "
      "WHERE v.id IS NULL "
      "AND b.item_child IS NULL "
      "AND SUBSTR(url,0,6) <> 'place:')"));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}






nsresult
nsNavHistoryExpire::ExpireFaviconsParanoid(mozIStorageConnection* aConnection)
{
  return aConnection->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_favicons WHERE id IN "
    "(SELECT f.id FROM moz_favicons f "
     "LEFT OUTER JOIN moz_places h ON f.id = h.favicon_id "
     "WHERE h.favicon_id IS NULL)"));
}




nsresult
nsNavHistoryExpire::ExpireAnnotationsParanoid(mozIStorageConnection* aConnection)
{
  
  
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
  ExpireItems(mAddCount - mExpiredItems, &keepGoing);
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
  nsNavHistoryExpire* that = NS_STATIC_CAST(nsNavHistoryExpire*, aClosure);
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
