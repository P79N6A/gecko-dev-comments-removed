




#ifndef GRAPHDRIVER_H_
#define GRAPHDRIVER_H_

namespace mozilla {





static const int32_t INITIAL_CURRENT_TIME = 1;

class MediaStreamGraphImpl;
class MessageBlock;




typedef int64_t GraphTime;
const GraphTime GRAPH_TIME_MAX = MEDIA_TIME_MAX;









class GraphDriver
{
public:
  GraphDriver(MediaStreamGraphImpl* aGraphImpl);
  virtual ~GraphDriver()
  { }

  



  virtual void RunThread() = 0;
  

  virtual void GetIntervalForIteration(GraphTime& aFrom,
                                       GraphTime& aTo) = 0;
  

  virtual GraphTime GetCurrentTime() = 0;
  

  virtual void WaitForNextIteration() = 0;
  
  virtual void WakeUp() = 0;
  
  virtual void Start() = 0;
  
  virtual void Stop() = 0;
  
  virtual void Dispatch(nsIRunnable* aEvent) = 0;

  virtual TimeStamp GetCurrentTimeStamp() {
    MOZ_ASSERT(false, "This clock does not support getting the current time stamp.");
  }

  bool IsWaiting() {
    return mWaitState == WAITSTATE_WAITING_INDEFINITELY ||
           mWaitState == WAITSTATE_WAITING_FOR_NEXT_ITERATION;
  }

  bool IsWaitingIndefinitly() {
    return mWaitState == WAITSTATE_WAITING_INDEFINITELY;
  }

  GraphTime IterationStart() {
    return mIterationStart;
  }

  GraphTime IterationEnd() {
    return mIterationEnd;
  }

  GraphTime StateComputedTime() {
    return mStateComputedTime;
  }

  void UpdateStateComputedTime(GraphTime aStateComputedTime) {
    MOZ_ASSERT(aStateComputedTime > mIterationEnd);

    mStateComputedTime = aStateComputedTime;
  }

  Monitor& GetThreadMonitor() {
    return mMonitor;
  }

  



  void EnsureImmediateWakeUpLocked() {
    mMonitor.AssertCurrentThreadOwns();
    mWaitState = WAITSTATE_WAKING_UP;
    mMonitor.Notify();
  }

  




  void EnsureNextIteration() {
    MonitorAutoLock lock(mMonitor);
    EnsureNextIterationLocked();
  }

  


  void EnsureNextIterationLocked() {
    mMonitor.AssertCurrentThreadOwns();

    if (mNeedAnotherIteration) {
      return;
    }
    mNeedAnotherIteration = true;
    if (IsWaitingIndefinitly()) {
      WakeUp();
    }
  }

protected:
  
  GraphTime mIterationStart;
  
  GraphTime mIterationEnd;
  
  GraphTime mStateComputedTime;
  
  
  MediaStreamGraphImpl* mGraphImpl;

  


  enum WaitState {
    
    WAITSTATE_RUNNING,
    
    
    WAITSTATE_WAITING_FOR_NEXT_ITERATION,
    
    WAITSTATE_WAITING_INDEFINITELY,
    
    
    WAITSTATE_WAKING_UP
  };
  WaitState mWaitState;

  bool mNeedAnotherIteration;
  
  Monitor mMonitor;
};






class DriverHolder
{
public:
  DriverHolder(MediaStreamGraphImpl* aGraphImpl);
  GraphTime GetCurrentTime();

  void Switch(GraphDriver* aDriver);

  GraphDriver* GetDriver() {
    MOZ_ASSERT(mDriver);
    return mDriver.get();
  }

protected:
  
  nsAutoPtr<GraphDriver> mDriver;
  
  
  MediaStreamGraphImpl* mGraphImpl;
  
  
  GraphTime mLastSwitchOffset;
};





class SystemClockDriver : public GraphDriver
{
public:
  SystemClockDriver(MediaStreamGraphImpl* aGraphImpl);
  virtual ~SystemClockDriver();
  virtual void Start() MOZ_OVERRIDE;
  virtual void Stop() MOZ_OVERRIDE;
  virtual void Dispatch(nsIRunnable* aEvent) MOZ_OVERRIDE;
  virtual void RunThread() MOZ_OVERRIDE;
  virtual void GetIntervalForIteration(GraphTime& aFrom,
                                       GraphTime& aTo) MOZ_OVERRIDE;
  virtual GraphTime GetCurrentTime() MOZ_OVERRIDE;
  virtual void WaitForNextIteration() MOZ_OVERRIDE;
  virtual void WakeUp() MOZ_OVERRIDE;

  virtual TimeStamp GetCurrentTimeStamp() MOZ_OVERRIDE;

private:
  TimeStamp mInitialTimeStamp;
  TimeStamp mLastTimeStamp;
  TimeStamp mCurrentTimeStamp;
  nsCOMPtr<nsIThread> mThread;
};





class OfflineClockDriver : public GraphDriver
{
public:
  OfflineClockDriver(MediaStreamGraphImpl* aGraphImpl, GraphTime aSlice);
  virtual ~OfflineClockDriver();
  virtual void Start() MOZ_OVERRIDE;
  virtual void Stop() MOZ_OVERRIDE;
  virtual void Dispatch(nsIRunnable* aEvent) MOZ_OVERRIDE;
  virtual void RunThread() MOZ_OVERRIDE;
  virtual void GetIntervalForIteration(GraphTime& aFrom,
                                       GraphTime& aTo) MOZ_OVERRIDE;
  virtual GraphTime GetCurrentTime() MOZ_OVERRIDE;
  virtual void WaitForNextIteration() MOZ_OVERRIDE;
  virtual void WakeUp() MOZ_OVERRIDE;

private:
  
  GraphTime mSlice;
  nsCOMPtr<nsIThread> mThread;
};

}

#endif 
