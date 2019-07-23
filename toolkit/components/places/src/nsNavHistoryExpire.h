













































class mozIStorageConnection;
class nsNavHistory;
struct nsNavHistoryExpireRecord;

class nsNavHistoryExpire
{
public:
  nsNavHistoryExpire();
  ~nsNavHistoryExpire();

  



  void OnDeleteVisits();

  


  void OnQuit();

  




  void OnExpirationChanged();

  




  nsresult ClearHistory();

  









  bool ExpireItems(PRUint32 aNumToExpire);

  







  void ExpireOrphans(PRUint32 aNumToExpire);

protected:
  nsNavHistory *mHistory;
  mozIStorageConnection *mDBConn;

  nsCOMPtr<nsITimer> mPartialExpirationTimer;
  void StartPartialExpirationTimer(PRUint32 aMilleseconds);
  static void PartialExpirationTimerCallback(nsITimer *aTimer, void *aClosure);

  
  
  
  PRTime mNextExpirationTime;

  



  void ComputeNextExpirationTime();

  nsresult DoPartialExpiration();

  


  void InitializeIdleTimer(PRUint32 aTimeInMs);
  nsCOMPtr<nsITimer> mIdleTimer;
  static void IdleTimerCallback(nsITimer *aTimer, void *aClosure);

  




  void OnIdle();

  PRTime GetExpirationTimeAgo(PRInt32 aExpireDays);

  nsresult ExpireAnnotations();

  












  nsresult FindVisits(PRTime aExpireThreshold, PRUint32 aNumToExpire,
                      nsTArray<nsNavHistoryExpireRecord> &aRecords);

  nsresult EraseVisits(const nsTArray<nsNavHistoryExpireRecord> &aRecords);

  







  nsresult EraseHistory(nsTArray<nsNavHistoryExpireRecord> &aRecords);

  nsresult EraseFavicons(const nsTArray<nsNavHistoryExpireRecord> &aRecords);

  






  nsresult EraseAnnotations(const nsTArray<nsNavHistoryExpireRecord> &aRecords);

  






  nsresult ExpireHistoryParanoid(PRInt32 aMaxRecords);

  


  nsresult ExpireFaviconsParanoid();

  



  nsresult ExpireAnnotationsParanoid();

  


  nsresult ExpireInputHistoryParanoid();
};
