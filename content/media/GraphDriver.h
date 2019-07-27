




#ifndef GRAPHDRIVER_H_
#define GRAPHDRIVER_H_

#include "nsAutoPtr.h"
#include "nsAutoRef.h"
#include "AudioBufferUtils.h"
#include "AudioMixer.h"
#include "AudioSegment.h"

struct cubeb_stream;

namespace mozilla {







static const int MEDIA_GRAPH_TARGET_PERIOD_MS = 10;





static const int SCHEDULE_SAFETY_MARGIN_MS = 10;









static const int AUDIO_TARGET_MS = 2*MEDIA_GRAPH_TARGET_PERIOD_MS +
    SCHEDULE_SAFETY_MARGIN_MS;







static const int VIDEO_TARGET_MS = 2*MEDIA_GRAPH_TARGET_PERIOD_MS +
    SCHEDULE_SAFETY_MARGIN_MS;

class MediaStreamGraphImpl;
class MessageBlock;




typedef int64_t GraphTime;
const GraphTime GRAPH_TIME_MAX = MEDIA_TIME_MAX;

class AudioCallbackDriver;









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

  
  bool mNeedAnotherIteration;
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
  virtual void Start() MOZ_OVERRIDE;
  virtual void Stop() MOZ_OVERRIDE;
  virtual void Resume() MOZ_OVERRIDE;
  virtual void Revive() MOZ_OVERRIDE;
  



  void RunThread();
  friend class MediaStreamGraphInitThreadRunnable;
  uint32_t IterationDuration() {
    return MEDIA_GRAPH_TARGET_PERIOD_MS;
  }
protected:
  nsCOMPtr<nsIThread> mThread;
};





class SystemClockDriver : public ThreadedDriver
{
public:
  explicit SystemClockDriver(MediaStreamGraphImpl* aGraphImpl);
  virtual ~SystemClockDriver();
  virtual void GetIntervalForIteration(GraphTime& aFrom,
                                       GraphTime& aTo) MOZ_OVERRIDE;
  virtual GraphTime GetCurrentTime() MOZ_OVERRIDE;
  virtual void WaitForNextIteration() MOZ_OVERRIDE;
  virtual void WakeUp() MOZ_OVERRIDE;


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
                                       GraphTime& aTo) MOZ_OVERRIDE;
  virtual GraphTime GetCurrentTime() MOZ_OVERRIDE;
  virtual void WaitForNextIteration() MOZ_OVERRIDE;
  virtual void WakeUp() MOZ_OVERRIDE;
  virtual TimeStamp GetCurrentTimeStamp() MOZ_OVERRIDE;

private:
  
  GraphTime mSlice;
};





















class AudioCallbackDriver : public GraphDriver,
                            public MixerCallbackReceiver
{
public:
  explicit AudioCallbackDriver(MediaStreamGraphImpl* aGraphImpl,
                               dom::AudioChannel aChannel = dom::AudioChannel::Normal);
  virtual ~AudioCallbackDriver();

  virtual void Destroy() MOZ_OVERRIDE;
  virtual void Start() MOZ_OVERRIDE;
  virtual void Stop() MOZ_OVERRIDE;
  virtual void Resume() MOZ_OVERRIDE;
  virtual void Revive() MOZ_OVERRIDE;
  virtual void GetIntervalForIteration(GraphTime& aFrom,
                                       GraphTime& aTo) MOZ_OVERRIDE;
  virtual GraphTime GetCurrentTime() MOZ_OVERRIDE;
  virtual void WaitForNextIteration() MOZ_OVERRIDE;
  virtual void WakeUp() MOZ_OVERRIDE;

  
  static long DataCallback_s(cubeb_stream * aStream,
                             void * aUser, void * aBuffer,
                             long aFrames);
  static void StateCallback_s(cubeb_stream* aStream, void * aUser,
                              cubeb_state aState);
  static void DeviceChangedCallback_s(void * aUser);
  




  long DataCallback(AudioDataValue* aBuffer, long aFrames);
  

  void StateCallback(cubeb_state aState);
  

  uint32_t IterationDuration();

  

  virtual void MixerCallback(AudioDataValue* aMixedBuffer,
                             AudioSampleFormat aFormat,
                             uint32_t aChannels,
                             uint32_t aFrames,
                             uint32_t aSampleRate) MOZ_OVERRIDE;

  virtual AudioCallbackDriver* AsAudioCallbackDriver() {
    return this;
  }

  


  bool InCallback();

  

  bool IsStarted();

  

  void SetMicrophoneActive(bool aActive);
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
  dom::AudioChannel mAudioChannel;
  
  bool mInCallback;
  


  bool mPauseRequested;
  


  bool mMicrophoneActive;
};

class AsyncCubebTask : public nsRunnable
{
public:
  enum AsyncCubebOperation {
    INIT,
    SHUTDOWN,
    SLEEP
  };


  AsyncCubebTask(AudioCallbackDriver* aDriver, AsyncCubebOperation aOperation)
    : mDriver(aDriver),
      mOperation(aOperation)
  {
    MOZ_ASSERT(mDriver->mAudioStream || aOperation == INIT, "No audio stream !");
  }

  nsresult Dispatch()
  {
    
    nsresult rv = NS_NewNamedThread("CubebOperation", getter_AddRefs(mThread));
    if (NS_SUCCEEDED(rv)) {
      
      rv = mThread->Dispatch(this, NS_DISPATCH_NORMAL);
    }
    return rv;
  }

protected:
  virtual ~AsyncCubebTask() {};

private:
  NS_IMETHOD Run() MOZ_OVERRIDE MOZ_FINAL;
  nsCOMPtr<nsIThread> mThread;
  nsRefPtr<AudioCallbackDriver> mDriver;
  AsyncCubebOperation mOperation;
};

}

#endif
