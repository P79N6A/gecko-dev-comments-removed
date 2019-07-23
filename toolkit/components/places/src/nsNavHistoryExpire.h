











































class mozIStorageConnection;
class nsNavHistory;
class nsNavHistoryExpireRecord;

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

protected:

  nsNavHistory* mHistory;

  
  PRUint32 mSequentialRuns;

  nsCOMPtr<nsITimer> mTimer;
  PRBool mTimerSet;

  
  
  
  PRBool mAnyEmptyRuns;

  
  
  
  PRTime mNextExpirationTime;
  void ComputeNextExpirationTime(mozIStorageConnection* aConnection);

  
  PRUint32 mAddCount;
  PRUint32 mExpiredItems;
  PRUint32 mExpireRuns;

  nsresult DoPartialExpiration();

  nsresult ExpireItems(PRUint32 aNumToExpire, PRBool* aKeepGoing);
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

  
  nsresult ExpireHistoryParanoid(mozIStorageConnection* aConnection);
  nsresult ExpireFaviconsParanoid(mozIStorageConnection* aConnection);
  nsresult ExpireAnnotationsParanoid(mozIStorageConnection* aConnection);

  PRBool ExpireForDegenerateRuns();

  nsresult StartTimer(PRUint32 aMilleseconds);
  static void TimerCallback(nsITimer* aTimer, void* aClosure);

  PRTime GetExpirationTimeAgo();
};
