





#ifndef MediaDecoderStateMachineScheduler_h__
#define MediaDecoderStateMachineScheduler_h__

#include "nsCOMPtr.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/DebugOnly.h"

class nsITimer;
class nsIEventTarget;

namespace mozilla {

class ReentrantMonitor;

class MediaDecoderStateMachineScheduler {
  enum State {
    SCHEDULER_STATE_NONE,
    SCHEDULER_STATE_SHUTDOWN
  };
public:
  MediaDecoderStateMachineScheduler(ReentrantMonitor& aMonitor,
                                    nsresult (*aTimeoutCallback)(void*),
                                    void* aClosure, bool aRealTime);
  ~MediaDecoderStateMachineScheduler();
  nsresult Init();
  nsresult Schedule(int64_t aUsecs = 0);
  void ScheduleAndShutdown();
  nsresult TimeoutExpired(int aTimerId);

  bool OnStateMachineThread() const;
  bool IsScheduled() const;

  bool IsRealTime() const {
    return mRealTime;
  }

  nsIEventTarget* GetStateMachineThread() const {
    return mEventTarget;
  }

private:
  void ResetTimer();

  
  
  nsresult (*const mTimeoutCallback)(void*);
  
  
  void* const mClosure;
  
  const bool mRealTime;
  
  ReentrantMonitor& mMonitor;
  
  const nsCOMPtr<nsIEventTarget> mEventTarget;
  
  nsCOMPtr<nsITimer> mTimer;
  
  TimeStamp mTimeout;
  
  int mTimerId;
  
  State mState;

  
  DebugOnly<bool> mInRunningStateMachine;
};

} 

#endif 
