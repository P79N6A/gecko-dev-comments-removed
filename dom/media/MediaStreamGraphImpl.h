




#ifndef MOZILLA_MEDIASTREAMGRAPHIMPL_H_
#define MOZILLA_MEDIASTREAMGRAPHIMPL_H_

#include "MediaStreamGraph.h"

#include "mozilla/Monitor.h"
#include "mozilla/TimeStamp.h"
#include "nsIMemoryReporter.h"
#include "nsIThread.h"
#include "nsIRunnable.h"
#include "Latency.h"
#include "mozilla/WeakPtr.h"
#include "GraphDriver.h"
#include "AudioMixer.h"

namespace mozilla {

template <typename T>
class LinkedList;
#ifdef MOZ_WEBRTC
class AudioOutputObserver;
#endif





struct StreamUpdate {
  int64_t mGraphUpdateIndex;
  nsRefPtr<MediaStream> mStream;
  StreamTime mNextMainThreadCurrentTime;
  bool mNextMainThreadFinished;
};





class ControlMessage {
public:
  explicit ControlMessage(MediaStream* aStream) : mStream(aStream)
  {
    MOZ_COUNT_CTOR(ControlMessage);
  }
  
  virtual ~ControlMessage()
  {
    MOZ_COUNT_DTOR(ControlMessage);
  }
  
  
  
  
  virtual void Run() = 0;
  
  
  
  virtual void RunDuringShutdown() {}
  MediaStream* GetStream() { return mStream; }

protected:
  
  
  
  MediaStream* mStream;
};

class MessageBlock {
public:
  int64_t mGraphUpdateIndex;
  nsTArray<nsAutoPtr<ControlMessage> > mMessages;
};









class MediaStreamGraphImpl : public MediaStreamGraph,
                             public nsIMemoryReporter {
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER

  






  explicit MediaStreamGraphImpl(bool aRealtime,
                                TrackRate aSampleRate,
                                bool aStartWithAudioDriver = false,
                                dom::AudioChannel aChannel = dom::AudioChannel::Normal);

  



  void Destroy();

  
  






  void RunInStableState(bool aSourceIsMSG);
  




  void EnsureRunInStableState();
  


  void ApplyStreamUpdate(StreamUpdate* aUpdate);
  



  void AppendMessage(ControlMessage* aMessage);
  





  void ForceShutDown();
  


  void ShutdownThreads();

  


  void Init();
  
  
  void AssertOnGraphThreadOrNotRunning() {
    
    
#ifdef DEBUG
    
    if (!mDriver->OnThread()) {
      if (!(mDetectedNotRunning &&
            mLifecycleState > LIFECYCLE_RUNNING &&
            NS_IsMainThread())) {
        mMonitor.AssertCurrentThreadOwns();
      }
    }
#endif
  }
  



  void DoIteration();

  bool OneIteration(GraphTime aFrom, GraphTime aTo,
                    GraphTime aStateFrom, GraphTime aStateEnd);

  bool Running() {
    mMonitor.AssertCurrentThreadOwns();
    return mLifecycleState == LIFECYCLE_RUNNING;
  }

  
  nsTArray<MessageBlock>& MessageQueue() {
    mMonitor.AssertCurrentThreadOwns();
    return mFrontMessageQueue;
  }

  

  GraphTime IterationEnd();
  




  void EnsureStableStateEventPosted();
  



  void PrepareUpdatesToMainThreadState(bool aFinalUpdate);
  



  bool AllFinishedStreamsNotified();
  




  bool ShouldUpdateMainThread();
  
  


  void UpdateCurrentTimeForStreams(GraphTime aPrevCurrentTime,
                                   GraphTime aNextCurrentTime);
  



  void UpdateGraph(GraphTime aEndBlockingDecisions);

  void SwapMessageQueues() {
    mMonitor.AssertCurrentThreadOwns();
    mFrontMessageQueue.SwapElements(mBackMessageQueue);
  }
  


  void Process(GraphTime aFrom, GraphTime aTo);
  



  void UpdateConsumptionState(SourceMediaStream* aStream);
  


  void ExtractPendingInput(SourceMediaStream* aStream,
                           GraphTime aDesiredUpToTime,
                           bool* aEnsureNextIteration);
  


  void UpdateBufferSufficiencyState(SourceMediaStream* aStream);
  


  static void MarkConsumed(MediaStream* aStream);
  




  void UpdateStreamOrder();
  





  void RecomputeBlocking(GraphTime aEndBlockingDecisions);
  
  




  void AddBlockingRelatedStreamsToSet(nsTArray<MediaStream*>* aStreams,
                                      MediaStream* aStream);
  




  void MarkStreamBlocking(MediaStream* aStream);
  





  void RecomputeBlockingAt(const nsTArray<MediaStream*>& aStreams,
                           GraphTime aTime, GraphTime aEndBlockingDecisions,
                           GraphTime* aEnd);
  



  GraphTime RoundUpToNextAudioBlock(GraphTime aTime);
  





  void ProduceDataForStreamsBlockByBlock(uint32_t aStreamIndex,
                                         TrackRate aSampleRate,
                                         GraphTime aFrom,
                                         GraphTime aTo);
  





  bool WillUnderrun(MediaStream* aStream, GraphTime aTime,
                    GraphTime aEndBlockingDecisions, GraphTime* aEnd);

  



  StreamTime GraphTimeToStreamTime(MediaStream* aStream, GraphTime aTime);
  




  StreamTime GraphTimeToStreamTimeOptimistic(MediaStream* aStream, GraphTime aTime);
  enum {
    INCLUDE_TRAILING_BLOCKED_INTERVAL = 0x01
  };
  








  GraphTime StreamTimeToGraphTime(MediaStream* aStream, StreamTime aTime,
                                  uint32_t aFlags = 0);
  


  GraphTime GetAudioPosition(MediaStream* aStream);
  


  void NotifyHasCurrentData(MediaStream* aStream);
  



  void CreateOrDestroyAudioStreams(GraphTime aAudioOutputStartTime, MediaStream* aStream);
  



  StreamTime PlayAudio(MediaStream* aStream, GraphTime aFrom, GraphTime aTo);
  


  void PlayVideo(MediaStream* aStream);
  




  void FinishStream(MediaStream* aStream);
  


  StreamTime GetDesiredBufferEnd(MediaStream* aStream);
  


  bool IsEmpty() { return mStreams.IsEmpty() && mPortCount == 0; }

  
  


  int64_t GetProcessingGraphUpdateIndex() { return mProcessingGraphUpdateIndex; }
  


  void AddStream(MediaStream* aStream);
  



  void RemoveStream(MediaStream* aStream);
  


  void DestroyPort(MediaInputPort* aPort);
  


  void SetStreamOrderDirty()
  {
    mStreamOrderDirty = true;
  }

  
  uint32_t AudioChannelCount() { return 2; }

  double MediaTimeToSeconds(GraphTime aTime)
  {
    NS_ASSERTION(0 <= aTime && aTime <= STREAM_TIME_MAX, "Bad time");
    return static_cast<double>(aTime)/GraphRate();
  }
  GraphTime SecondsToMediaTime(double aS)
  {
    NS_ASSERTION(0 <= aS && aS <= TRACK_TICKS_MAX/TRACK_RATE_MAX,
                 "Bad seconds");
    return GraphRate() * aS;
  }
  GraphTime MillisecondsToMediaTime(int32_t aMS)
  {
    return RateConvertTicksRoundDown(GraphRate(), 1000, aMS);
  }

  



  void PausedIndefinitly();
  void ResumedFromPaused();

  


  GraphDriver* CurrentDriver() {
    AssertOnGraphThreadOrNotRunning();
    return mDriver;
  }

  bool RemoveMixerCallback(MixerCallbackReceiver* aReceiver)
  {
    return mMixer.RemoveCallback(aReceiver);
  }

  







  void SetCurrentDriver(GraphDriver* aDriver) {
    AssertOnGraphThreadOrNotRunning();
    mDriver = aDriver;
  }

  Monitor& GetMonitor() {
    return mMonitor;
  }

  void EnsureNextIteration() {
    mNeedAnotherIteration = true; 
    if (mGraphDriverAsleep) { 
      MonitorAutoLock mon(mMonitor);
      CurrentDriver()->WakeUp(); 
    }
  }

  void EnsureNextIterationLocked() {
    mNeedAnotherIteration = true; 
    if (mGraphDriverAsleep) { 
      CurrentDriver()->WakeUp(); 
    }
  }

  
  
  





  nsRefPtr<GraphDriver> mDriver;

  
  
  

  




  nsTArray<MediaStream*> mStreams;
  




  uint32_t mFirstCycleBreaker;
  


  TimeStamp mLastMainThreadUpdate;
  


  int64_t mProcessingGraphUpdateIndex;
  


  int32_t mPortCount;

  
  Atomic<bool> mNeedAnotherIteration;
  
  Atomic<bool> mGraphDriverAsleep;

  
  
  
  
  
  Monitor mMonitor;

  
  

  


  nsTArray<StreamUpdate> mStreamUpdates;
  


  nsTArray<nsCOMPtr<nsIRunnable> > mUpdateRunnables;
  



  
  nsTArray<MessageBlock> mFrontMessageQueue;
  
  nsTArray<MessageBlock> mBackMessageQueue;

  
  bool MessagesQueued() {
    mMonitor.AssertCurrentThreadOwns();
    return !mBackMessageQueue.IsEmpty();
  }
  






















  enum LifecycleState {
    
    LIFECYCLE_THREAD_NOT_STARTED,
    
    LIFECYCLE_RUNNING,
    
    
    
    
    
    
    LIFECYCLE_WAITING_FOR_MAIN_THREAD_CLEANUP,
    
    
    LIFECYCLE_WAITING_FOR_THREAD_SHUTDOWN,
    
    
    
    
    LIFECYCLE_WAITING_FOR_STREAM_DESTRUCTION
  };
  LifecycleState mLifecycleState;
  


  GraphTime mEndTime;

  


  bool mForceShutDown;
  



  bool mPostedRunInStableStateEvent;

  



  bool mFlushSourcesNow;
  bool mFlushSourcesOnNextIteration;

  

  





  nsTArray<nsAutoPtr<ControlMessage> > mCurrentTaskMessageQueue;
  




  bool mDetectedNotRunning;
  



  bool mPostedRunInStableState;
  



  bool mRealtime;
  



  bool mNonRealtimeProcessing;
  



  bool mStreamOrderDirty;
  


  nsRefPtr<AsyncLatencyLogger> mLatencyLog;
  AudioMixer mMixer;
#ifdef MOZ_WEBRTC
  nsRefPtr<AudioOutputObserver> mFarendObserverRef;
#endif

  uint32_t AudioChannel() const { return mAudioChannel; }

private:
  virtual ~MediaStreamGraphImpl();

  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf)

  


  Monitor mMemoryReportMonitor;
  






  nsRefPtr<MediaStreamGraphImpl> mSelfRef;
  


  nsTArray<AudioNodeSizes> mAudioStreamSizes;
  


  bool mNeedsMemoryReport;

#ifdef DEBUG
  


  bool mCanRunMessagesSynchronously;
#endif

  
  
  uint32_t mAudioChannel;
};

}

#endif 
