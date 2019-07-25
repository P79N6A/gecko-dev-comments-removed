




#include "MediaStreamGraph.h"

#include "mozilla/Monitor.h"
#include "mozilla/TimeStamp.h"
#include "AudioSegment.h"
#include "VideoSegment.h"
#include "nsContentUtils.h"
#include "nsIAppShell.h"
#include "nsIObserver.h"
#include "nsServiceManagerUtils.h"
#include "nsWidgetsCID.h"
#include "nsXPCOMCIDInternal.h"
#include "prlog.h"
#include "VideoUtils.h"
#include "mozilla/Attributes.h"

using namespace mozilla::layers;

namespace mozilla {

namespace {

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaStreamGraphLog;
#define LOG(type, msg) PR_LOG(gMediaStreamGraphLog, type, msg)
#else
#define LOG(type, msg)
#endif






const int MEDIA_GRAPH_TARGET_PERIOD_MS = 10;





const int SCHEDULE_SAFETY_MARGIN_MS = 10;









const int AUDIO_TARGET_MS = 2*MEDIA_GRAPH_TARGET_PERIOD_MS +
    SCHEDULE_SAFETY_MARGIN_MS;







const int VIDEO_TARGET_MS = 2*MEDIA_GRAPH_TARGET_PERIOD_MS +
    SCHEDULE_SAFETY_MARGIN_MS;





struct StreamUpdate {
  PRInt64 mGraphUpdateIndex;
  nsRefPtr<MediaStream> mStream;
  StreamTime mNextMainThreadCurrentTime;
  bool mNextMainThreadFinished;
};





class ControlMessage {
public:
  ControlMessage(MediaStream* aStream) : mStream(aStream)
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

}








class MediaStreamGraphImpl : public MediaStreamGraph {
public:
  MediaStreamGraphImpl();
  ~MediaStreamGraphImpl()
  {
    NS_ASSERTION(IsEmpty(),
                 "All streams should have been destroyed by messages from the main thread");
    LOG(PR_LOG_DEBUG, ("MediaStreamGraph %p destroyed", this));
  }

  
  





  void RunInStableState();
  




  void EnsureRunInStableState();
  


  void ApplyStreamUpdate(StreamUpdate* aUpdate);
  



  void AppendMessage(ControlMessage* aMessage);
  





  void ForceShutDown();
  


  void ShutdownThreads();

  
  
  



  void RunThread();
  



  void EnsureNextIteration();
  


  void EnsureNextIterationLocked(MonitorAutoLock& aLock);
  



  void EnsureImmediateWakeUpLocked(MonitorAutoLock& aLock);
  




  void EnsureStableStateEventPosted();
  



  void PrepareUpdatesToMainThreadState();
  
  



  void UpdateCurrentTime();
  



  void UpdateConsumptionState(SourceMediaStream* aStream);
  


  void ExtractPendingInput(SourceMediaStream* aStream,
                           GraphTime aDesiredUpToTime,
                           bool* aEnsureNextIteration);
  


  void UpdateBufferSufficiencyState(SourceMediaStream* aStream);
  





  void RecomputeBlocking(GraphTime aEndBlockingDecisions);
  
  




  void MarkStreamBlocked(MediaStream* aStream, GraphTime aTime, GraphTime* aEnd);
  





  void RecomputeBlockingAt(GraphTime aTime, GraphTime aEndBlockingDecisions,
                           GraphTime* aEnd);
  





  bool WillUnderrun(MediaStream* aStream, GraphTime aTime,
                    GraphTime aEndBlockingDecisions, GraphTime* aEnd);
  



  bool IsAlwaysExplicitlyBlocked(MediaStream* aStream);
  



  StreamTime GraphTimeToStreamTime(MediaStream* aStream, GraphTime aTime);
  enum {
    INCLUDE_TRAILING_BLOCKED_INTERVAL = 0x01
  };
  








  GraphTime StreamTimeToGraphTime(MediaStream* aStream, StreamTime aTime,
                                  PRUint32 aFlags = 0);
  


  GraphTime GetAudioPosition(MediaStream* aStream);
  



  void CreateOrDestroyAudioStream(GraphTime aAudioOutputStartTime,
                                  MediaStream* aStream);
  


  void UpdateFirstActiveTracks(MediaStream* aStream);
  



  void PlayAudio(MediaStream* aStream, GraphTime aFrom, GraphTime aTo);
  


  void PlayVideo(MediaStream* aStream);
  




  void FinishStream(MediaStream* aStream);
  


  StreamTime GetDesiredBufferEnd(MediaStream* aStream);
  


  bool IsEmpty() { return mStreams.IsEmpty(); }

  
  


  PRInt64 GetProcessingGraphUpdateIndex() { return mProcessingGraphUpdateIndex; }
  


  void AddStream(MediaStream* aStream);
  



  void RemoveStream(MediaStream* aStream);

  

  



  nsCOMPtr<nsIThread> mThread;

  
  
  

  nsTArray<nsRefPtr<MediaStream> > mStreams;
  



  GraphTime mCurrentTime;
  




  GraphTime mStateComputedTime;
  


  TimeStamp mInitialTimeStamp;
  


  TimeStamp mCurrentTimeStamp;
  


  PRInt64 mProcessingGraphUpdateIndex;

  
  
  
  
  
  Monitor mMonitor;

  
  

  


  nsTArray<StreamUpdate> mStreamUpdates;
  


  nsTArray<nsCOMPtr<nsIRunnable> > mUpdateRunnables;
  struct MessageBlock {
    PRInt64 mGraphUpdateIndex;
    nsTArray<nsAutoPtr<ControlMessage> > mMessages;
  };
  



  nsTArray<MessageBlock> mMessageQueue;
  


















  enum LifecycleState {
    
    LIFECYCLE_THREAD_NOT_STARTED,
    
    LIFECYCLE_RUNNING,
    
    
    
    
    
    
    LIFECYCLE_WAITING_FOR_MAIN_THREAD_CLEANUP,
    
    
    LIFECYCLE_WAITING_FOR_THREAD_SHUTDOWN,
    
    
    
    LIFECYCLE_WAITING_FOR_STREAM_DESTRUCTION
  };
  LifecycleState mLifecycleState;
  


  enum WaitState {
    
    WAITSTATE_RUNNING,
    
    
    WAITSTATE_WAITING_FOR_NEXT_ITERATION,
    
    WAITSTATE_WAITING_INDEFINITELY,
    
    
    WAITSTATE_WAKING_UP
  };
  WaitState mWaitState;
  


  bool mNeedAnotherIteration;
  


  bool mForceShutDown;
  



  bool mPostedRunInStableStateEvent;

  

  





  nsTArray<nsAutoPtr<ControlMessage> > mCurrentTaskMessageQueue;
  




  bool mDetectedNotRunning;
  



  bool mPostedRunInStableState;
};




static MediaStreamGraphImpl* gGraph;

StreamTime
MediaStreamGraphImpl::GetDesiredBufferEnd(MediaStream* aStream)
{
  StreamTime current = mCurrentTime - aStream->mBufferStartTime;
  StreamTime desiredEnd = current;
  if (!aStream->mAudioOutputs.IsEmpty()) {
    desiredEnd = NS_MAX(desiredEnd, current + MillisecondsToMediaTime(AUDIO_TARGET_MS));
  }
  if (!aStream->mVideoOutputs.IsEmpty()) {
    desiredEnd = NS_MAX(desiredEnd, current + MillisecondsToMediaTime(VIDEO_TARGET_MS));
  }
  return desiredEnd;
}

bool
MediaStreamGraphImpl::IsAlwaysExplicitlyBlocked(MediaStream* aStream)
{
  GraphTime t = mCurrentTime;
  while (true) {
    GraphTime end;
    if (aStream->mExplicitBlockerCount.GetAt(t, &end) == 0)
      return false;
    if (end >= GRAPH_TIME_MAX)
      return true;
    t = end;
  }
}

void
MediaStreamGraphImpl::FinishStream(MediaStream* aStream)
{
  if (aStream->mFinished)
    return;
  printf("MediaStreamGraphImpl::FinishStream\n");
  LOG(PR_LOG_DEBUG, ("MediaStream %p will finish", aStream));
  aStream->mFinished = true;
  
  
  
  EnsureNextIteration();
}

void
MediaStreamGraphImpl::AddStream(MediaStream* aStream)
{
  aStream->mBufferStartTime = mCurrentTime;
  *mStreams.AppendElement() = already_AddRefed<MediaStream>(aStream);
  LOG(PR_LOG_DEBUG, ("Adding media stream %p to the graph", aStream));
}

void
MediaStreamGraphImpl::RemoveStream(MediaStream* aStream)
{
  
  
  
  {
    MonitorAutoLock lock(mMonitor);
    for (PRUint32 i = 0; i < mStreamUpdates.Length(); ++i) {
      if (mStreamUpdates[i].mStream == aStream) {
        mStreamUpdates[i].mStream = nullptr;
      }
    }
  }

  
  mStreams.RemoveElement(aStream);

  LOG(PR_LOG_DEBUG, ("Removing media stream %p from the graph", aStream));
}

void
MediaStreamGraphImpl::UpdateConsumptionState(SourceMediaStream* aStream)
{
  bool isConsumed = !aStream->mAudioOutputs.IsEmpty() ||
    !aStream->mVideoOutputs.IsEmpty();
  MediaStreamListener::Consumption state = isConsumed ? MediaStreamListener::CONSUMED
    : MediaStreamListener::NOT_CONSUMED;
  if (state != aStream->mLastConsumptionState) {
    aStream->mLastConsumptionState = state;
    for (PRUint32 j = 0; j < aStream->mListeners.Length(); ++j) {
      MediaStreamListener* l = aStream->mListeners[j];
      l->NotifyConsumptionChanged(this, state);
    }
  }
}

void
MediaStreamGraphImpl::ExtractPendingInput(SourceMediaStream* aStream,
                                          GraphTime aDesiredUpToTime,
                                          bool* aEnsureNextIteration)
{
  bool finished;
  {
    MutexAutoLock lock(aStream->mMutex);
    if (aStream->mPullEnabled) {
      for (PRUint32 j = 0; j < aStream->mListeners.Length(); ++j) {
        MediaStreamListener* l = aStream->mListeners[j];
        {
          
          
          
          StreamTime t =
            GraphTimeToStreamTime(aStream, mStateComputedTime) +
            (aDesiredUpToTime - mStateComputedTime);
          MutexAutoUnlock unlock(aStream->mMutex);
          l->NotifyPull(this, t);
          *aEnsureNextIteration = true;
        }
      }
    }
    finished = aStream->mUpdateFinished;
    for (PRInt32 i = aStream->mUpdateTracks.Length() - 1; i >= 0; --i) {
      SourceMediaStream::TrackData* data = &aStream->mUpdateTracks[i];
      for (PRUint32 j = 0; j < aStream->mListeners.Length(); ++j) {
        MediaStreamListener* l = aStream->mListeners[j];
        TrackTicks offset = (data->mCommands & SourceMediaStream::TRACK_CREATE)
            ? data->mStart : aStream->mBuffer.FindTrack(data->mID)->GetSegment()->GetDuration();
        l->NotifyQueuedTrackChanges(this, data->mID, data->mRate,
                                    offset, data->mCommands, *data->mData);
      }
      if (data->mCommands & SourceMediaStream::TRACK_CREATE) {
        MediaSegment* segment = data->mData.forget();
        LOG(PR_LOG_DEBUG, ("SourceMediaStream %p creating track %d, rate %d, start %lld, initial end %lld",
                           aStream, data->mID, data->mRate, PRInt64(data->mStart),
                           PRInt64(segment->GetDuration())));
        aStream->mBuffer.AddTrack(data->mID, data->mRate, data->mStart, segment);
        
        
        data->mData = segment->CreateEmptyClone();
        data->mCommands &= ~SourceMediaStream::TRACK_CREATE;
      } else if (data->mData->GetDuration() > 0) {
        MediaSegment* dest = aStream->mBuffer.FindTrack(data->mID)->GetSegment();
        LOG(PR_LOG_DEBUG, ("SourceMediaStream %p track %d, advancing end from %lld to %lld",
                           aStream, data->mID,
                           PRInt64(dest->GetDuration()),
                           PRInt64(dest->GetDuration() + data->mData->GetDuration())));
        dest->AppendFrom(data->mData);
      }
      if (data->mCommands & SourceMediaStream::TRACK_END) {
        aStream->mBuffer.FindTrack(data->mID)->SetEnded();
        aStream->mUpdateTracks.RemoveElementAt(i);
      }
    }
    aStream->mBuffer.AdvanceKnownTracksTime(aStream->mUpdateKnownTracksTime);
  }
  if (finished) {
    FinishStream(aStream);
  }
}

void
MediaStreamGraphImpl::UpdateBufferSufficiencyState(SourceMediaStream* aStream)
{
  StreamTime desiredEnd = GetDesiredBufferEnd(aStream);
  nsTArray<SourceMediaStream::ThreadAndRunnable> runnables;

  {
    MutexAutoLock lock(aStream->mMutex);
    for (PRUint32 i = 0; i < aStream->mUpdateTracks.Length(); ++i) {
      SourceMediaStream::TrackData* data = &aStream->mUpdateTracks[i];
      if (data->mCommands & SourceMediaStream::TRACK_CREATE) {
        
        
        
        
        continue;
      }
      if (data->mCommands & SourceMediaStream::TRACK_END) {
        
        
        continue;
      }
      StreamBuffer::Track* track = aStream->mBuffer.FindTrack(data->mID);
      
      
      NS_ASSERTION(!track->IsEnded(), "What is this track doing here?");
      data->mHaveEnough = track->GetEndTimeRoundDown() >= desiredEnd;
      if (!data->mHaveEnough) {
        runnables.MoveElementsFrom(data->mDispatchWhenNotEnough);
      }
    }
  }

  for (PRUint32 i = 0; i < runnables.Length(); ++i) {
    runnables[i].mThread->Dispatch(runnables[i].mRunnable, 0);
  }
}


StreamTime
MediaStreamGraphImpl::GraphTimeToStreamTime(MediaStream* aStream,
                                            GraphTime aTime)
{
  NS_ASSERTION(aTime <= mStateComputedTime,
               "Don't ask about times where we haven't made blocking decisions yet");
  if (aTime <= mCurrentTime) {
    return NS_MAX<StreamTime>(0, aTime - aStream->mBufferStartTime);
  }
  GraphTime t = mCurrentTime;
  StreamTime s = t - aStream->mBufferStartTime;
  while (t < aTime) {
    GraphTime end;
    if (!aStream->mBlocked.GetAt(t, &end)) {
      s += NS_MIN(aTime, end) - t;
    }
    t = end;
  }
  return NS_MAX<StreamTime>(0, s);
}  

GraphTime
MediaStreamGraphImpl::StreamTimeToGraphTime(MediaStream* aStream,
                                            StreamTime aTime, PRUint32 aFlags)
{
  if (aTime >= STREAM_TIME_MAX) {
    return GRAPH_TIME_MAX;
  }
  MediaTime bufferElapsedToCurrentTime = mCurrentTime - aStream->mBufferStartTime;
  if (aTime < bufferElapsedToCurrentTime ||
      (aTime == bufferElapsedToCurrentTime && !(aFlags & INCLUDE_TRAILING_BLOCKED_INTERVAL))) {
    return aTime + aStream->mBufferStartTime;
  }

  MediaTime streamAmount = aTime - bufferElapsedToCurrentTime;
  NS_ASSERTION(streamAmount >= 0, "Can't answer queries before current time");

  GraphTime t = mCurrentTime;
  while (t < GRAPH_TIME_MAX) {
    bool blocked;
    GraphTime end;
    if (t < mStateComputedTime) {
      blocked = aStream->mBlocked.GetAt(t, &end);
      end = NS_MIN(end, mStateComputedTime);
    } else {
      blocked = false;
      end = GRAPH_TIME_MAX;
    }
    if (blocked) {
      t = end;
    } else {
      if (streamAmount == 0) {
        
        break;
      }
      MediaTime consume = NS_MIN(end - t, streamAmount);
      streamAmount -= consume;
      t += consume;
    }
  }
  return t;
}

GraphTime
MediaStreamGraphImpl::GetAudioPosition(MediaStream* aStream)
{
  if (!aStream->mAudioOutput) {
    return mCurrentTime;
  }
  PRInt64 positionInFrames = aStream->mAudioOutput->GetPositionInFrames();
  if (positionInFrames < 0) {
    return mCurrentTime;
  }
  return aStream->mAudioPlaybackStartTime +
      TicksToTimeRoundDown(aStream->mAudioOutput->GetRate(),
                           positionInFrames);
}

void
MediaStreamGraphImpl::UpdateCurrentTime()
{
  GraphTime prevCurrentTime = mCurrentTime;
  TimeStamp now = TimeStamp::Now();
  GraphTime nextCurrentTime =
    SecondsToMediaTime((now - mCurrentTimeStamp).ToSeconds()) + mCurrentTime;
  if (mStateComputedTime < nextCurrentTime) {
    LOG(PR_LOG_WARNING, ("Media graph global underrun detected"));
    LOG(PR_LOG_DEBUG, ("Advancing mStateComputedTime from %f to %f",
                       MediaTimeToSeconds(mStateComputedTime),
                       MediaTimeToSeconds(nextCurrentTime)));
    
    
    for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
      mStreams[i]->mBlocked.SetAtAndAfter(mStateComputedTime, true);
    }
    mStateComputedTime = nextCurrentTime;
  }
  mCurrentTimeStamp = now;

  LOG(PR_LOG_DEBUG, ("Updating current time to %f (real %f, mStateComputedTime %f)",
                     MediaTimeToSeconds(nextCurrentTime),
                     (now - mInitialTimeStamp).ToSeconds(),
                     MediaTimeToSeconds(mStateComputedTime)));

  if (prevCurrentTime >= nextCurrentTime) {
    NS_ASSERTION(prevCurrentTime == nextCurrentTime, "Time can't go backwards!");
    
    LOG(PR_LOG_DEBUG, ("Time did not advance"));
    
    
  }

  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    MediaStream* stream = mStreams[i];

    
    GraphTime blockedTime = 0;
    GraphTime t = prevCurrentTime;
    
    bool wasBlocked = stream->mBlocked.GetAt(prevCurrentTime);
    while (t < nextCurrentTime) {
      GraphTime end;
      bool blocked = stream->mBlocked.GetAt(t, &end);
      if (blocked) {
        blockedTime += NS_MIN(end, nextCurrentTime) - t;
      }
      if (blocked != wasBlocked) {
        for (PRUint32 j = 0; j < stream->mListeners.Length(); ++j) {
          MediaStreamListener* l = stream->mListeners[j];
          l->NotifyBlockingChanged(this,
              blocked ? MediaStreamListener::BLOCKED : MediaStreamListener::UNBLOCKED);
        }
        wasBlocked = blocked;
      }
      t = end;
    }

    stream->AdvanceTimeVaryingValuesToCurrentTime(nextCurrentTime, blockedTime);
    
    
    stream->mBlocked.AdvanceCurrentTime(nextCurrentTime);

    if (blockedTime < nextCurrentTime - prevCurrentTime) {
      for (PRUint32 i = 0; i < stream->mListeners.Length(); ++i) {
        MediaStreamListener* l = stream->mListeners[i];
        l->NotifyOutput(this);
      }
    }

    if (stream->mFinished && !stream->mNotifiedFinished &&
        stream->mBufferStartTime + stream->GetBufferEnd() <= nextCurrentTime) {
      stream->mNotifiedFinished = true;
      for (PRUint32 j = 0; j < stream->mListeners.Length(); ++j) {
        MediaStreamListener* l = stream->mListeners[j];
        l->NotifyFinished(this);
      }
    }

    LOG(PR_LOG_DEBUG, ("MediaStream %p bufferStartTime=%f blockedTime=%f",
                       stream, MediaTimeToSeconds(stream->mBufferStartTime),
                       MediaTimeToSeconds(blockedTime)));
  }

  mCurrentTime = nextCurrentTime;
}

void
MediaStreamGraphImpl::MarkStreamBlocked(MediaStream* aStream,
                                        GraphTime aTime, GraphTime* aEnd)
{
  NS_ASSERTION(!aStream->mBlocked.GetAt(aTime), "MediaStream already blocked");

  aStream->mBlocked.SetAtAndAfter(aTime, true);
}

bool
MediaStreamGraphImpl::WillUnderrun(MediaStream* aStream, GraphTime aTime,
                                   GraphTime aEndBlockingDecisions, GraphTime* aEnd)
{
  
  if (aStream->mFinished ||
      (aStream->mAudioOutputs.IsEmpty() && aStream->mVideoOutputs.IsEmpty())) {
    return false;
  }
  GraphTime bufferEnd =
    StreamTimeToGraphTime(aStream, aStream->GetBufferEnd(),
                          INCLUDE_TRAILING_BLOCKED_INTERVAL);
  NS_ASSERTION(bufferEnd >= mCurrentTime, "Buffer underran");
  
  if (bufferEnd <= aTime) {
    LOG(PR_LOG_DEBUG, ("MediaStream %p will block due to data underrun, "
                       "bufferEnd %f",
                       aStream, MediaTimeToSeconds(bufferEnd)));
    return true;
  }
  
  
  
  
  
  if (bufferEnd <= aEndBlockingDecisions && aStream->mBlocked.GetBefore(aTime)) {
    LOG(PR_LOG_DEBUG, ("MediaStream %p will block due to speculative data underrun, "
                       "bufferEnd %f",
                       aStream, MediaTimeToSeconds(bufferEnd)));
    return true;
  }
  
  *aEnd = NS_MIN(*aEnd, bufferEnd);
  return false;
}

void
MediaStreamGraphImpl::RecomputeBlocking(GraphTime aEndBlockingDecisions)
{
  bool blockingDecisionsWillChange = false;
  while (mStateComputedTime < aEndBlockingDecisions) {
    LOG(PR_LOG_DEBUG, ("Media graph %p computing blocking for time %f",
                       this, MediaTimeToSeconds(mStateComputedTime)));
    GraphTime end = GRAPH_TIME_MAX;
    RecomputeBlockingAt(mStateComputedTime, aEndBlockingDecisions, &end);
    LOG(PR_LOG_DEBUG, ("Media graph %p computed blocking for interval %f to %f",
                       this, MediaTimeToSeconds(mStateComputedTime),
                       MediaTimeToSeconds(end)));                       
    mStateComputedTime = end;
    if (end < GRAPH_TIME_MAX) {
      blockingDecisionsWillChange = true;
    }
  }
  mStateComputedTime = aEndBlockingDecisions;

  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    MediaStream* stream = mStreams[i];
    GraphTime end;
    stream->mBlocked.GetAt(mCurrentTime, &end);
    if (end < GRAPH_TIME_MAX) {
      blockingDecisionsWillChange = true;
    }
  }
  if (blockingDecisionsWillChange) {
    
    EnsureNextIteration();
  }
}

void
MediaStreamGraphImpl::RecomputeBlockingAt(GraphTime aTime,
                                          GraphTime aEndBlockingDecisions,
                                          GraphTime* aEnd)
{
  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    MediaStream* stream = mStreams[i];
    stream->mBlocked.SetAtAndAfter(aTime, false);
  }

  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    MediaStream* stream = mStreams[i];
    
    
    if (stream->mBlocked.GetAt(aTime)) {
      continue;
    }

    if (stream->mFinished) {
      GraphTime endTime = StreamTimeToGraphTime(stream, stream->GetBufferEnd());
      if (endTime <= aTime) {
        LOG(PR_LOG_DEBUG, ("MediaStream %p is blocked due to being finished", stream));
        MarkStreamBlocked(stream, aTime, aEnd);
        continue;
      } else {
        LOG(PR_LOG_DEBUG, ("MediaStream %p is finished, but not blocked yet (end at %f, with blocking at %f)",
                           stream, MediaTimeToSeconds(stream->GetBufferEnd()),
                           MediaTimeToSeconds(endTime)));
        *aEnd = NS_MIN(*aEnd, endTime);
      }
    }

    
    
    GraphTime end;
    bool explicitBlock = stream->mExplicitBlockerCount.GetAt(aTime, &end) > 0;
    *aEnd = NS_MIN(*aEnd, end);
    if (explicitBlock) {
      LOG(PR_LOG_DEBUG, ("MediaStream %p is blocked due to explicit blocker", stream));
      MarkStreamBlocked(stream, aTime, aEnd);
      continue;
    }

    bool underrun = WillUnderrun(stream, aTime, aEndBlockingDecisions, aEnd);
    if (underrun) {
      MarkStreamBlocked(stream, aTime, aEnd);
      continue;
    }
  }

  NS_ASSERTION(*aEnd > aTime, "Failed to advance!");
}

void
MediaStreamGraphImpl::UpdateFirstActiveTracks(MediaStream* aStream)
{
  StreamBuffer::Track* newTracksByType[MediaSegment::TYPE_COUNT];
  for (PRUint32 i = 0; i < ArrayLength(newTracksByType); ++i) {
    newTracksByType[i] = nullptr;
  }

  for (StreamBuffer::TrackIter iter(aStream->mBuffer);
       !iter.IsEnded(); iter.Next()) {
    MediaSegment::Type type = iter->GetType();
    if ((newTracksByType[type] &&
         iter->GetStartTimeRoundDown() < newTracksByType[type]->GetStartTimeRoundDown()) ||
         aStream->mFirstActiveTracks[type] == TRACK_NONE) {
      newTracksByType[type] = &(*iter);
      aStream->mFirstActiveTracks[type] = iter->GetID();
    }
  }
}

void
MediaStreamGraphImpl::CreateOrDestroyAudioStream(GraphTime aAudioOutputStartTime,
                                                 MediaStream* aStream)
{
  StreamBuffer::Track* track;

  if (aStream->mAudioOutputs.IsEmpty() ||
      !(track = aStream->mBuffer.FindTrack(aStream->mFirstActiveTracks[MediaSegment::AUDIO]))) {
    if (aStream->mAudioOutput) {
      aStream->mAudioOutput->Shutdown();
      aStream->mAudioOutput = nullptr;
    }
    return;
  }

  if (aStream->mAudioOutput)
    return;

  
  GraphTime startTime =
    StreamTimeToGraphTime(aStream, track->GetStartTimeRoundDown(),
                          INCLUDE_TRAILING_BLOCKED_INTERVAL);
  if (startTime >= mStateComputedTime) {
    
    
    return;
  }

  

  
  
  

  AudioSegment* audio = track->Get<AudioSegment>();
  aStream->mAudioPlaybackStartTime = aAudioOutputStartTime;
  aStream->mAudioOutput = nsAudioStream::AllocateStream();
  aStream->mAudioOutput->Init(audio->GetChannels(),
                              track->GetRate(),
                              audio->GetFirstFrameFormat());
}

void
MediaStreamGraphImpl::PlayAudio(MediaStream* aStream,
                                GraphTime aFrom, GraphTime aTo)
{
  if (!aStream->mAudioOutput)
    return;

  StreamBuffer::Track* track =
    aStream->mBuffer.FindTrack(aStream->mFirstActiveTracks[MediaSegment::AUDIO]);
  AudioSegment* audio = track->Get<AudioSegment>();

  
  
  float volume = 0.0f;
  for (PRUint32 i = 0; i < aStream->mAudioOutputs.Length(); ++i) {
    volume += aStream->mAudioOutputs[i].mVolume;
  }

  
  
  
  
  
  GraphTime t = aFrom;
  while (t < aTo) {
    GraphTime end;
    bool blocked = aStream->mBlocked.GetAt(t, &end);
    end = NS_MIN(end, aTo);

    AudioSegment output;
    if (blocked) {
      
      
      
      TrackTicks startTicks =
          TimeToTicksRoundDown(track->GetRate(), aStream->mBlockedAudioTime);
      aStream->mBlockedAudioTime += end - t;
      TrackTicks endTicks =
          TimeToTicksRoundDown(track->GetRate(), aStream->mBlockedAudioTime);

      output.InitFrom(*audio);
      output.InsertNullDataAtStart(endTicks - startTicks);
      LOG(PR_LOG_DEBUG, ("MediaStream %p writing blocking-silence samples for %f to %f",
                         aStream, MediaTimeToSeconds(t), MediaTimeToSeconds(end)));
    } else {
      TrackTicks startTicks =
          track->TimeToTicksRoundDown(GraphTimeToStreamTime(aStream, t));
      TrackTicks endTicks =
          track->TimeToTicksRoundDown(GraphTimeToStreamTime(aStream, end));

      output.SliceFrom(*audio, startTicks, endTicks);
      output.ApplyVolume(volume);
      LOG(PR_LOG_DEBUG, ("MediaStream %p writing samples for %f to %f (samples %lld to %lld)",
                         aStream, MediaTimeToSeconds(t), MediaTimeToSeconds(end),
                         startTicks, endTicks));
    }
    output.WriteTo(aStream->mAudioOutput);
    t = end;
  }
}

void
MediaStreamGraphImpl::PlayVideo(MediaStream* aStream)
{
  if (aStream->mVideoOutputs.IsEmpty())
    return;

  StreamBuffer::Track* track =
    aStream->mBuffer.FindTrack(aStream->mFirstActiveTracks[MediaSegment::VIDEO]);
  if (!track)
    return;
  VideoSegment* video = track->Get<VideoSegment>();

  
  
  GraphTime framePosition = mCurrentTime + MEDIA_GRAPH_TARGET_PERIOD_MS;
  NS_ASSERTION(framePosition >= aStream->mBufferStartTime, "frame position before buffer?");
  StreamTime frameBufferTime = GraphTimeToStreamTime(aStream, framePosition);
  TrackTicks start;
  const VideoFrame* frame =
    video->GetFrameAt(track->TimeToTicksRoundDown(frameBufferTime), &start);
  if (!frame) {
    frame = video->GetLastFrame(&start);
    if (!frame)
      return;
  }

  if (*frame != aStream->mLastPlayedVideoFrame) {
    LOG(PR_LOG_DEBUG, ("MediaStream %p writing video frame %p (%dx%d)",
                       aStream, frame->GetImage(), frame->GetIntrinsicSize().width,
                       frame->GetIntrinsicSize().height));
    GraphTime startTime = StreamTimeToGraphTime(aStream,
        track->TicksToTimeRoundDown(start), INCLUDE_TRAILING_BLOCKED_INTERVAL);
    TimeStamp targetTime = mCurrentTimeStamp +
        TimeDuration::FromMilliseconds(double(startTime - mCurrentTime));
    for (PRUint32 i = 0; i < aStream->mVideoOutputs.Length(); ++i) {
      VideoFrameContainer* output = aStream->mVideoOutputs[i];
      output->SetCurrentFrame(frame->GetIntrinsicSize(), frame->GetImage(),
                              targetTime);
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(output, &VideoFrameContainer::Invalidate);
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    }
    aStream->mLastPlayedVideoFrame = *frame;
  }
}

void
MediaStreamGraphImpl::PrepareUpdatesToMainThreadState()
{
  mMonitor.AssertCurrentThreadOwns();

  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    MediaStream* stream = mStreams[i];
    StreamUpdate* update = mStreamUpdates.AppendElement();
    update->mGraphUpdateIndex = stream->mGraphUpdateIndices.GetAt(mCurrentTime);
    update->mStream = stream;
    update->mNextMainThreadCurrentTime =
      GraphTimeToStreamTime(stream, mCurrentTime);
    update->mNextMainThreadFinished =
      stream->mFinished &&
      StreamTimeToGraphTime(stream, stream->GetBufferEnd()) <= mCurrentTime;
  }
  mUpdateRunnables.MoveElementsFrom(mPendingUpdateRunnables);

  EnsureStableStateEventPosted();
}

void
MediaStreamGraphImpl::EnsureImmediateWakeUpLocked(MonitorAutoLock& aLock)
{
  if (mWaitState == WAITSTATE_WAITING_FOR_NEXT_ITERATION ||
      mWaitState == WAITSTATE_WAITING_INDEFINITELY) {
    mWaitState = WAITSTATE_WAKING_UP;
    aLock.Notify();
  }
}

void
MediaStreamGraphImpl::EnsureNextIteration()
{
  MonitorAutoLock lock(mMonitor);
  EnsureNextIterationLocked(lock);
}

void
MediaStreamGraphImpl::EnsureNextIterationLocked(MonitorAutoLock& aLock)
{
  if (mNeedAnotherIteration)
    return;
  mNeedAnotherIteration = true;
  if (mWaitState == WAITSTATE_WAITING_INDEFINITELY) {
    mWaitState = WAITSTATE_WAKING_UP;
    aLock.Notify();
  }
}

void
MediaStreamGraphImpl::RunThread()
{
  nsTArray<MessageBlock> messageQueue;
  {
    MonitorAutoLock lock(mMonitor);
    messageQueue.SwapElements(mMessageQueue);
  }
  NS_ASSERTION(!messageQueue.IsEmpty(),
               "Shouldn't have started a graph with empty message queue!");

  for (;;) {
    
    
    UpdateCurrentTime();

    
    
    
    for (PRUint32 i = 0; i < messageQueue.Length(); ++i) {
      mProcessingGraphUpdateIndex = messageQueue[i].mGraphUpdateIndex;
      nsTArray<nsAutoPtr<ControlMessage> >& messages = messageQueue[i].mMessages;

      for (PRUint32 j = 0; j < messages.Length(); ++j) {
        messages[j]->Run();
      }
    }
    messageQueue.Clear();

    PRInt32 writeAudioUpTo = AUDIO_TARGET_MS;
    GraphTime endBlockingDecisions =
      mCurrentTime + MillisecondsToMediaTime(writeAudioUpTo);

    
    bool ensureNextIteration = false;
    for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
      SourceMediaStream* is = mStreams[i]->AsSourceStream();
      if (is) {
        UpdateConsumptionState(is);
        ExtractPendingInput(is, endBlockingDecisions, &ensureNextIteration);
      }
    }

    GraphTime prevComputedTime = mStateComputedTime;
    RecomputeBlocking(endBlockingDecisions);

    PRUint32 audioStreamsActive = 0;
    bool allBlockedForever = true;
    
    for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
      MediaStream* stream = mStreams[i];
      UpdateFirstActiveTracks(stream);
      CreateOrDestroyAudioStream(prevComputedTime, stream);
      PlayAudio(stream, prevComputedTime, mStateComputedTime);
      if (stream->mAudioOutput) {
        ++audioStreamsActive;
      }
      PlayVideo(stream);
      SourceMediaStream* is = stream->AsSourceStream();
      if (is) {
        UpdateBufferSufficiencyState(is);
      }
      GraphTime end;
      if (!stream->mBlocked.GetAt(mCurrentTime, &end) || end < GRAPH_TIME_MAX) {
        allBlockedForever = false;
      }
    }
    if (ensureNextIteration || !allBlockedForever || audioStreamsActive > 0) {
      EnsureNextIteration();
    }

    {
      
      
      MonitorAutoLock lock(mMonitor);
      PrepareUpdatesToMainThreadState();
      if (mForceShutDown || (IsEmpty() && mMessageQueue.IsEmpty())) {
        
        
        LOG(PR_LOG_DEBUG, ("MediaStreamGraph %p waiting for main thread cleanup", this));
        
        mLifecycleState = LIFECYCLE_WAITING_FOR_MAIN_THREAD_CLEANUP;
        
        
        return;
      }

      PRIntervalTime timeout = PR_INTERVAL_NO_TIMEOUT;
      TimeStamp now = TimeStamp::Now();
      if (mNeedAnotherIteration) {
        PRInt64 timeoutMS = MEDIA_GRAPH_TARGET_PERIOD_MS -
          PRInt64((now - mCurrentTimeStamp).ToMilliseconds());
        
        
        timeoutMS = NS_MAX<PRInt64>(0, NS_MIN<PRInt64>(timeoutMS, 60*1000));
        timeout = PR_MillisecondsToInterval(PRUint32(timeoutMS));
        LOG(PR_LOG_DEBUG, ("Waiting for next iteration; at %f, timeout=%f",
                           (now - mInitialTimeStamp).ToSeconds(), timeoutMS/1000.0));
        mWaitState = WAITSTATE_WAITING_FOR_NEXT_ITERATION;
      } else {
        mWaitState = WAITSTATE_WAITING_INDEFINITELY;
      }
      if (timeout > 0) {
        mMonitor.Wait(timeout);
        LOG(PR_LOG_DEBUG, ("Resuming after timeout; at %f, elapsed=%f",
                           (TimeStamp::Now() - mInitialTimeStamp).ToSeconds(),
                           (TimeStamp::Now() - now).ToSeconds()));
      }
      mWaitState = WAITSTATE_RUNNING;
      mNeedAnotherIteration = false;
      messageQueue.SwapElements(mMessageQueue);
    }
  }
}

void
MediaStreamGraphImpl::ApplyStreamUpdate(StreamUpdate* aUpdate)
{
  mMonitor.AssertCurrentThreadOwns();

  MediaStream* stream = aUpdate->mStream;
  if (!stream)
    return;
  stream->mMainThreadCurrentTime = aUpdate->mNextMainThreadCurrentTime;
  stream->mMainThreadFinished = aUpdate->mNextMainThreadFinished;
}

void
MediaStreamGraphImpl::ShutdownThreads()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be called on main thread");
  
  LOG(PR_LOG_DEBUG, ("Stopping threads for MediaStreamGraph %p", this));

  if (mThread) {
    mThread->Shutdown();
    mThread = nullptr;
  }
}

void
MediaStreamGraphImpl::ForceShutDown()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be called on main thread");
  LOG(PR_LOG_DEBUG, ("MediaStreamGraph %p ForceShutdown", this));
  {
    MonitorAutoLock lock(mMonitor);
    mForceShutDown = true;
    EnsureImmediateWakeUpLocked(lock);
  }
}

namespace {

class MediaStreamGraphThreadRunnable : public nsRunnable {
public:
  NS_IMETHOD Run()
  {
    gGraph->RunThread();
    return NS_OK;
  }
};

class MediaStreamGraphShutDownRunnable : public nsRunnable {
public:
  MediaStreamGraphShutDownRunnable(MediaStreamGraphImpl* aGraph) : mGraph(aGraph) {}
  NS_IMETHOD Run()
  {
    NS_ASSERTION(mGraph->mDetectedNotRunning,
                 "We should know the graph thread control loop isn't running!");
    
    if (mGraph->IsEmpty()) {
      
      
      
      delete mGraph;
    } else {
      NS_ASSERTION(mGraph->mForceShutDown, "Not in forced shutdown?");
      mGraph->mLifecycleState =
        MediaStreamGraphImpl::LIFECYCLE_WAITING_FOR_STREAM_DESTRUCTION;
    }
    return NS_OK;
  }
private:
  MediaStreamGraphImpl* mGraph;
};

class MediaStreamGraphStableStateRunnable : public nsRunnable {
public:
  NS_IMETHOD Run()
  {
    if (gGraph) {
      gGraph->RunInStableState();
    }
    return NS_OK;
  }
};




class CreateMessage : public ControlMessage {
public:
  CreateMessage(MediaStream* aStream) : ControlMessage(aStream) {}
  virtual void Run()
  {
    mStream->GraphImpl()->AddStream(mStream);
    mStream->Init();
  }
};

class MediaStreamGraphShutdownObserver MOZ_FINAL : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

}

void
MediaStreamGraphImpl::RunInStableState()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be called on main thread");

  nsTArray<nsCOMPtr<nsIRunnable> > runnables;
  
  
  
  nsTArray<nsAutoPtr<ControlMessage> > controlMessagesToRunDuringShutdown;

  {
    MonitorAutoLock lock(mMonitor);
    mPostedRunInStableStateEvent = false;

    runnables.SwapElements(mUpdateRunnables);
    for (PRUint32 i = 0; i < mStreamUpdates.Length(); ++i) {
      StreamUpdate* update = &mStreamUpdates[i];
      if (update->mStream) {
        ApplyStreamUpdate(update);
      }
    }
    mStreamUpdates.Clear();

    if (mLifecycleState == LIFECYCLE_WAITING_FOR_MAIN_THREAD_CLEANUP && mForceShutDown) {
      
      for (PRUint32 i = 0; i < mMessageQueue.Length(); ++i) {
        MessageBlock& mb = mMessageQueue[i];
        controlMessagesToRunDuringShutdown.MoveElementsFrom(mb.mMessages);
      }
      mMessageQueue.Clear();
      controlMessagesToRunDuringShutdown.MoveElementsFrom(mCurrentTaskMessageQueue);
      
      
      mLifecycleState = LIFECYCLE_WAITING_FOR_THREAD_SHUTDOWN;
      nsCOMPtr<nsIRunnable> event = new MediaStreamGraphShutDownRunnable(this);
      NS_DispatchToMainThread(event);
    }

    if (mLifecycleState == LIFECYCLE_THREAD_NOT_STARTED) {
      mLifecycleState = LIFECYCLE_RUNNING;
      
      
      
      nsCOMPtr<nsIRunnable> event = new MediaStreamGraphThreadRunnable();
      NS_NewThread(getter_AddRefs(mThread), event);
    }

    if (mCurrentTaskMessageQueue.IsEmpty()) {
      if (mLifecycleState == LIFECYCLE_WAITING_FOR_MAIN_THREAD_CLEANUP && IsEmpty()) {
        NS_ASSERTION(gGraph == this, "Not current graph??");
        
        
        LOG(PR_LOG_DEBUG, ("Disconnecting MediaStreamGraph %p", gGraph));
        gGraph = nullptr;
        
        
        
        mLifecycleState = LIFECYCLE_WAITING_FOR_THREAD_SHUTDOWN;
        nsCOMPtr<nsIRunnable> event = new MediaStreamGraphShutDownRunnable(this);
        NS_DispatchToMainThread(event);
      }
    } else {
      if (mLifecycleState <= LIFECYCLE_WAITING_FOR_MAIN_THREAD_CLEANUP) {
        MessageBlock* block = mMessageQueue.AppendElement();
        block->mMessages.SwapElements(mCurrentTaskMessageQueue);
        block->mGraphUpdateIndex = mGraphUpdatesSent;
        ++mGraphUpdatesSent;
        EnsureNextIterationLocked(lock);
      }

      if (mLifecycleState == LIFECYCLE_WAITING_FOR_MAIN_THREAD_CLEANUP) {
        mLifecycleState = LIFECYCLE_RUNNING;
        
        
        
        nsCOMPtr<nsIRunnable> event = new MediaStreamGraphThreadRunnable();
        mThread->Dispatch(event, 0);
      }
    }

    mDetectedNotRunning = mLifecycleState > LIFECYCLE_RUNNING;
  }

  
  mPostedRunInStableState = false;

  for (PRUint32 i = 0; i < runnables.Length(); ++i) {
    runnables[i]->Run();
  }
  for (PRUint32 i = 0; i < controlMessagesToRunDuringShutdown.Length(); ++i) {
    controlMessagesToRunDuringShutdown[i]->RunDuringShutdown();
  }
}

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

void
MediaStreamGraphImpl::EnsureRunInStableState()
{
  NS_ASSERTION(NS_IsMainThread(), "main thread only");

  if (mPostedRunInStableState)
    return;
  mPostedRunInStableState = true;
  nsCOMPtr<nsIRunnable> event = new MediaStreamGraphStableStateRunnable();
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->RunInStableState(event);
  } else {
    NS_ERROR("Appshell already destroyed?");
  }
}

void
MediaStreamGraphImpl::EnsureStableStateEventPosted()
{
  mMonitor.AssertCurrentThreadOwns();

  if (mPostedRunInStableStateEvent)
    return;
  mPostedRunInStableStateEvent = true;
  nsCOMPtr<nsIRunnable> event = new MediaStreamGraphStableStateRunnable();
  NS_DispatchToMainThread(event);
}

void
MediaStreamGraphImpl::AppendMessage(ControlMessage* aMessage)
{
  NS_ASSERTION(NS_IsMainThread(), "main thread only");
  NS_ASSERTION(!aMessage->GetStream() ||
               !aMessage->GetStream()->IsDestroyed(),
               "Stream already destroyed");

  if (mDetectedNotRunning &&
      mLifecycleState > LIFECYCLE_WAITING_FOR_MAIN_THREAD_CLEANUP) {
    
    
    
    
    
    aMessage->RunDuringShutdown();
    delete aMessage;
    if (IsEmpty()) {
      NS_ASSERTION(gGraph == this, "Switched managers during forced shutdown?");
      gGraph = nullptr;
      delete this;
    }
    return;
  }

  mCurrentTaskMessageQueue.AppendElement(aMessage);
  EnsureRunInStableState();
}

void
MediaStream::Init()
{
  MediaStreamGraphImpl* graph = GraphImpl();
  mBlocked.SetAtAndAfter(graph->mCurrentTime, true);
  mExplicitBlockerCount.SetAtAndAfter(graph->mCurrentTime, true);
  mExplicitBlockerCount.SetAtAndAfter(graph->mStateComputedTime, false);
}

MediaStreamGraphImpl*
MediaStream::GraphImpl()
{
  return gGraph;
}

void
MediaStream::DestroyImpl()
{
  if (mAudioOutput) {
    mAudioOutput->Shutdown();
    mAudioOutput = nullptr;
  }
}

void
MediaStream::Destroy()
{
  class Message : public ControlMessage {
  public:
    Message(MediaStream* aStream) : ControlMessage(aStream) {}
    virtual void Run()
    {
      mStream->DestroyImpl();
      mStream->GraphImpl()->RemoveStream(mStream);
    }
    virtual void RunDuringShutdown()
    { Run(); }
  };
  mWrapper = nullptr;
  GraphImpl()->AppendMessage(new Message(this));
  mMainThreadDestroyed = true;
}

void
MediaStream::AddAudioOutput(void* aKey)
{
  class Message : public ControlMessage {
  public:
    Message(MediaStream* aStream, void* aKey) : ControlMessage(aStream), mKey(aKey) {}
    virtual void Run()
    {
      mStream->AddAudioOutputImpl(mKey);
    }
    void* mKey;
  };
  GraphImpl()->AppendMessage(new Message(this, aKey));
}

void
MediaStream::SetAudioOutputVolumeImpl(void* aKey, float aVolume)
{
  for (PRUint32 i = 0; i < mAudioOutputs.Length(); ++i) {
    if (mAudioOutputs[i].mKey == aKey) {
      mAudioOutputs[i].mVolume = aVolume;
      return;
    }
  }
  NS_ERROR("Audio output key not found");
}

void
MediaStream::SetAudioOutputVolume(void* aKey, float aVolume)
{
  class Message : public ControlMessage {
  public:
    Message(MediaStream* aStream, void* aKey, float aVolume) :
      ControlMessage(aStream), mKey(aKey), mVolume(aVolume) {}
    virtual void Run()
    {
      mStream->SetAudioOutputVolumeImpl(mKey, mVolume);
    }
    void* mKey;
    float mVolume;
  };
  GraphImpl()->AppendMessage(new Message(this, aKey, aVolume));
}

void
MediaStream::RemoveAudioOutputImpl(void* aKey)
{
  for (PRUint32 i = 0; i < mAudioOutputs.Length(); ++i) {
    if (mAudioOutputs[i].mKey == aKey) {
      mAudioOutputs.RemoveElementAt(i);
      return;
    }
  }
  NS_ERROR("Audio output key not found");
}

void
MediaStream::RemoveAudioOutput(void* aKey)
{
  class Message : public ControlMessage {
  public:
    Message(MediaStream* aStream, void* aKey) :
      ControlMessage(aStream), mKey(aKey) {}
    virtual void Run()
    {
      mStream->RemoveAudioOutputImpl(mKey);
    }
    void* mKey;
  };
  GraphImpl()->AppendMessage(new Message(this, aKey));
}

void
MediaStream::AddVideoOutput(VideoFrameContainer* aContainer)
{
  class Message : public ControlMessage {
  public:
    Message(MediaStream* aStream, VideoFrameContainer* aContainer) :
      ControlMessage(aStream), mContainer(aContainer) {}
    virtual void Run()
    {
      mStream->AddVideoOutputImpl(mContainer.forget());
    }
    nsRefPtr<VideoFrameContainer> mContainer;
  };
  GraphImpl()->AppendMessage(new Message(this, aContainer));
}

void
MediaStream::RemoveVideoOutput(VideoFrameContainer* aContainer)
{
  class Message : public ControlMessage {
  public:
    Message(MediaStream* aStream, VideoFrameContainer* aContainer) :
      ControlMessage(aStream), mContainer(aContainer) {}
    virtual void Run()
    {
      mStream->RemoveVideoOutputImpl(mContainer);
    }
    nsRefPtr<VideoFrameContainer> mContainer;
  };
  GraphImpl()->AppendMessage(new Message(this, aContainer));
}

void
MediaStream::ChangeExplicitBlockerCount(PRInt32 aDelta)
{
  class Message : public ControlMessage {
  public:
    Message(MediaStream* aStream, PRInt32 aDelta) :
      ControlMessage(aStream), mDelta(aDelta) {}
    virtual void Run()
    {
      mStream->ChangeExplicitBlockerCountImpl(
          mStream->GraphImpl()->mStateComputedTime, mDelta);
    }
    PRInt32 mDelta;
  };
  GraphImpl()->AppendMessage(new Message(this, aDelta));
}

void
MediaStream::AddListenerImpl(already_AddRefed<MediaStreamListener> aListener)
{
  MediaStreamListener* listener = *mListeners.AppendElement() = aListener;
  listener->NotifyBlockingChanged(GraphImpl(),
    mBlocked.GetAt(GraphImpl()->mCurrentTime) ? MediaStreamListener::BLOCKED : MediaStreamListener::UNBLOCKED);
  if (mNotifiedFinished) {
    listener->NotifyFinished(GraphImpl());
  }
}

void
MediaStream::AddListener(MediaStreamListener* aListener)
{
  class Message : public ControlMessage {
  public:
    Message(MediaStream* aStream, MediaStreamListener* aListener) :
      ControlMessage(aStream), mListener(aListener) {}
    virtual void Run()
    {
      mStream->AddListenerImpl(mListener.forget());
    }
    nsRefPtr<MediaStreamListener> mListener;
  };
  GraphImpl()->AppendMessage(new Message(this, aListener));
}

void
MediaStream::RemoveListener(MediaStreamListener* aListener)
{
  class Message : public ControlMessage {
  public:
    Message(MediaStream* aStream, MediaStreamListener* aListener) :
      ControlMessage(aStream), mListener(aListener) {}
    virtual void Run()
    {
      mStream->RemoveListenerImpl(mListener);
    }
    nsRefPtr<MediaStreamListener> mListener;
  };
  GraphImpl()->AppendMessage(new Message(this, aListener));
}

void
SourceMediaStream::DestroyImpl()
{
  {
    MutexAutoLock lock(mMutex);
    mDestroyed = true;
  }
  MediaStream::DestroyImpl();
}

void
SourceMediaStream::SetPullEnabled(bool aEnabled)
{
  MutexAutoLock lock(mMutex);
  mPullEnabled = aEnabled;
  if (mPullEnabled && !mDestroyed) {
    GraphImpl()->EnsureNextIteration();
  }
}

void
SourceMediaStream::AddTrack(TrackID aID, TrackRate aRate, TrackTicks aStart,
                            MediaSegment* aSegment)
{
  MutexAutoLock lock(mMutex);
  TrackData* data = mUpdateTracks.AppendElement();
  data->mID = aID;
  data->mRate = aRate;
  data->mStart = aStart;
  data->mCommands = TRACK_CREATE;
  data->mData = aSegment;
  data->mHaveEnough = false;
  if (!mDestroyed) {
    GraphImpl()->EnsureNextIteration();
  }
}

void
SourceMediaStream::AppendToTrack(TrackID aID, MediaSegment* aSegment)
{
  MutexAutoLock lock(mMutex);
  TrackData *track = FindDataForTrack(aID);
  if (track) {
    track->mData->AppendFrom(aSegment);
  } else {
    NS_ERROR("Append to non-existent track!");
  }
  if (!mDestroyed) {
    GraphImpl()->EnsureNextIteration();
  }
}

bool
SourceMediaStream::HaveEnoughBuffered(TrackID aID)
{
  MutexAutoLock lock(mMutex);
  TrackData *track = FindDataForTrack(aID);
  if (track) {
    return track->mHaveEnough;
  }
  NS_ERROR("No track in HaveEnoughBuffered!");
  return true;
}

void
SourceMediaStream::DispatchWhenNotEnoughBuffered(TrackID aID,
    nsIThread* aSignalThread, nsIRunnable* aSignalRunnable)
{
  MutexAutoLock lock(mMutex);
  TrackData* data = FindDataForTrack(aID);
  if (!data) {
    NS_ERROR("No track in DispatchWhenNotEnoughBuffered");
    return;
  }

  if (data->mHaveEnough) {
    data->mDispatchWhenNotEnough.AppendElement()->Init(aSignalThread, aSignalRunnable);
  } else {
    aSignalThread->Dispatch(aSignalRunnable, 0);
  }
}

void
SourceMediaStream::EndTrack(TrackID aID)
{
  MutexAutoLock lock(mMutex);
  TrackData *track = FindDataForTrack(aID);
  if (track) {
    track->mCommands |= TRACK_END;
  } else {
    NS_ERROR("End of non-existant track");
  }
  if (!mDestroyed) {
    GraphImpl()->EnsureNextIteration();
  }
}

void
SourceMediaStream::AdvanceKnownTracksTime(StreamTime aKnownTime)
{
  MutexAutoLock lock(mMutex);
  mUpdateKnownTracksTime = aKnownTime;
  if (!mDestroyed) {
    GraphImpl()->EnsureNextIteration();
  }
}

void
SourceMediaStream::Finish()
{
  MutexAutoLock lock(mMutex);
  mUpdateFinished = true;
  if (!mDestroyed) {
    GraphImpl()->EnsureNextIteration();
  }
}

static const PRUint32 kThreadLimit = 4;
static const PRUint32 kIdleThreadLimit = 4;
static const PRUint32 kIdleThreadTimeoutMs = 2000;





static const PRInt32 INITIAL_CURRENT_TIME = 1;

MediaStreamGraphImpl::MediaStreamGraphImpl()
  : mCurrentTime(INITIAL_CURRENT_TIME)
  , mStateComputedTime(INITIAL_CURRENT_TIME)
  , mProcessingGraphUpdateIndex(0)
  , mMonitor("MediaStreamGraphImpl")
  , mLifecycleState(LIFECYCLE_THREAD_NOT_STARTED)
  , mWaitState(WAITSTATE_RUNNING)
  , mNeedAnotherIteration(false)
  , mForceShutDown(false)
  , mPostedRunInStableStateEvent(false)
  , mDetectedNotRunning(false)
  , mPostedRunInStableState(false)
{
#ifdef PR_LOGGING
  if (!gMediaStreamGraphLog) {
    gMediaStreamGraphLog = PR_NewLogModule("MediaStreamGraph");
  }
#endif

  mCurrentTimeStamp = mInitialTimeStamp = TimeStamp::Now();
}

NS_IMPL_ISUPPORTS1(MediaStreamGraphShutdownObserver, nsIObserver)

static bool gShutdownObserverRegistered = false;

NS_IMETHODIMP
MediaStreamGraphShutdownObserver::Observe(nsISupports *aSubject,
                                          const char *aTopic,
                                          const PRUnichar *aData)
{
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    if (gGraph) {
      gGraph->ForceShutDown();
    }
    nsContentUtils::UnregisterShutdownObserver(this);
    gShutdownObserverRegistered = false;
  }
  return NS_OK;
}

MediaStreamGraph*
MediaStreamGraph::GetInstance()
{
  NS_ASSERTION(NS_IsMainThread(), "Main thread only");

  if (!gGraph) {
    if (!gShutdownObserverRegistered) {
      gShutdownObserverRegistered = true;
      nsContentUtils::RegisterShutdownObserver(new MediaStreamGraphShutdownObserver());
    }

    gGraph = new MediaStreamGraphImpl();
    LOG(PR_LOG_DEBUG, ("Starting up MediaStreamGraph %p", gGraph));
  }

  return gGraph;
}

SourceMediaStream*
MediaStreamGraph::CreateInputStream(nsDOMMediaStream* aWrapper)
{
  SourceMediaStream* stream = new SourceMediaStream(aWrapper);
  NS_ADDREF(stream);
  static_cast<MediaStreamGraphImpl*>(this)->AppendMessage(new CreateMessage(stream));
  return stream;
}

}
