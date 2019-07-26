




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
#include "nsThreadUtils.h"

namespace mozilla {

class DOMMediaStream;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaStreamGraphLog;
#endif




typedef int64_t GraphTime;
const GraphTime GRAPH_TIME_MAX = MEDIA_TIME_MAX;


































class MediaStreamGraph;





















class MediaStreamListener {
public:
  virtual ~MediaStreamListener() {}

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

  


  virtual void NotifyOutput(MediaStreamGraph* aGraph) {}

  


  virtual void NotifyFinished(MediaStreamGraph* aGraph) {}

  



  virtual void NotifyRemoved(MediaStreamGraph* aGraph) {}

  enum {
    TRACK_EVENT_CREATED = 0x01,
    TRACK_EVENT_ENDED = 0x02
  };
  





  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        TrackRate aTrackRate,
                                        TrackTicks aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) {}
};







class MediaStreamDirectListener : public MediaStreamListener
{
public:
  virtual ~MediaStreamDirectListener() {}

  





  virtual void NotifyRealtimeData(MediaStreamGraph* aGraph, TrackID aID,
                                  TrackRate aTrackRate,
                                  TrackTicks aTrackOffset,
                                  uint32_t aTrackEvents,
                                  const MediaSegment& aMedia) {}
};














class MainThreadMediaStreamListener {
public:
  virtual void NotifyMainThreadStateChanged() = 0;
};

class MediaStreamGraphImpl;
class SourceMediaStream;
class ProcessedMediaStream;
class MediaInputPort;
class AudioNodeEngine;
class AudioNodeExternalInputStream;
class AudioNodeStream;
struct AudioChunk;
































































class MediaStream : public mozilla::LinkedListElement<MediaStream> {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaStream)

  MediaStream(DOMMediaStream* aWrapper);
  virtual ~MediaStream()
  {
    MOZ_COUNT_DTOR(MediaStream);
    NS_ASSERTION(mMainThreadDestroyed, "Should have been destroyed already");
    NS_ASSERTION(mMainThreadListeners.IsEmpty(),
                 "All main thread listeners should have been removed");
  }

  


  MediaStreamGraphImpl* GraphImpl();
  MediaStreamGraph* Graph();
  


  void SetGraphImpl(MediaStreamGraphImpl* aGraph);
  void SetGraphImpl(MediaStreamGraph* aGraph);

  
  
  
  
  
  
  
  virtual void AddAudioOutput(void* aKey);
  virtual void SetAudioOutputVolume(void* aKey, float aVolume);
  virtual void RemoveAudioOutput(void* aKey);
  
  
  
  virtual void AddVideoOutput(VideoFrameContainer* aContainer);
  virtual void RemoveVideoOutput(VideoFrameContainer* aContainer);
  
  
  virtual void ChangeExplicitBlockerCount(int32_t aDelta);
  
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
  void AddListenerImpl(already_AddRefed<MediaStreamListener> aListener);
  void RemoveListenerImpl(MediaStreamListener* aListener);
  void RemoveAllListenersImpl();
  void SetTrackEnabledImpl(TrackID aTrackID, bool aEnabled);
  





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
  const StreamBuffer& GetStreamBuffer() { return mBuffer; }
  GraphTime GetStreamBufferStartTime() { return mBufferStartTime; }
  




  StreamTime GraphTimeToStreamTime(GraphTime aTime);
  




  StreamTime GraphTimeToStreamTimeOptimistic(GraphTime aTime);
  




  GraphTime StreamTimeToGraphTime(StreamTime aTime);
  bool IsFinishedOnGraphThread() { return mFinished; }
  void FinishOnGraphThread();
  


  int64_t GetProcessingGraphUpdateIndex();

  bool HasCurrentData() { return mHasCurrentData; }

  StreamBuffer::Track* EnsureTrack(TrackID aTrack, TrackRate aSampleRate);

  void ApplyTrackDisabling(TrackID aTrackID, MediaSegment* aSegment, MediaSegment* aRawSegment = nullptr);

  DOMMediaStream* GetWrapper()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only use DOMMediaStream on main thread");
    return mWrapper;
  }

  
  virtual bool MainThreadNeedsUpdates() const
  {
    return true;
  }

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
    AudioOutput(void* aKey) : mKey(aKey), mVolume(1.0f) {}
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
    nsAutoPtr<AudioStream> mStream;
    TrackID mTrackID;
  };
  nsTArray<AudioOutputStream> mAudioOutputStreams;

  



  bool mFinished;
  



  bool mNotifiedFinished;
  



  bool mNotifiedBlocked;
  





  bool mHasCurrentData;
  


  bool mNotifiedHasCurrentData;

  
  bool mHasBeenOrdered;
  bool mIsOnOrderingStack;
  
  
  bool mIsConsumed;
  
  
  
  bool mInBlockingSet;
  
  bool mBlockInThisPhase;

  
  DOMMediaStream* mWrapper;
  
  StreamTime mMainThreadCurrentTime;
  bool mMainThreadFinished;
  bool mMainThreadDestroyed;

  
  MediaStreamGraphImpl* mGraph;
};







class SourceMediaStream : public MediaStream {
public:
  SourceMediaStream(DOMMediaStream* aWrapper) :
    MediaStream(aWrapper),
    mLastConsumptionState(MediaStreamListener::NOT_CONSUMED),
    mMutex("mozilla::media::SourceMediaStream"),
    mUpdateKnownTracksTime(0),
    mPullEnabled(false),
    mUpdateFinished(false), mDestroyed(false)
  {}

  virtual SourceMediaStream* AsSourceStream() { return this; }

  
  virtual void DestroyImpl();

  
  






  void SetPullEnabled(bool aEnabled);

  void AddDirectListener(MediaStreamDirectListener* aListener);
  void RemoveDirectListener(MediaStreamDirectListener* aListener);

  





  void AddTrack(TrackID aID, TrackRate aRate, TrackTicks aStart,
                MediaSegment* aSegment);
  





  bool AppendToTrack(TrackID aID, MediaSegment* aSegment, MediaSegment *aRawSegment = nullptr);
  



  bool HaveEnoughBuffered(TrackID aID);
  





  void DispatchWhenNotEnoughBuffered(TrackID aID,
      nsIThread* aSignalThread, nsIRunnable* aSignalRunnable);
  




  void EndTrack(TrackID aID);
  



  void AdvanceKnownTracksTime(StreamTime aKnownTime);
  





  void FinishWithLockHeld();
  void Finish()
    {
      MutexAutoLock lock(mMutex);
      FinishWithLockHeld();
    }

  
  void SetTrackEnabledImpl(TrackID aTrackID, bool aEnabled) {
    MutexAutoLock lock(mMutex);
    MediaStream::SetTrackEnabledImpl(aTrackID, aEnabled);
  }

  



  void EndAllTrackAndFinish();

  








  TrackTicks GetBufferedTicks(TrackID aID);

  

  friend class MediaStreamGraphImpl;

  struct ThreadAndRunnable {
    void Init(nsIThread* aThread, nsIRunnable* aRunnable)
    {
      mThread = aThread;
      mRunnable = aRunnable;
    }

    nsCOMPtr<nsIThread> mThread;
    nsCOMPtr<nsIRunnable> mRunnable;
  };
  enum TrackCommands {
    TRACK_CREATE = MediaStreamListener::TRACK_EVENT_CREATED,
    TRACK_END = MediaStreamListener::TRACK_EVENT_ENDED
  };
  


  struct TrackData {
    TrackID mID;
    TrackRate mRate;
    TrackTicks mStart;
    
    
    uint32_t mCommands;
    
    
    nsAutoPtr<MediaSegment> mData;
    nsTArray<ThreadAndRunnable> mDispatchWhenNotEnough;
    bool mHaveEnough;
  };

protected:
  TrackData* FindDataForTrack(TrackID aID)
  {
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
  nsTArray<nsRefPtr<MediaStreamDirectListener> > mDirectListeners;
  bool mPullEnabled;
  bool mUpdateFinished;
  bool mDestroyed;
};


















class MediaInputPort {
  
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

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaInputPort)

  




  enum {
    
    
    FLAG_BLOCK_INPUT = 0x01,
    
    
    FLAG_BLOCK_OUTPUT = 0x02
  };
  ~MediaInputPort()
  {
    MOZ_COUNT_DTOR(MediaInputPort);
  }

  
  
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

protected:
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
  ProcessedMediaStream(DOMMediaStream* aWrapper)
    : MediaStream(aWrapper), mAutofinish(false), mInCycle(false)
  {}

  
  



  already_AddRefed<MediaInputPort> AllocateInputPort(MediaStream* aStream,
                                                     uint32_t aFlags = 0,
                                                     uint16_t aInputNumber = 0,
                                                     uint16_t aOutputNumber = 0);
  


  void Finish();
  




  void SetAutofinish(bool aAutofinish);

  virtual ProcessedMediaStream* AsProcessedStream() { return this; }

  friend class MediaStreamGraphImpl;

  
  virtual void AddInput(MediaInputPort* aPort)
  {
    mInputs.AppendElement(aPort);
  }
  virtual void RemoveInput(MediaInputPort* aPort)
  {
    mInputs.RemoveElement(aPort);
  }
  bool HasInputPort(MediaInputPort* aPort)
  {
    return mInputs.Contains(aPort);
  }
  virtual void DestroyImpl();
  







  virtual void ProduceOutput(GraphTime aFrom, GraphTime aTo) = 0;
  void SetAutofinishImpl(bool aAutofinish) { mAutofinish = aAutofinish; }

  


  virtual void ForwardTrackEnabled(TrackID aOutputID, bool aEnabled) {};

protected:
  

  
  nsTArray<MediaInputPort*> mInputs;
  bool mAutofinish;
  
  
  bool mInCycle;
};


inline TrackRate IdealAudioRate() { return 48000; }






class MediaStreamGraph {
public:
  
  
  
  

  
  static MediaStreamGraph* GetInstance();
  static MediaStreamGraph* CreateNonRealtimeInstance();
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

  




  int64_t GetCurrentGraphUpdateIndex() { return mGraphUpdatesSent; }
  


  void StartNonRealtimeProcessing(uint32_t aTicksToProcess);

  





  void DispatchToMainThreadAfterStreamStateUpdate(already_AddRefed<nsIRunnable> aRunnable)
  {
    *mPendingUpdateRunnables.AppendElement() = aRunnable;
  }

protected:
  MediaStreamGraph()
    : mGraphUpdatesSent(1)
  {
    MOZ_COUNT_CTOR(MediaStreamGraph);
  }
  ~MediaStreamGraph()
  {
    MOZ_COUNT_DTOR(MediaStreamGraph);
  }

  
  nsTArray<nsCOMPtr<nsIRunnable> > mPendingUpdateRunnables;

  
  
  
  int64_t mGraphUpdatesSent;
};

}

#endif 
