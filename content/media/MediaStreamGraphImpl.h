




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
class AudioOutputObserver;





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
                                DOMMediaStream::TrackTypeHints aHint,
                                dom::AudioChannel aChannel = dom::AudioChannel::Normal);

  



  void Destroy();

  
  






  void RunInStableState(bool aSourceIsMSG);
  




  void EnsureRunInStableState();
  


  void ApplyStreamUpdate(StreamUpdate* aUpdate);
  



  void AppendMessage(ControlMessage* aMessage);
  





  void ForceShutDown();
  


  void ShutdownThreads();

  


  void Init();
  
  
  



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
  



  TrackTicks PlayAudio(MediaStream* aStream, GraphTime aFrom, GraphTime aTo);
  


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

  TrackRate AudioSampleRate() const { return mSampleRate; }
  TrackRate GraphRate() const { return mSampleRate; }
  
  uint32_t AudioChannelCount() { return 2; }

  double MediaTimeToSeconds(GraphTime aTime)
  {
    return TrackTicksToSeconds(GraphRate(), aTime);
  }
  GraphTime SecondsToMediaTime(double aS)
  {
    return SecondsToTicksRoundDown(GraphRate(), aS);
  }
  GraphTime MillisecondsToMediaTime(int32_t aMS)
  {
    return RateConvertTicksRoundDown(GraphRate(), 1000, aMS);
  }

  TrackTicks TimeToTicksRoundDown(TrackRate aRate, StreamTime aTime)
  {
    return RateConvertTicksRoundDown(aRate, GraphRate(), aTime);
  }

  



  void PausedIndefinitly();
  void ResumedFromPaused();

  GraphDriver* CurrentDriver() {
    return mDriver;
  }

  





  void SetCurrentDriver(GraphDriver* aDriver) {
    mDriver = aDriver;
  }

  Monitor& GetMonitor() {
    return mMonitor;
  }

  
  
  





  nsRefPtr<GraphDriver> mDriver;

  
  
  

  




  nsTArray<MediaStream*> mStreams;
  




  uint32_t mFirstCycleBreaker;
  


  TimeStamp mLastMainThreadUpdate;
  


  int64_t mProcessingGraphUpdateIndex;
  


  int32_t mPortCount;

  
  
  
  
  
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

  




  TrackRate mSampleRate;
  


  bool mNeedAnotherIteration;
  


  bool mForceShutDown;
  



  bool mPostedRunInStableStateEvent;

  

  





  nsTArray<nsAutoPtr<ControlMessage> > mCurrentTaskMessageQueue;
  




  bool mDetectedNotRunning;
  



  bool mPostedRunInStableState;
  



  bool mRealtime;
  



  bool mNonRealtimeProcessing;
  



  bool mStreamOrderDirty;
  


  nsRefPtr<AsyncLatencyLogger> mLatencyLog;
  AudioMixer mMixer;
  nsRefPtr<AudioOutputObserver> mFarendObserverRef;

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

};

}

#endif 
