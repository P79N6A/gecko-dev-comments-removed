




#ifndef GRAPHDRIVER_H_
#define GRAPHDRIVER_H_

#include "nsAutoPtr.h"
#include "nsAutoRef.h"
#include "AudioBufferUtils.h"
#include "AudioMixer.h"
#include "AudioSegment.h"
#include "SelfRef.h"
#include "mozilla/Atomics.h"
#include "AudioContext.h"

struct cubeb_stream;

template <>
class nsAutoRefTraits<cubeb_stream> : public nsPointerRefTraits<cubeb_stream>
{
public:
  static void Release(cubeb_stream* aStream) { cubeb_stream_destroy(aStream); }
};

namespace mozilla {






static const int MEDIA_GRAPH_TARGET_PERIOD_MS = 10;





static const int SCHEDULE_SAFETY_MARGIN_MS = 10;









static const int AUDIO_TARGET_MS = 2*MEDIA_GRAPH_TARGET_PERIOD_MS +
    SCHEDULE_SAFETY_MARGIN_MS;







static const int VIDEO_TARGET_MS = 2*MEDIA_GRAPH_TARGET_PERIOD_MS +
    SCHEDULE_SAFETY_MARGIN_MS;

class MediaStreamGraphImpl;




typedef int64_t GraphTime;
const GraphTime GRAPH_TIME_MAX = MEDIA_TIME_MAX;

class AudioCallbackDriver;
class OfflineClockDriver;









class GraphDriver
{
public:
  explicit GraphDriver(MediaStreamGraphImpl* aGraphImpl);

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GraphDriver);
  

  virtual void GetIntervalForIteration(GraphTime& aFrom,
                                       GraphTime& aTo) = 0;
  

  virtual GraphTime GetCurrentTime() = 0;
  

  virtual void WaitForNextIteration() = 0;
  
  virtual void WakeUp() = 0;
  virtual void Destroy() {}
  
  virtual void Start() = 0;
  
  virtual void Stop() = 0;
  
  virtual void Resume() = 0;
  
  virtual void Revive() = 0;
  void Shutdown();
  





  virtual uint32_t IterationDuration() = 0;

  
  bool Switching() {
    return mNextDriver || mPreviousDriver;
  }

  



  virtual TimeStamp GetCurrentTimeStamp() {
    return mCurrentTimeStamp;
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

  virtual void GetAudioBuffer(float** aBuffer, long& aFrames) {
    MOZ_CRASH("This is not an Audio GraphDriver!");
  }

  virtual AudioCallbackDriver* AsAudioCallbackDriver() {
    return nullptr;
  }

  virtual OfflineClockDriver* AsOfflineClockDriver() {
    return nullptr;
  }

  



  virtual void SwitchAtNextIteration(GraphDriver* aDriver);

  



  void SetGraphTime(GraphDriver* aPreviousDriver,
                    GraphTime aLastSwitchNextIterationStart,
                    GraphTime aLastSwitchNextIterationEnd,
                    GraphTime aLastSwitchNextStateComputedTime,
                    GraphTime aLastSwitchStateComputedTime);

  




  void UpdateStateComputedTime(GraphTime aStateComputedTime);

  



  void EnsureImmediateWakeUpLocked();

  




  void EnsureNextIteration();

  


  void EnsureNextIterationLocked();

  MediaStreamGraphImpl* GraphImpl() {
    return mGraphImpl;
  }

  virtual bool OnThread() = 0;

protected:
  
  GraphTime mIterationStart;
  
  GraphTime mIterationEnd;
  
  GraphTime mStateComputedTime;
  GraphTime mNextStateComputedTime;
  
  
  MediaStreamGraphImpl* mGraphImpl;

  
  enum WaitState {
    
    WAITSTATE_RUNNING,
    
    
    WAITSTATE_WAITING_FOR_NEXT_ITERATION,
    
    WAITSTATE_WAITING_INDEFINITELY,
    
    
    WAITSTATE_WAKING_UP
  };
  WaitState mWaitState;

  TimeStamp mCurrentTimeStamp;
  
  
  
  nsRefPtr<GraphDriver> mPreviousDriver;
  
  
  nsRefPtr<GraphDriver> mNextDriver;
  virtual ~GraphDriver()
  { }
};

class MediaStreamGraphInitThreadRunnable;




class ThreadedDriver : public GraphDriver
{
public:
  explicit ThreadedDriver(MediaStreamGraphImpl* aGraphImpl);
  virtual ~ThreadedDriver();
  virtual void Start() override;
  virtual void Stop() override;
  virtual void Resume() override;
  virtual void Revive() override;
  



  void RunThread();
  friend class MediaStreamGraphInitThreadRunnable;
  virtual uint32_t IterationDuration() override {
    return MEDIA_GRAPH_TARGET_PERIOD_MS;
  }

  virtual bool OnThread() override { return !mThread || NS_GetCurrentThread() == mThread; }

protected:
  nsCOMPtr<nsIThread> mThread;
};





class SystemClockDriver : public ThreadedDriver
{
public:
  explicit SystemClockDriver(MediaStreamGraphImpl* aGraphImpl);
  virtual ~SystemClockDriver();
  virtual void GetIntervalForIteration(GraphTime& aFrom,
                                       GraphTime& aTo) override;
  virtual GraphTime GetCurrentTime() override;
  virtual void WaitForNextIteration() override;
  virtual void WakeUp() override;


private:
  TimeStamp mInitialTimeStamp;
  TimeStamp mLastTimeStamp;
};





class OfflineClockDriver : public ThreadedDriver
{
public:
  OfflineClockDriver(MediaStreamGraphImpl* aGraphImpl, GraphTime aSlice);
  virtual ~OfflineClockDriver();
  virtual void GetIntervalForIteration(GraphTime& aFrom,
                                       GraphTime& aTo) override;
  virtual GraphTime GetCurrentTime() override;
  virtual void WaitForNextIteration() override;
  virtual void WakeUp() override;
  virtual TimeStamp GetCurrentTimeStamp() override;
  virtual OfflineClockDriver* AsOfflineClockDriver() override {
    return this;
  }

private:
  
  GraphTime mSlice;
};

struct StreamAndPromiseForOperation
{
  StreamAndPromiseForOperation(MediaStream* aStream,
                               void* aPromise,
                               dom::AudioContextOperation aOperation);
  nsRefPtr<MediaStream> mStream;
  void* mPromise;
  dom::AudioContextOperation mOperation;
};

enum AsyncCubebOperation {
  INIT,
  SHUTDOWN
};





















class AudioCallbackDriver : public GraphDriver,
                            public MixerCallbackReceiver
{
public:
  explicit AudioCallbackDriver(MediaStreamGraphImpl* aGraphImpl,
                               dom::AudioChannel aChannel = dom::AudioChannel::Normal);
  virtual ~AudioCallbackDriver();

  virtual void Destroy() override;
  virtual void Start() override;
  virtual void Stop() override;
  virtual void Resume() override;
  virtual void Revive() override;
  virtual void GetIntervalForIteration(GraphTime& aFrom,
                                       GraphTime& aTo) override;
  virtual GraphTime GetCurrentTime() override;
  virtual void WaitForNextIteration() override;
  virtual void WakeUp() override;

  
  static long DataCallback_s(cubeb_stream * aStream,
                             void * aUser, void * aBuffer,
                             long aFrames);
  static void StateCallback_s(cubeb_stream* aStream, void * aUser,
                              cubeb_state aState);
  static void DeviceChangedCallback_s(void * aUser);
  




  long DataCallback(AudioDataValue* aBuffer, long aFrames);
  

  void StateCallback(cubeb_state aState);
  

  virtual uint32_t IterationDuration() override;

  

  virtual void MixerCallback(AudioDataValue* aMixedBuffer,
                             AudioSampleFormat aFormat,
                             uint32_t aChannels,
                             uint32_t aFrames,
                             uint32_t aSampleRate) override;

  virtual AudioCallbackDriver* AsAudioCallbackDriver() override {
    return this;
  }

  

  void EnqueueStreamAndPromiseForOperation(MediaStream* aStream,
                                         void* aPromise,
                                         dom::AudioContextOperation aOperation);

  bool IsSwitchingDevice() {
#ifdef XP_MACOSX
    return mSelfReference;
#else
    return false;
#endif
  }

  


  bool InCallback();

  virtual bool OnThread() override { return !mStarted || InCallback(); }

  

  bool IsStarted();

  

  void SetMicrophoneActive(bool aActive);

  void CompleteAudioContextOperations(AsyncCubebOperation aOperation);
private:
  



  void PanOutputIfNeeded(bool aMicrophoneActive);
  

  void DeviceChangedCallback();
  
  void StartStream();
  friend class AsyncCubebTask;
  void Init();
  
  static const uint32_t ChannelCount = 2;
  



  SpillBuffer<AudioDataValue, WEBAUDIO_BLOCK_SIZE * 2, ChannelCount> mScratchBuffer;
  

  AudioCallbackBufferWrapper<AudioDataValue, ChannelCount> mBuffer;
  

  nsAutoRef<cubeb_stream> mAudioStream;
  
  uint32_t mSampleRate;
  

  uint32_t mIterationDurationMS;
  













  bool mStarted;

  struct AutoInCallback
  {
    explicit AutoInCallback(AudioCallbackDriver* aDriver);
    ~AutoInCallback();
    AudioCallbackDriver* mDriver;
  };

  

  nsCOMPtr<nsIThread> mInitShutdownThread;
  nsAutoTArray<StreamAndPromiseForOperation, 1> mPromisesForOperation;
  dom::AudioChannel mAudioChannel;
  Atomic<bool> mInCallback;
  


  bool mPauseRequested;
  


  bool mMicrophoneActive;

#ifdef XP_MACOSX
  

  bool OSXDeviceSwitchingWorkaround();
  

  SelfReference<AudioCallbackDriver> mSelfReference;
  

  uint32_t mCallbackReceivedWhileSwitching;
#endif
};

class AsyncCubebTask : public nsRunnable
{
public:

  AsyncCubebTask(AudioCallbackDriver* aDriver, AsyncCubebOperation aOperation);

  nsresult Dispatch()
  {
    
    nsresult rv = NS_NewNamedThread("CubebOperation", getter_AddRefs(mThread));
    if (NS_SUCCEEDED(rv)) {
      
      rv = mThread->Dispatch(this, NS_DISPATCH_NORMAL);
    }
    return rv;
  }

protected:
  virtual ~AsyncCubebTask();

private:
  NS_IMETHOD Run() override final;
  nsCOMPtr<nsIThread> mThread;
  nsRefPtr<AudioCallbackDriver> mDriver;
  AsyncCubebOperation mOperation;
  nsRefPtr<MediaStreamGraphImpl> mShutdownGrip;
};

}

#endif 
