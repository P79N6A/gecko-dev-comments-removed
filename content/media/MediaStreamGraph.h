




#ifndef MOZILLA_MEDIASTREAMGRAPH_H_
#define MOZILLA_MEDIASTREAMGRAPH_H_

#include "mozilla/Mutex.h"
#include "nsAudioStream.h"
#include "nsTArray.h"
#include "nsIRunnable.h"
#include "nsISupportsImpl.h"
#include "StreamBuffer.h"
#include "TimeVarying.h"
#include "VideoFrameContainer.h"
#include "VideoSegment.h"

class nsDOMMediaStream;

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaStreamGraphLog;
#endif




typedef PRInt64 GraphTime;
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

  


  virtual void NotifyOutput(MediaStreamGraph* aGraph) {}

  


  virtual void NotifyFinished(MediaStreamGraph* aGraph) {}

  enum {
    TRACK_EVENT_CREATED = 0x01,
    TRACK_EVENT_ENDED = 0x02
  };
  






  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        TrackRate aTrackRate,
                                        TrackTicks aTrackOffset,
                                        PRUint32 aTrackEvents,
                                        const MediaSegment& aQueuedMedia) {}
};

class MediaStreamGraphImpl;
class SourceMediaStream;
class ProcessedMediaStream;
class MediaInputPort;
































































class MediaStream {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaStream)

  MediaStream(nsDOMMediaStream* aWrapper)
    : mBufferStartTime(0)
    , mExplicitBlockerCount(0)
    , mBlocked(false)
    , mGraphUpdateIndices(0)
    , mFinished(false)
    , mNotifiedFinished(false)
    , mAudioPlaybackStartTime(0)
    , mBlockedAudioTime(0)
    , mWrapper(aWrapper)
    , mMainThreadCurrentTime(0)
    , mMainThreadFinished(false)
    , mMainThreadDestroyed(false)
  {
    for (PRUint32 i = 0; i < ArrayLength(mFirstActiveTracks); ++i) {
      mFirstActiveTracks[i] = TRACK_NONE;
    }
  }
  virtual ~MediaStream() {}

  


  MediaStreamGraphImpl* GraphImpl();
  MediaStreamGraph* Graph();

  
  
  
  
  
  
  
  void AddAudioOutput(void* aKey);
  void SetAudioOutputVolume(void* aKey, float aVolume);
  void RemoveAudioOutput(void* aKey);
  
  
  
  void AddVideoOutput(VideoFrameContainer* aContainer);
  void RemoveVideoOutput(VideoFrameContainer* aContainer);
  
  
  void ChangeExplicitBlockerCount(PRInt32 aDelta);
  
  void AddListener(MediaStreamListener* aListener);
  void RemoveListener(MediaStreamListener* aListener);
  
  void Destroy();
  
  
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

  virtual SourceMediaStream* AsSourceStream() { return nullptr; }
  virtual ProcessedMediaStream* AsProcessedStream() { return nullptr; }

  
  void Init();
  
  
  



  virtual void DestroyImpl();
  StreamTime GetBufferEnd() { return mBuffer.GetEnd(); }
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
  void ChangeExplicitBlockerCountImpl(StreamTime aTime, PRInt32 aDelta)
  {
    mExplicitBlockerCount.SetAtAndAfter(aTime, mExplicitBlockerCount.GetAt(aTime) + aDelta);
  }
  void AddListenerImpl(already_AddRefed<MediaStreamListener> aListener);
  void RemoveListenerImpl(MediaStreamListener* aListener)
  {
    mListeners.RemoveElement(aListener);
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
  bool IsFinishedOnGraphThread() { return mFinished; }
  void FinishOnGraphThread();

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
  
  
  TimeVarying<GraphTime,PRUint32> mExplicitBlockerCount;
  nsTArray<nsRefPtr<MediaStreamListener> > mListeners;

  
  
  
  
  
  
  TimeVarying<GraphTime,bool> mBlocked;
  
  TimeVarying<GraphTime,PRInt64> mGraphUpdateIndices;

  
  nsTArray<MediaInputPort*> mConsumers;

  



  bool mFinished;
  



  bool mNotifiedFinished;

  
  nsRefPtr<nsAudioStream> mAudioOutput;
  
  
  GraphTime mAudioPlaybackStartTime;
  
  
  MediaTime mBlockedAudioTime;

  
  
  
  TrackID mFirstActiveTracks[MediaSegment::TYPE_COUNT];

  
  bool mHasBeenOrdered;
  bool mIsOnOrderingStack;
  
  
  
  bool mIsConsumed;
  
  bool mKnowIsConsumed;
  
  
  
  bool mInBlockingSet;
  
  bool mBlockInThisPhase;

  
  nsDOMMediaStream* mWrapper;
  
  StreamTime mMainThreadCurrentTime;
  bool mMainThreadFinished;
  bool mMainThreadDestroyed;
};







class SourceMediaStream : public MediaStream {
public:
  SourceMediaStream(nsDOMMediaStream* aWrapper) :
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
  





  void AddTrack(TrackID aID, TrackRate aRate, TrackTicks aStart,
                MediaSegment* aSegment);
  



  void AppendToTrack(TrackID aID, MediaSegment* aSegment);
  


  bool HaveEnoughBuffered(TrackID aID);
  




  void DispatchWhenNotEnoughBuffered(TrackID aID,
      nsIThread* aSignalThread, nsIRunnable* aSignalRunnable);
  



  void EndTrack(TrackID aID);
  



  void AdvanceKnownTracksTime(StreamTime aKnownTime);
  





  void Finish();

  

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
    
    
    PRUint32 mCommands;
    
    
    nsAutoPtr<MediaSegment> mData;
    nsTArray<ThreadAndRunnable> mDispatchWhenNotEnough;
    bool mHaveEnough;
  };

protected:
  TrackData* FindDataForTrack(TrackID aID)
  {
    for (PRUint32 i = 0; i < mUpdateTracks.Length(); ++i) {
      if (mUpdateTracks[i].mID == aID) {
        return &mUpdateTracks[i];
      }
    }
    NS_ERROR("Bad track ID!");
    return nullptr;
  }

  
  MediaStreamListener::Consumption mLastConsumptionState;

  
  
  Mutex mMutex;
  
  StreamTime mUpdateKnownTracksTime;
  nsTArray<TrackData> mUpdateTracks;
  bool mPullEnabled;
  bool mUpdateFinished;
  bool mDestroyed;
};


















class MediaInputPort {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaInputPort)

  




  enum {
    
    
    FLAG_BLOCK_INPUT = 0x01,
    
    
    FLAG_BLOCK_OUTPUT = 0x02
  };
  
  MediaInputPort(MediaStream* aSource, ProcessedMediaStream* aDest,
                 PRUint32 aFlags)
    : mSource(aSource)
    , mDest(aDest)
    , mFlags(aFlags)
  {
    MOZ_COUNT_CTOR(MediaInputPort);
  }
  ~MediaInputPort()
  {
    MOZ_COUNT_DTOR(MediaInputPort);
  }

  
  
  void Init();
  
  void Disconnect();

  
  



  void Destroy();

  
  MediaStream* GetSource() { return mSource; }
  ProcessedMediaStream* GetDestination() { return mDest; }

  
  struct InputInterval {
    GraphTime mStart;
    GraphTime mEnd;
    bool mInputIsBlocked;
  };
  
  
  InputInterval GetNextInputInterval(GraphTime aTime);

protected:
  friend class MediaStreamGraphImpl;
  friend class MediaStream;
  friend class ProcessedMediaStream;
  
  MediaStream* mSource;
  ProcessedMediaStream* mDest;
  PRUint32 mFlags;
};






class ProcessedMediaStream : public MediaStream {
public:
  ProcessedMediaStream(nsDOMMediaStream* aWrapper)
    : MediaStream(aWrapper), mAutofinish(false), mInCycle(false)
  {}

  
  



  MediaInputPort* AllocateInputPort(MediaStream* aStream, PRUint32 aFlags = 0);
  


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

protected:
  

  
  nsTArray<MediaInputPort*> mInputs;
  bool mAutofinish;
  
  
  bool mInCycle;
};





class MediaStreamGraph {
public:
  
  static MediaStreamGraph* GetInstance();
  
  



  SourceMediaStream* CreateInputStream(nsDOMMediaStream* aWrapper);
  













  ProcessedMediaStream* CreateTrackUnionStream(nsDOMMediaStream* aWrapper);
  




  PRInt64 GetCurrentGraphUpdateIndex() { return mGraphUpdatesSent; }

  





  void DispatchToMainThreadAfterStreamStateUpdate(nsIRunnable* aRunnable)
  {
    mPendingUpdateRunnables.AppendElement(aRunnable);
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

  
  
  
  PRInt64 mGraphUpdatesSent;
};

}

#endif 
