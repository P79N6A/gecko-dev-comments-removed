





#ifndef NetEventTokenBucket_h__
#define NetEventTokenBucket_h__

#include "nsCOMPtr.h"
#include "nsDeque.h"
#include "nsITimer.h"

#include "mozilla/TimeStamp.h"

class nsICancelable;

namespace mozilla {
namespace net {








































class EventTokenBucket;

class ATokenBucketEvent 
{
public:
  virtual void OnTokenBucketAdmitted() = 0;
};

class TokenBucketCancelable;

class EventTokenBucket : public nsITimerCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  
  EventTokenBucket(uint32_t eventsPerSecond, uint32_t burstSize);
  virtual ~EventTokenBucket();

  
  void ClearCredits();
  uint32_t BurstEventsAvailable();
  uint32_t QueuedEvents();

  
  
  void Pause();
  void UnPause();
  void Stop() { mStopped = true; }

  
  nsresult SubmitEvent(ATokenBucketEvent *event, nsICancelable **cancelable);

private:
  friend class RunNotifyEvent;
  friend class SetTimerEvent;

  bool TryImmediateDispatch(TokenBucketCancelable *event);
  void SetRate(uint32_t eventsPerSecond, uint32_t burstSize);

  void DispatchEvents();
  void UpdateTimer();
  void UpdateCredits();

  const static uint64_t kUsecPerSec =  1000000;
  const static uint64_t kUsecPerMsec = 1000;
  const static uint64_t kMaxHz = 10000;

  uint64_t mUnitCost;   
  uint64_t mMaxCredit; 
  uint64_t mCredit; 

  bool     mPaused;
  bool     mStopped;
  nsDeque  mEvents;
  bool     mTimerArmed;
  TimeStamp mLastUpdate;

  
  
  
  nsCOMPtr<nsITimer> mTimer;

#ifdef XP_WIN
  
  
  
  
  const static uint64_t kCostFineGrainThreshold =  50 * kUsecPerMsec;

  void FineGrainTimers(); 
  void NormalTimers(); 
  void WantNormalTimers(); 
  void FineGrainResetTimerNotify(); 

  TimeStamp mLastFineGrainTimerUse;
  bool mFineGrainTimerInUse;
  bool mFineGrainResetTimerArmed;
  nsCOMPtr<nsITimer> mFineGrainResetTimer;
#endif
};

} 
} 

#endif
