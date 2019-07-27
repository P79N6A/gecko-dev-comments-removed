






#ifndef nsIdleService_h__
#define nsIdleService_h__

#include "nsIIdleServiceInternal.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsIObserver.h"
#include "nsIIdleService.h"
#include "nsCategoryCache.h"
#include "nsWeakReference.h"
#include "mozilla/TimeStamp.h"





class IdleListener {
public:
  nsCOMPtr<nsIObserver> observer;
  uint32_t reqIdleTime;
  bool isIdle;

  IdleListener(nsIObserver* obs, uint32_t reqIT, bool aIsIdle = false) :
    observer(obs), reqIdleTime(reqIT), isIdle(aIsIdle) {}
  ~IdleListener() {}
};


class nsIdleService;




class nsIdleServiceDaily : public nsIObserver,
                           public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  explicit nsIdleServiceDaily(nsIIdleService* aIdleService);

  




  void Init();

private:
  virtual ~nsIdleServiceDaily();

  









  void StageIdleDaily(bool aHasBeenLongWait);

  




  nsIIdleService* mIdleService;

  



  nsCOMPtr<nsITimer> mTimer;

  


  static void DailyCallback(nsITimer* aTimer, void* aClosure);

  


  nsCategoryCache<nsIObserver> mCategoryObservers;

  


  bool mShutdownInProgress;

  



  PRTime mExpectedTriggerTime;

  




  int32_t mIdleDailyTriggerWait;
};

class nsIdleService : public nsIIdleServiceInternal
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDLESERVICE
  NS_DECL_NSIIDLESERVICEINTERNAL

protected:
  static already_AddRefed<nsIdleService> GetInstance();

  nsIdleService();
  virtual ~nsIdleService();

  















  virtual bool PollIdleTime(uint32_t* aIdleTime);

  




  virtual bool UsePollMode();

private:
  







  void SetTimerExpiryIfBefore(mozilla::TimeStamp aNextTimeout);

  


  mozilla::TimeStamp mCurrentlySetToTimeoutAt;

  



  nsCOMPtr<nsITimer> mTimer;

  


  nsTArray<IdleListener> mArrayListeners;

  


  nsRefPtr<nsIdleServiceDaily> mDailyIdle;

  


  uint32_t mIdleObserverCount;

  







  uint32_t mDeltaToNextIdleSwitchInS;

  


  mozilla::TimeStamp mLastUserInteraction;


  



  void ReconfigureTimer(void);

  




  static void StaticIdleTimerCallback(nsITimer* aTimer, void* aClosure);

  


  void IdleTimerCallback(void);
};

#endif 
