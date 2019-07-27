




#ifndef MOZILLA_TRACKUNIONSTREAM_H_
#define MOZILLA_TRACKUNIONSTREAM_H_

#include "MediaStreamGraph.h"
#include <algorithm>

namespace mozilla {

#ifdef PR_LOGGING
#define STREAM_LOG(type, msg) PR_LOG(gMediaStreamGraphLog, type, msg)
#else
#define STREAM_LOG(type, msg)
#endif






class TrackUnionStream : public ProcessedMediaStream {
public:
  explicit TrackUnionStream(DOMMediaStream* aWrapper) :
    ProcessedMediaStream(aWrapper),
    mFilterCallback(nullptr)
  {}

  virtual void RemoveInput(MediaInputPort* aPort) MOZ_OVERRIDE
  {
    for (int32_t i = mTrackMap.Length() - 1; i >= 0; --i) {
      if (mTrackMap[i].mInputPort == aPort) {
        EndTrack(i);
        mTrackMap.RemoveElementAt(i);
      }
    }
    ProcessedMediaStream::RemoveInput(aPort);
  }
  virtual void ProcessInput(GraphTime aFrom, GraphTime aTo, uint32_t aFlags) MOZ_OVERRIDE
  {
    if (IsFinishedOnGraphThread()) {
      return;
    }
    nsAutoTArray<bool,8> mappedTracksFinished;
    nsAutoTArray<bool,8> mappedTracksWithMatchingInputTracks;
    for (uint32_t i = 0; i < mTrackMap.Length(); ++i) {
      mappedTracksFinished.AppendElement(true);
      mappedTracksWithMatchingInputTracks.AppendElement(false);
    }
    bool allFinished = true;
    bool allHaveCurrentData = true;
    for (uint32_t i = 0; i < mInputs.Length(); ++i) {
      MediaStream* stream = mInputs[i]->GetSource();
      if (!stream->IsFinishedOnGraphThread()) {
        
        
        
        allFinished = false;
      }
      if (!stream->HasCurrentData()) {
        allHaveCurrentData = false;
      }
      for (StreamBuffer::TrackIter tracks(stream->GetStreamBuffer());
           !tracks.IsEnded(); tracks.Next()) {
        bool found = false;
        for (uint32_t j = 0; j < mTrackMap.Length(); ++j) {
          TrackMapEntry* map = &mTrackMap[j];
          if (map->mInputPort == mInputs[i] && map->mInputTrackID == tracks->GetID()) {
            bool trackFinished;
            StreamBuffer::Track* outputTrack = mBuffer.FindTrack(map->mOutputTrackID);
            if (!outputTrack || outputTrack->IsEnded()) {
              trackFinished = true;
            } else {
              CopyTrackData(tracks.get(), j, aFrom, aTo, &trackFinished);
            }
            mappedTracksFinished[j] = trackFinished;
            mappedTracksWithMatchingInputTracks[j] = true;
            found = true;
            break;
          }
        }
        if (!found && (!mFilterCallback || mFilterCallback(tracks.get()))) {
          bool trackFinished = false;
          uint32_t mapIndex = AddTrack(mInputs[i], tracks.get(), aFrom);
          CopyTrackData(tracks.get(), mapIndex, aFrom, aTo, &trackFinished);
          mappedTracksFinished.AppendElement(trackFinished);
          mappedTracksWithMatchingInputTracks.AppendElement(true);
        }
      }
    }
    for (int32_t i = mTrackMap.Length() - 1; i >= 0; --i) {
      if (mappedTracksFinished[i]) {
        EndTrack(i);
      } else {
        allFinished = false;
      }
      if (!mappedTracksWithMatchingInputTracks[i]) {
        mTrackMap.RemoveElementAt(i);
      }
    }
    if (allFinished && mAutofinish && (aFlags & ALLOW_FINISH)) {
      
      
      
      FinishOnGraphThread();
    } else {
      mBuffer.AdvanceKnownTracksTime(GraphTimeToStreamTime(aTo));
    }
    if (allHaveCurrentData) {
      
      mHasCurrentData = true;
    }
  }

  
  
  typedef bool (*TrackIDFilterCallback)(StreamBuffer::Track*);
  void SetTrackIDFilter(TrackIDFilterCallback aCallback) {
    mFilterCallback = aCallback;
  }

  
  
  virtual void ForwardTrackEnabled(TrackID aOutputID, bool aEnabled) MOZ_OVERRIDE {
    for (int32_t i = mTrackMap.Length() - 1; i >= 0; --i) {
      if (mTrackMap[i].mOutputTrackID == aOutputID) {
        mTrackMap[i].mInputPort->GetSource()->
          SetTrackEnabled(mTrackMap[i].mInputTrackID, aEnabled);
      }
    }
  }

protected:
  TrackIDFilterCallback mFilterCallback;

  
  struct TrackMapEntry {
    
    
    TrackTicks mEndOfConsumedInputTicks;
    
    
    
    StreamTime mEndOfLastInputIntervalInInputStream;
    
    
    
    StreamTime mEndOfLastInputIntervalInOutputStream;
    MediaInputPort* mInputPort;
    
    
    
    
    
    TrackID mInputTrackID;
    TrackID mOutputTrackID;
    nsAutoPtr<MediaSegment> mSegment;
  };

  uint32_t AddTrack(MediaInputPort* aPort, StreamBuffer::Track* aTrack,
                    GraphTime aFrom)
  {
    
    
    TrackID id = aTrack->GetID();
    TrackID maxTrackID = 0;
    for (uint32_t i = 0; i < mTrackMap.Length(); ++i) {
      TrackID outID = mTrackMap[i].mOutputTrackID;
      maxTrackID = std::max(maxTrackID, outID);
    }
    
    
    
    while (1) {
      
      if (!mBuffer.FindTrack(id)) {
        break;
      }
      id = ++maxTrackID;
    }

    TrackRate rate = aTrack->GetRate();
    
    
    
    TrackTicks outputStart = TimeToTicksRoundUp(rate, GraphTimeToStreamTime(aFrom));

    nsAutoPtr<MediaSegment> segment;
    segment = aTrack->GetSegment()->CreateEmptyClone();
    for (uint32_t j = 0; j < mListeners.Length(); ++j) {
      MediaStreamListener* l = mListeners[j];
      l->NotifyQueuedTrackChanges(Graph(), id, rate, outputStart,
                                  MediaStreamListener::TRACK_EVENT_CREATED,
                                  *segment);
    }
    segment->AppendNullData(outputStart);
    StreamBuffer::Track* track =
      &mBuffer.AddTrack(id, rate, outputStart, segment.forget());
    STREAM_LOG(PR_LOG_DEBUG, ("TrackUnionStream %p adding track %d for input stream %p track %d, start ticks %lld",
                              this, id, aPort->GetSource(), aTrack->GetID(),
                              (long long)outputStart));

    TrackMapEntry* map = mTrackMap.AppendElement();
    map->mEndOfConsumedInputTicks = 0;
    map->mEndOfLastInputIntervalInInputStream = -1;
    map->mEndOfLastInputIntervalInOutputStream = -1;
    map->mInputPort = aPort;
    map->mInputTrackID = aTrack->GetID();
    map->mOutputTrackID = track->GetID();
    map->mSegment = aTrack->GetSegment()->CreateEmptyClone();
    return mTrackMap.Length() - 1;
  }
  void EndTrack(uint32_t aIndex)
  {
    StreamBuffer::Track* outputTrack = mBuffer.FindTrack(mTrackMap[aIndex].mOutputTrackID);
    if (!outputTrack || outputTrack->IsEnded())
      return;
    for (uint32_t j = 0; j < mListeners.Length(); ++j) {
      MediaStreamListener* l = mListeners[j];
      TrackTicks offset = outputTrack->GetSegment()->GetDuration();
      nsAutoPtr<MediaSegment> segment;
      segment = outputTrack->GetSegment()->CreateEmptyClone();
      l->NotifyQueuedTrackChanges(Graph(), outputTrack->GetID(),
                                  outputTrack->GetRate(), offset,
                                  MediaStreamListener::TRACK_EVENT_ENDED,
                                  *segment);
    }
    outputTrack->SetEnded();
  }
  void CopyTrackData(StreamBuffer::Track* aInputTrack,
                     uint32_t aMapIndex, GraphTime aFrom, GraphTime aTo,
                     bool* aOutputTrackFinished)
  {
    TrackMapEntry* map = &mTrackMap[aMapIndex];
    StreamBuffer::Track* outputTrack = mBuffer.FindTrack(map->mOutputTrackID);
    MOZ_ASSERT(outputTrack && !outputTrack->IsEnded(), "Can't copy to ended track");

    TrackRate rate = outputTrack->GetRate();
    MediaSegment* segment = map->mSegment;
    MediaStream* source = map->mInputPort->GetSource();

    GraphTime next;
    *aOutputTrackFinished = false;
    for (GraphTime t = aFrom; t < aTo; t = next) {
      MediaInputPort::InputInterval interval = map->mInputPort->GetNextInputInterval(t);
      interval.mEnd = std::min(interval.mEnd, aTo);
      StreamTime inputEnd = source->GraphTimeToStreamTime(interval.mEnd);
      TrackTicks inputTrackEndPoint = TRACK_TICKS_MAX;

      if (aInputTrack->IsEnded() &&
          aInputTrack->GetEndTimeRoundDown() <= inputEnd) {
        inputTrackEndPoint = aInputTrack->GetEnd();
        *aOutputTrackFinished = true;
      }

      if (interval.mStart >= interval.mEnd)
        break;
      next = interval.mEnd;

      
      StreamTime outputEnd = GraphTimeToStreamTime(interval.mEnd);
      TrackTicks startTicks = outputTrack->GetEnd();
      StreamTime outputStart = GraphTimeToStreamTime(interval.mStart);
      MOZ_ASSERT(startTicks == TimeToTicksRoundUp(rate, outputStart), "Samples missing");
      TrackTicks endTicks = TimeToTicksRoundUp(rate, outputEnd);
      TrackTicks ticks = endTicks - startTicks;
      StreamTime inputStart = source->GraphTimeToStreamTime(interval.mStart);

      if (interval.mInputIsBlocked) {
        
        segment->AppendNullData(ticks);
        STREAM_LOG(PR_LOG_DEBUG+1, ("TrackUnionStream %p appending %lld ticks of null data to track %d",
                   this, (long long)ticks, outputTrack->GetID()));
      } else {
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (map->mEndOfLastInputIntervalInInputStream != inputStart ||
            map->mEndOfLastInputIntervalInOutputStream != outputStart) {
          
          map->mEndOfConsumedInputTicks = TimeToTicksRoundDown(rate, inputStart) - 1;
        }
        TrackTicks inputStartTicks = map->mEndOfConsumedInputTicks;
        TrackTicks inputEndTicks = inputStartTicks + ticks;
        map->mEndOfConsumedInputTicks = inputEndTicks;
        map->mEndOfLastInputIntervalInInputStream = inputEnd;
        map->mEndOfLastInputIntervalInOutputStream = outputEnd;
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        if (inputStartTicks < 0) {
          
          
          
          
          
          
          segment->AppendNullData(-inputStartTicks);
          inputStartTicks = 0;
        }
        if (inputEndTicks > inputStartTicks) {
          segment->AppendSlice(*aInputTrack->GetSegment(),
                               std::min(inputTrackEndPoint, inputStartTicks),
                               std::min(inputTrackEndPoint, inputEndTicks));
        }
        STREAM_LOG(PR_LOG_DEBUG+1, ("TrackUnionStream %p appending %lld ticks of input data to track %d",
                   this, (long long)(std::min(inputTrackEndPoint, inputEndTicks) - std::min(inputTrackEndPoint, inputStartTicks)),
                   outputTrack->GetID()));
      }
      ApplyTrackDisabling(outputTrack->GetID(), segment);
      for (uint32_t j = 0; j < mListeners.Length(); ++j) {
        MediaStreamListener* l = mListeners[j];
        l->NotifyQueuedTrackChanges(Graph(), outputTrack->GetID(),
                                    outputTrack->GetRate(), startTicks, 0,
                                    *segment);
      }
      outputTrack->GetSegment()->AppendFrom(segment);
    }
  }

  nsTArray<TrackMapEntry> mTrackMap;
};

}

#endif 
