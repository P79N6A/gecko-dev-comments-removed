











































class mozIStorageConnection;
class nsNavHistory;
struct nsNavHistoryExpireRecord;

class nsNavHistoryExpire
{
public:
  nsNavHistoryExpire(nsNavHistory* aHistory);
  ~nsNavHistoryExpire();

  void OnAddURI(PRTime aNow);
  void OnDeleteURI();
  void OnQuit();
  nsresult ClearHistory();
  void OnExpirationChanged();
  nsresult ExpireItems(PRUint32 aNumToExpire, PRBool* aKeepGoing);

protected:

  nsNavHistory* mHistory;

  nsCOMPtr<nsITimer> mTimer;
  PRBool mTimerSet;

  
  
  
  PRBool mAnyEmptyRuns;

  
  
  
  PRTime mNextExpirationTime;
  void ComputeNextExpirationTime(mozIStorageConnection* aConnection);

  
  PRUint32 mAddCount;
  PRUint32 mExpiredItems;

  nsresult DoPartialExpiration();

  nsresult ExpireAnnotations(mozIStorageConnection* aConnection);

  
  nsresult FindVisits(PRTime aExpireThreshold, PRUint32 aNumToExpire,
                      mozIStorageConnection* aConnection,
                      nsTArray<nsNavHistoryExpireRecord>& aRecords);
  nsresult EraseVisits(mozIStorageConnection* aConnection,
                       const nsTArray<nsNavHistoryExpireRecord>& aRecords);
  nsresult EraseHistory(mozIStorageConnection* aConnection,
                        nsTArray<nsNavHistoryExpireRecord>& aRecords);
  nsresult EraseFavicons(mozIStorageConnection* aConnection,
                         const nsTArray<nsNavHistoryExpireRecord>& aRecords);
  nsresult EraseAnnotations(mozIStorageConnection* aConnection,
                            const nsTArray<nsNavHistoryExpireRecord>& aRecords);

  
  nsresult ExpireHistoryParanoid(mozIStorageConnection* aConnection, PRInt32 aMaxRecords);
  nsresult ExpireFaviconsParanoid(mozIStorageConnection* aConnection);
  nsresult ExpireAnnotationsParanoid(mozIStorageConnection* aConnection);
  nsresult ExpireInputHistoryParanoid(mozIStorageConnection* aConnection);

  PRBool ExpireForDegenerateRuns();

  nsresult StartTimer(PRUint32 aMilleseconds);
  static void TimerCallback(nsITimer* aTimer, void* aClosure);

  PRTime GetExpirationTimeAgo(PRInt32 aExpireDays);
};
