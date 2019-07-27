




#ifndef MOZILLA_MEDIASTREAMGRAPH_H_
#define MOZILLA_MEDIASTREAMGRAPH_H_

#include "mozilla/Mutex.h"
#include "mozilla/LinkedList.h"
#include "AudioStream.h"
#include "nsTArray.h"
#include "nsIRunnable.h"
#include "StreamBuffer.h"
#include "TimeVarying.h"
#include "VideoFrameContainer.h"
#include "VideoSegment.h"
#include "MainThreadUtils.h"
#include "MediaTaskQueue.h"
#include "nsAutoRef.h"
#include "GraphDriver.h"
#include <speex/speex_resampler.h>
#include "mozilla/dom/AudioChannelBinding.h"
#include "DOMMediaStream.h"
#include "AudioContext.h"

class nsIRunnable;

template <>
class nsAutoRefTraits<SpeexResamplerState> : public nsPointerRefTraits<SpeexResamplerState>
{
  public:
  static void Release(SpeexResamplerState* aState) { speex_resampler_destroy(aState); }
};

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaStreamGraphLog;
#endif


































class MediaStreamGraph;





















class MediaStreamListener {
protected:
  
  virtual ~MediaStreamListener() {}

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaStreamListener)

  enum Consumption {
    CONSUMED,
    NOT_CONSUMED
  };

  




  virtual void NotifyConsumptionChanged(MediaStreamGraph* aGraph, Consumption aConsuming) {}

  











  virtual void NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime) {}

  enum Blocking {
    BLOCKED,
    UNBLOCKED
  };
  



  virtual void NotifyBlockingChanged(MediaStreamGraph* aGraph, Blocking aBlocked) {}

  





  virtual void NotifyHasCurrentData(MediaStreamGraph* aGraph) {}

  




  virtual void NotifyOutput(MediaStreamGraph* aGraph, GraphTime aCurrentTime) {}

  enum MediaStreamGraphEvent {
    EVENT_FINISHED,
    EVENT_REMOVED,
    EVENT_HAS_DIRECT_LISTENERS, 
    EVENT_HAS_NO_DIRECT_LISTENERS,  
  };

  


  virtual void NotifyEvent(MediaStreamGraph* aGraph, MediaStreamGraphEvent aEvent) {}

  enum {
    TRACK_EVENT_CREATED = 0x01,
    TRACK_EVENT_ENDED = 0x02
  };
  





  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        StreamTime aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) {}

  




  virtual void NotifyFinishedTrackCreation(MediaStreamGraph* aGraph) {}
};







class MediaStreamDirectListener : public MediaStreamListener
{
public:
  virtual ~MediaStreamDirectListener() {}

  





  virtual void NotifyRealtimeData(MediaStreamGraph* aGraph, TrackID aID,
                                  StreamTime aTrackOffset,
                                  uint32_t aTrackEvents,
                                  const MediaSegment& aMedia) {}
};














class MainThreadMediaStreamListener {
public:
  virtual void NotifyMainThreadStateChanged() = 0;
};




struct AudioNodeSizes
{
  AudioNodeSizes() : mDomNode(0), mStream(0), mEngine(0), mNodeType() {}
  size_t mDomNode;
  size_t mStream;
  size_t mEngine;
  nsCString mNodeType;
};

class MediaStreamGraphImpl;
class SourceMediaStream;
class ProcessedMediaStream;
class MediaInputPort;
class AudioNodeEngine;
class AudioNodeExternalInputStream;
class AudioNodeStream;
class CameraPreviewMediaStream;
































































class MediaStream : public mozilla::LinkedListElement<MediaStream> {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaStream)

  explicit MediaStream(DOMMediaStream* aWrapper);
  virtual dom::AudioContext::AudioContextId AudioContextId() const { return 0; }

protected:
  
  virtual ~MediaStream()
  {
    MOZ_COUNT_DTOR(MediaStream);
    NS_ASSERTION(mMainThreadDestroyed, "Should have been destroyed already");
    NS_ASSERTION(mMainThreadListeners.IsEmpty(),
                 "All main thread listeners should have been removed");
  }

public:
  


  MediaStreamGraphImpl* GraphImpl();
  MediaStreamGraph* Graph();
  


  void SetGraphImpl(MediaStreamGraphImpl* aGraph);
  void SetGraphImpl(MediaStreamGraph* aGraph);

  


  TrackRate GraphRate() { return mBuffer.GraphRate(); }

  
  
  
  
  
  
  
  virtual void AddAudioOutput(void* aKey);
  virtual void SetAudioOutputVolume(void* aKey, float aVolume);
  virtual void RemoveAudioOutput(void* aKey);
  
  
  
  virtual void AddVideoOutput(VideoFrameContainer* aContainer);
  virtual void RemoveVideoOutput(VideoFrameContainer* aContainer);
  
  
  virtual void ChangeExplicitBlockerCount(int32_t aDelta);
  void BlockStreamIfNeeded();
  void UnblockStreamIfNeeded();
  
  virtual void AddListener(MediaStreamListener* aListener);
  virtual void RemoveListener(MediaStreamListener* aListener);
  
  
  void SetTrackEnabled(TrackID aTrackID, bool aEnabled);
  
  
  void AddMainThreadListener(MainThreadMediaStreamListener* aListener)
  {
    NS_ASSERTION(NS_IsMainThread(), "Call only on main thread");
    mMainThreadListeners.AppendElement(aListener);
  }
  
  
  void RemoveMainThreadListener(MainThreadMediaStreamListener* aListener)
  {
    NS_ASSERTION(NS_IsMainThread(), "Call only on main thread");
    mMainThreadListeners.RemoveElement(aListener);
  }
  












  void RunAfterPendingUpdates(already_AddRefed<nsIRunnable> aRunnable);

  
  virtual void Destroy();
  
  
  StreamTime GetCurrentTime()
  {
    NS_ASSERTION(NS_IsMainThread(), "Call only on main thread");
    return mMainThreadCurrentTime;
  }
  
  bool IsFinished()
  {
    NS_ASSERTION(NS_IsMainThread(), "Call only on main thread");
    return mMainThreadFinished;
  }
  bool IsDestroyed()
  {
    NS_ASSERTION(NS_IsMainThread(), "Call only on main thread");
    return mMainThreadDestroyed;
  }

  friend class MediaStreamGraphImpl;
  friend class MediaInputPort;
  friend class AudioNodeExternalInputStream;

  virtual SourceMediaStream* AsSourceStream() { return nullptr; }
  virtual ProcessedMediaStream* AsProcessedStream() { return nullptr; }
  virtual AudioNodeStream* AsAudioNodeStream() { return nullptr; }
  virtual CameraPreviewMediaStream* AsCameraPreviewStream() { return nullptr; }

  
  void Init();
  
  
  



  virtual void DestroyImpl();
  StreamTime GetBufferEnd() { return mBuffer.GetEnd(); }
#ifdef DEBUG
  void DumpTrackInfo() { return mBuffer.DumpTrackInfo(); }
#endif
  void SetAudioOutputVolumeImpl(void* aKey, float aVolume);
  void AddAudioOutputImpl(void* aKey)
  {
    mAudioOutputs.AppendElement(AudioOutput(aKey));
  }
  
  bool HasAudioOutput()
  {
    return !mAudioOutputs.IsEmpty();
  }
  void RemoveAudioOutputImpl(void* aKey);
  void AddVideoOutputImpl(already_AddRefed<VideoFrameContainer> aContainer)
  {
    *mVideoOutputs.AppendElement() = aContainer;
  }
  void RemoveVideoOutputImpl(VideoFrameContainer* aContainer)
  {
    mVideoOutputs.RemoveElement(aContainer);
  }
  void ChangeExplicitBlockerCountImpl(GraphTime aTime, int32_t aDelta)
  {
    mExplicitBlockerCount.SetAtAndAfter(aTime, mExplicitBlockerCount.GetAt(aTime) + aDelta);
  }
  void BlockStreamIfNeededImpl(GraphTime aTime)
  {
    bool blocked = mExplicitBlockerCount.GetAt(aTime) > 0;
    if (blocked) {
      return;
    }
    ChangeExplicitBlockerCountImpl(aTime, 1);
  }
  void UnblockStreamIfNeededImpl(GraphTime aTime)
  {
    bool blocked = mExplicitBlockerCount.GetAt(aTime) > 0;
    if (!blocked) {
      return;
    }
    ChangeExplicitBlockerCountImpl(aTime, -1);
  }
  void AddListenerImpl(already_AddRefed<MediaStreamListener> aListener);
  void RemoveListenerImpl(MediaStreamListener* aListener);
  void RemoveAllListenersImpl();
  virtual void SetTrackEnabledImpl(TrackID aTrackID, bool aEnabled);
  





  virtual bool IsIntrinsicallyConsumed() const
  {
    return !mAudioOutputs.IsEmpty() || !mVideoOutputs.IsEmpty();
  }

  void AddConsumer(MediaInputPort* aPort)
  {
    mConsumers.AppendElement(aPort);
  }
  void RemoveConsumer(MediaInputPort* aPort)
  {
    mConsumers.RemoveElement(aPort);
  }
  uint32_t ConsumerCount()
  {
    return mConsumers.Length();
  }
  const StreamBuffer& GetStreamBuffer() { return mBuffer; }
  GraphTime GetStreamBufferStartTime() { return mBufferStartTime; }

  double StreamTimeToSeconds(StreamTime aTime)
  {
    NS_ASSERTION(0 <= aTime && aTime <= STREAM_TIME_MAX, "Bad time");
    return static_cast<double>(aTime)/mBuffer.GraphRate();
  }
  int64_t StreamTimeToMicroseconds(StreamTime aTime)
  {
    NS_ASSERTION(0 <= aTime && aTime <= STREAM_TIME_MAX, "Bad time");
    return (aTime*1000000)/mBuffer.GraphRate();
  }
  StreamTime MicrosecondsToStreamTimeRoundDown(int64_t aMicroseconds) {
    return (aMicroseconds*mBuffer.GraphRate())/1000000;
  }

  TrackTicks TimeToTicksRoundUp(TrackRate aRate, StreamTime aTime)
  {
    return RateConvertTicksRoundUp(aRate, mBuffer.GraphRate(), aTime);
  }
  StreamTime TicksToTimeRoundDown(TrackRate aRate, TrackTicks aTicks)
  {
    return RateConvertTicksRoundDown(mBuffer.GraphRate(), aRate, aTicks);
  }
  




  StreamTime GraphTimeToStreamTime(GraphTime aTime);
  




  StreamTime GraphTimeToStreamTimeOptimistic(GraphTime aTime);
  




  GraphTime StreamTimeToGraphTime(StreamTime aTime);
  bool IsFinishedOnGraphThread() { return mFinished; }
  void FinishOnGraphThread();
  


  int64_t GetProcessingGraphUpdateIndex();

  bool HasCurrentData() { return mHasCurrentData; }

  StreamBuffer::Track* EnsureTrack(TrackID aTrack);

  virtual void ApplyTrackDisabling(TrackID aTrackID, MediaSegment* aSegment, MediaSegment* aRawSegment = nullptr);

  DOMMediaStream* GetWrapper()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only use DOMMediaStream on main thread");
    return mWrapper;
  }

  
  virtual bool MainThreadNeedsUpdates() const
  {
    return true;
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;
  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

  void SetAudioChannelType(dom::AudioChannel aType) { mAudioChannelType = aType; }
  dom::AudioChannel AudioChannelType() const { return mAudioChannelType; }

protected:
  virtual void AdvanceTimeVaryingValuesToCurrentTime(GraphTime aCurrentTime, GraphTime aBlockedTime)
  {
    mBufferStartTime += aBlockedTime;
    mGraphUpdateIndices.InsertTimeAtStart(aBlockedTime);
    mGraphUpdateIndices.AdvanceCurrentTime(aCurrentTime);
    mExplicitBlockerCount.AdvanceCurrentTime(aCurrentTime);

    mBuffer.ForgetUpTo(aCurrentTime - mBufferStartTime);
  }

  
  

  
  
  
  StreamBuffer mBuffer;
  
  
  
  GraphTime mBufferStartTime;

  
  struct AudioOutput {
    explicit AudioOutput(void* aKey) : mKey(aKey), mVolume(1.0f) {}
    void* mKey;
    float mVolume;
  };
  nsTArray<AudioOutput> mAudioOutputs;
  nsTArray<nsRefPtr<VideoFrameContainer> > mVideoOutputs;
  
  
  VideoFrame mLastPlayedVideoFrame;
  
  
  TimeVarying<GraphTime,uint32_t,0> mExplicitBlockerCount;
  nsTArray<nsRefPtr<MediaStreamListener> > mListeners;
  nsTArray<MainThreadMediaStreamListener*> mMainThreadListeners;
  nsTArray<TrackID> mDisabledTrackIDs;

  
  
  
  
  
  
  TimeVarying<GraphTime,bool,5> mBlocked;
  
  TimeVarying<GraphTime,int64_t,0> mGraphUpdateIndices;

  
  nsTArray<MediaInputPort*> mConsumers;

  
  
  struct AudioOutputStream {
    
    
    GraphTime mAudioPlaybackStartTime;
    
    
    MediaTime mBlockedAudioTime;
    
    StreamTime mLastTickWritten;
    TrackID mTrackID;
  };
  nsTArray<AudioOutputStream> mAudioOutputStreams;

  



  bool mFinished;
  



  bool mNotifiedFinished;
  



  bool mNotifiedBlocked;
  





  bool mHasCurrentData;
  


  bool mNotifiedHasCurrentData;

  
  
  bool mIsConsumed;
  
  
  
  bool mInBlockingSet;
  
  bool mBlockInThisPhase;

  
  DOMMediaStream* mWrapper;
  
  StreamTime mMainThreadCurrentTime;
  bool mMainThreadFinished;
  bool mMainThreadDestroyed;

  
  MediaStreamGraphImpl* mGraph;

  dom::AudioChannel mAudioChannelType;
};







class SourceMediaStream : public MediaStream {
public:
  explicit SourceMediaStream(DOMMediaStream* aWrapper) :
    MediaStream(aWrapper),
    mLastConsumptionState(MediaStreamListener::NOT_CONSUMED),
    mMutex("mozilla::media::SourceMediaStream"),
    mUpdateKnownTracksTime(0),
    mPullEnabled(false),
    mUpdateFinished(false),
    mNeedsMixing(false)
  {}

  virtual SourceMediaStream* AsSourceStream() override { return this; }

  
  virtual void DestroyImpl() override;

  
  






  void SetPullEnabled(bool aEnabled);

  




  void NotifyListenersEventImpl(MediaStreamListener::MediaStreamGraphEvent aEvent);
  void NotifyListenersEvent(MediaStreamListener::MediaStreamGraphEvent aEvent);
  void AddDirectListener(MediaStreamDirectListener* aListener);
  void RemoveDirectListener(MediaStreamDirectListener* aListener);

  enum {
    ADDTRACK_QUEUED    = 0x01 
  };
  





  void AddTrack(TrackID aID, StreamTime aStart, MediaSegment* aSegment,
                uint32_t aFlags = 0)
  {
    AddTrackInternal(aID, GraphRate(), aStart, aSegment, aFlags);
  }

  


  void AddAudioTrack(TrackID aID, TrackRate aRate, StreamTime aStart,
                     AudioSegment* aSegment, uint32_t aFlags = 0)
  {
    AddTrackInternal(aID, aRate, aStart, aSegment, aFlags);
  }

  



  void FinishAddTracks();

  





  bool AppendToTrack(TrackID aID, MediaSegment* aSegment, MediaSegment *aRawSegment = nullptr);
  



  bool HaveEnoughBuffered(TrackID aID);
  





  StreamTime GetEndOfAppendedData(TrackID aID);
  





  void DispatchWhenNotEnoughBuffered(TrackID aID,
      MediaTaskQueue* aSignalQueue, nsIRunnable* aSignalRunnable);
  




  void EndTrack(TrackID aID);
  



  void AdvanceKnownTracksTime(StreamTime aKnownTime);
  




  void FinishWithLockHeld();
  void Finish()
  {
    MutexAutoLock lock(mMutex);
    FinishWithLockHeld();
  }

  
  virtual void
  SetTrackEnabledImpl(TrackID aTrackID, bool aEnabled) override {
    MutexAutoLock lock(mMutex);
    MediaStream::SetTrackEnabledImpl(aTrackID, aEnabled);
  }

  
  virtual void
  ApplyTrackDisabling(TrackID aTrackID, MediaSegment* aSegment,
                      MediaSegment* aRawSegment = nullptr) override {
    mMutex.AssertCurrentThreadOwns();
    MediaStream::ApplyTrackDisabling(aTrackID, aSegment, aRawSegment);
  }

  



  void EndAllTrackAndFinish();

  








  StreamTime GetBufferedTicks(TrackID aID);

  void RegisterForAudioMixing();

  

  friend class MediaStreamGraphImpl;

protected:
  struct ThreadAndRunnable {
    void Init(MediaTaskQueue* aTarget, nsIRunnable* aRunnable)
    {
      mTarget = aTarget;
      mRunnable = aRunnable;
    }

    nsRefPtr<MediaTaskQueue> mTarget;
    nsCOMPtr<nsIRunnable> mRunnable;
  };
  enum TrackCommands {
    TRACK_CREATE = MediaStreamListener::TRACK_EVENT_CREATED,
    TRACK_END = MediaStreamListener::TRACK_EVENT_ENDED
  };
  


  struct TrackData {
    TrackID mID;
    
    TrackRate mInputRate;
    
    
    nsAutoRef<SpeexResamplerState> mResampler;
#ifdef DEBUG
    int mResamplerChannelCount;
#endif
    StreamTime mStart;
    
    StreamTime mEndOfFlushedData;
    
    
    nsAutoPtr<MediaSegment> mData;
    nsTArray<ThreadAndRunnable> mDispatchWhenNotEnough;
    
    
    uint32_t mCommands;
    bool mHaveEnough;
  };

  bool NeedsMixing();

  void ResampleAudioToGraphSampleRate(TrackData* aTrackData, MediaSegment* aSegment);

  void AddTrackInternal(TrackID aID, TrackRate aRate,
                        StreamTime aStart, MediaSegment* aSegment,
                        uint32_t aFlags);

  TrackData* FindDataForTrack(TrackID aID)
  {
    mMutex.AssertCurrentThreadOwns();
    for (uint32_t i = 0; i < mUpdateTracks.Length(); ++i) {
      if (mUpdateTracks[i].mID == aID) {
        return &mUpdateTracks[i];
      }
    }
    return nullptr;
  }

  





  void NotifyDirectConsumers(TrackData *aTrack,
                             MediaSegment *aSegment);

  
  MediaStreamListener::Consumption mLastConsumptionState;

  
  
  Mutex mMutex;
  
  StreamTime mUpdateKnownTracksTime;
  nsTArray<TrackData> mUpdateTracks;
  nsTArray<TrackData> mPendingTracks;
  nsTArray<nsRefPtr<MediaStreamDirectListener> > mDirectListeners;
  bool mPullEnabled;
  bool mUpdateFinished;
  bool mNeedsMixing;
};


















class MediaInputPort final {
private:
  
  MediaInputPort(MediaStream* aSource, ProcessedMediaStream* aDest,
                 uint32_t aFlags, uint16_t aInputNumber,
                 uint16_t aOutputNumber)
    : mSource(aSource)
    , mDest(aDest)
    , mFlags(aFlags)
    , mInputNumber(aInputNumber)
    , mOutputNumber(aOutputNumber)
    , mGraph(nullptr)
  {
    MOZ_COUNT_CTOR(MediaInputPort);
  }

  
  ~MediaInputPort()
  {
    MOZ_COUNT_DTOR(MediaInputPort);
  }

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaInputPort)

  




  enum {
    
    
    FLAG_BLOCK_INPUT = 0x01,
    
    
    FLAG_BLOCK_OUTPUT = 0x02
  };

  
  
  void Init();
  
  void Disconnect();

  
  



  void Destroy();

  
  MediaStream* GetSource() { return mSource; }
  ProcessedMediaStream* GetDestination() { return mDest; }

  uint16_t InputNumber() const { return mInputNumber; }
  uint16_t OutputNumber() const { return mOutputNumber; }

  
  struct InputInterval {
    GraphTime mStart;
    GraphTime mEnd;
    bool mInputIsBlocked;
  };
  
  
  InputInterval GetNextInputInterval(GraphTime aTime);

  


  MediaStreamGraphImpl* GraphImpl();
  MediaStreamGraph* Graph();
  


  void SetGraphImpl(MediaStreamGraphImpl* aGraph);

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    size_t amount = 0;

    
    
    
    
    return amount;
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  friend class MediaStreamGraphImpl;
  friend class MediaStream;
  friend class ProcessedMediaStream;
  
  MediaStream* mSource;
  ProcessedMediaStream* mDest;
  uint32_t mFlags;
  
  
  const uint16_t mInputNumber;
  const uint16_t mOutputNumber;

  
  MediaStreamGraphImpl* mGraph;
};






class ProcessedMediaStream : public MediaStream {
public:
  explicit ProcessedMediaStream(DOMMediaStream* aWrapper)
    : MediaStream(aWrapper), mAutofinish(false)
  {}

  
  



  already_AddRefed<MediaInputPort> AllocateInputPort(MediaStream* aStream,
                                                     uint32_t aFlags = 0,
                                                     uint16_t aInputNumber = 0,
                                                     uint16_t aOutputNumber = 0);
  


  void Finish();
  




  void SetAutofinish(bool aAutofinish);

  virtual ProcessedMediaStream* AsProcessedStream() override { return this; }

  friend class MediaStreamGraphImpl;

  
  virtual void AddInput(MediaInputPort* aPort);
  virtual void RemoveInput(MediaInputPort* aPort)
  {
    mInputs.RemoveElement(aPort);
  }
  bool HasInputPort(MediaInputPort* aPort)
  {
    return mInputs.Contains(aPort);
  }
  uint32_t InputPortCount()
  {
    return mInputs.Length();
  }
  virtual void DestroyImpl() override;
  















  enum {
    ALLOW_FINISH = 0x01
  };
  virtual void ProcessInput(GraphTime aFrom, GraphTime aTo, uint32_t aFlags) = 0;
  void SetAutofinishImpl(bool aAutofinish) { mAutofinish = aAutofinish; }

  


  virtual void ForwardTrackEnabled(TrackID aOutputID, bool aEnabled) {};

  
  
  
  bool InMutedCycle() const { return mCycleMarker; }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    size_t amount = MediaStream::SizeOfExcludingThis(aMallocSizeOf);
    
    
    amount += mInputs.SizeOfExcludingThis(aMallocSizeOf);
    return amount;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  

  
  nsTArray<MediaInputPort*> mInputs;
  bool mAutofinish;
  
  
  
  uint32_t mCycleMarker;
};






class MediaStreamGraph {
public:
  
  
  
  
  

  
  static MediaStreamGraph* GetInstance(bool aStartWithAudioDriver = false,
                                       dom::AudioChannel aChannel = dom::AudioChannel::Normal);
  static MediaStreamGraph* CreateNonRealtimeInstance(TrackRate aSampleRate);
  
  static void DestroyNonRealtimeInstance(MediaStreamGraph* aGraph);

  
  



  SourceMediaStream* CreateSourceStream(DOMMediaStream* aWrapper);
  













  ProcessedMediaStream* CreateTrackUnionStream(DOMMediaStream* aWrapper);
  
  
  
  enum AudioNodeStreamKind { SOURCE_STREAM, INTERNAL_STREAM, EXTERNAL_STREAM };
  





  AudioNodeStream* CreateAudioNodeStream(AudioNodeEngine* aEngine,
                                         AudioNodeStreamKind aKind,
                                         TrackRate aSampleRate = 0);

  AudioNodeExternalInputStream*
  CreateAudioNodeExternalInputStream(AudioNodeEngine* aEngine,
                                     TrackRate aSampleRate = 0);

  

  void NotifyWhenGraphStarted(AudioNodeStream* aNodeStream);
  







  void ApplyAudioContextOperation(AudioNodeStream* aNodeStream,
                                  dom::AudioContextOperation aState,
                                  void * aPromise);

  bool IsNonRealtime() const;
  


  void StartNonRealtimeProcessing(uint32_t aTicksToProcess);

  






  virtual void DispatchToMainThreadAfterStreamStateUpdate(already_AddRefed<nsIRunnable> aRunnable)
  {
    *mPendingUpdateRunnables.AppendElement() = aRunnable;
  }

  


  TrackRate GraphRate() const { return mSampleRate; }

protected:
  explicit MediaStreamGraph(TrackRate aSampleRate)
    : mNextGraphUpdateIndex(1)
    , mSampleRate(aSampleRate)
  {
    MOZ_COUNT_CTOR(MediaStreamGraph);
  }
  virtual ~MediaStreamGraph()
  {
    MOZ_COUNT_DTOR(MediaStreamGraph);
  }

  
  nsTArray<nsCOMPtr<nsIRunnable> > mPendingUpdateRunnables;

  
  
  
  int64_t mNextGraphUpdateIndex;

  




  TrackRate mSampleRate;
};

}

#endif 
