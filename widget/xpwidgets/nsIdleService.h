








































#ifndef nsIdleService_h__
#define nsIdleService_h__

#include "nsIIdleService.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsIObserver.h"
#include "nsIIdleService.h"
#include "nsCategoryCache.h"





class IdleListener {
public:
  nsCOMPtr<nsIObserver> observer;
  PRUint32 reqIdleTime;
  bool isIdle;

  IdleListener(nsIObserver* obs, PRUint32 reqIT, bool aIsIdle = false) :
    observer(obs), reqIdleTime(reqIT), isIdle(aIsIdle) {}
  ~IdleListener() {}
};


class nsIdleService;




class nsIdleServiceDaily : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsIdleServiceDaily(nsIIdleService* aIdleService);

  




  void Init();

  virtual ~nsIdleServiceDaily();

private:
  




  nsIIdleService* mIdleService;

  


  bool mObservesIdle;

  



  nsCOMPtr<nsITimer> mTimer;

  


  static void DailyCallback(nsITimer* aTimer, void* aClosure);

  


  nsCategoryCache<nsIObserver> mCategoryObservers;
};

class nsIdleService : public nsIIdleService
{
public:
  nsIdleService();

  
  NS_IMETHOD AddIdleObserver(nsIObserver* aObserver, PRUint32 aIdleTime);
  NS_IMETHOD RemoveIdleObserver(nsIObserver* aObserver, PRUint32 aIdleTime);
  NS_IMETHOD GetIdleTime(PRUint32* idleTime);

  void ResetIdleTimeOut();

protected:
  ~nsIdleService();

  















  virtual bool PollIdleTime(PRUint32* aIdleTime);

  




  virtual bool UsePollMode();

  





  void CheckAwayState(bool aNewObserver);

private:
  






  void StartTimer(PRUint32 aDelay);

  



  void StopTimer();

  



  nsCOMPtr<nsITimer> mTimer;

  


  nsTArray<IdleListener> mArrayListeners;

  


  nsRefPtr<nsIdleServiceDaily> mDailyIdle;

  





  PRUint32 mLastIdleReset;

  






  PRUint32 mLastHandledActivity;

  


  static void IdleTimerCallback(nsITimer* aTimer, void* aClosure);

  



  bool mPolledIdleTimeIsValid;

  








  bool TryNotifyBackState(PRUint32 aIdleTime);

  








  bool TryNotifyIdleState(PRUint32 aIdleTime);

  





  void RescheduleIdleTimer(PRUint32 aIdleTime);
};

#endif 
