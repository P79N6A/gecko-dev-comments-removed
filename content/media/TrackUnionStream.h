




#ifndef MOZILLA_TRACKUNIONSTREAM_H_
#define MOZILLA_TRACKUNIONSTREAM_H_

#include "MediaStreamGraph.h"

namespace mozilla {

#ifdef PR_LOGGING
#define LOG(type, msg) PR_LOG(gMediaStreamGraphLog, type, msg)
#else
#define LOG(type, msg)
#endif






class TrackUnionStream : public ProcessedMediaStream {
public:
  TrackUnionStream(nsDOMMediaStream* aWrapper) :
    ProcessedMediaStream(aWrapper),
    mMaxTrackID(0) {}

  virtual void RemoveInput(MediaInputPort* aPort)
  {
    for (PRInt32 i = mTrackMap.Length() - 1; i >= 0; --i) {
      if (mTrackMap[i].mInputPort == aPort) {
        EndTrack(i);
        mTrackMap.RemoveElementAt(i);
      }
    }
    ProcessedMediaStream::RemoveInput(aPort);
  }
  virtual void ProduceOutput(GraphTime aFrom, GraphTime aTo)
  {
    nsAutoTArray<bool,8> mappedTracksFinished;
    nsAutoTArray<bool,8> mappedTracksWithMatchingInputTracks;
    for (PRUint32 i = 0; i < mTrackMap.Length(); ++i) {
      mappedTracksFinished.AppendElement(true);
      mappedTracksWithMatchingInputTracks.AppendElement(false);
    }
    bool allFinished = true;
    for (PRUint32 i = 0; i < mInputs.Length(); ++i) {
      MediaStream* stream = mInputs[i]->GetSource();
      if (!stream->IsFinishedOnGraphThread()) {
        allFinished = false;
      }
      for (StreamBuffer::TrackIter tracks(stream->GetStreamBuffer());
           !tracks.IsEnded(); tracks.Next()) {
        bool found = false;
        for (PRUint32 j = 0; j < mTrackMap.Length(); ++j) {
          TrackMapEntry* map = &mTrackMap[j];
          if (map->mInputPort == mInputs[i] && map->mInputTrack == tracks.get()) {
            bool trackFinished;
            if (map->mOutputTrack->IsEnded()) {
              trackFinished = true;
            } else {
              CopyTrackData(j, aFrom, aTo, &trackFinished);
            }
            mappedTracksFinished[j] = trackFinished;
            mappedTracksWithMatchingInputTracks[j] = true;
            found = true;
            break;
          }
        }
        if (!found) {
          bool trackFinished = false;
          PRUint32 mapIndex = AddTrack(mInputs[i], tracks.get(), aFrom);
          CopyTrackData(mapIndex, aFrom, aTo, &trackFinished);
          mappedTracksFinished.AppendElement(trackFinished);
          mappedTracksWithMatchingInputTracks.AppendElement(true);
        }
      }
    }
    for (PRInt32 i = mTrackMap.Length() - 1; i >= 0; --i) {
      if (mappedTracksFinished[i]) {
        EndTrack(i);
      } else {
        allFinished = false;
      }
      if (!mappedTracksWithMatchingInputTracks[i]) {
        mTrackMap.RemoveElementAt(i);
      }
    }
    if (allFinished && mAutofinish) {
      
      
      
      FinishOnGraphThread();
    }
    mBuffer.AdvanceKnownTracksTime(GraphTimeToStreamTime(aTo));
  }

protected:
  
  struct TrackMapEntry {
    MediaInputPort* mInputPort;
    StreamBuffer::Track* mInputTrack;
    StreamBuffer::Track* mOutputTrack;
    nsAutoPtr<MediaSegment> mSegment;
  };

  PRUint32 AddTrack(MediaInputPort* aPort, StreamBuffer::Track* aTrack,
                    GraphTime aFrom)
  {
    
    
    TrackID id = NS_MAX(mMaxTrackID + 1, aTrack->GetID());
    mMaxTrackID = id;

    TrackRate rate = aTrack->GetRate();
    
    
    
    TrackTicks outputStart = TimeToTicksRoundUp(rate, GraphTimeToStreamTime(aFrom));

    nsAutoPtr<MediaSegment> segment;
    segment = aTrack->GetSegment()->CreateEmptyClone();
    for (PRUint32 j = 0; j < mListeners.Length(); ++j) {
      MediaStreamListener* l = mListeners[j];
      l->NotifyQueuedTrackChanges(Graph(), id, rate, outputStart,
                                  MediaStreamListener::TRACK_EVENT_CREATED,
                                  *segment);
    }
    segment->AppendNullData(outputStart);
    StreamBuffer::Track* track =
      &mBuffer.AddTrack(id, rate, outputStart, segment.forget());
    LOG(PR_LOG_DEBUG, ("TrackUnionStream %p adding track %d for input stream %p track %d, start ticks %lld",
                       this, id, aPort->GetSource(), aTrack->GetID(),
                       (long long)outputStart));

    TrackMapEntry* map = mTrackMap.AppendElement();
    map->mInputPort = aPort;
    map->mInputTrack = aTrack;
    map->mOutputTrack = track;
    map->mSegment = aTrack->GetSegment()->CreateEmptyClone();
    return mTrackMap.Length() - 1;
  }
  void EndTrack(PRUint32 aIndex)
  {
    StreamBuffer::Track* outputTrack = mTrackMap[aIndex].mOutputTrack;
    if (outputTrack->IsEnded())
      return;
    for (PRUint32 j = 0; j < mListeners.Length(); ++j) {
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
  void CopyTrackData(PRUint32 aMapIndex, GraphTime aFrom, GraphTime aTo,
                     bool* aOutputTrackFinished)
  {
    TrackMapEntry* map = &mTrackMap[aMapIndex];
    StreamBuffer::Track* inputTrack = map->mInputTrack;
    StreamBuffer::Track* outputTrack = map->mOutputTrack;
    TrackRate rate = outputTrack->GetRate();
    MediaSegment* segment = map->mSegment;
    MediaStream* source = map->mInputPort->GetSource();

    NS_ASSERTION(!outputTrack->IsEnded(), "Can't copy to ended track");

    GraphTime next;
    *aOutputTrackFinished = false;
    for (GraphTime t = aFrom; t < aTo; t = next) {
      MediaInputPort::InputInterval interval = map->mInputPort->GetNextInputInterval(t);
      interval.mEnd = NS_MIN(interval.mEnd, aTo);
      if (interval.mStart >= interval.mEnd)
        break;
      next = interval.mEnd;

      
      StreamTime outputEnd = GraphTimeToStreamTime(interval.mEnd);
      TrackTicks startTicks = outputTrack->GetEnd();
#ifdef DEBUG
      StreamTime outputStart = GraphTimeToStreamTime(interval.mStart);
#endif
      NS_ASSERTION(startTicks == TimeToTicksRoundUp(rate, outputStart),
                   "Samples missing");
      TrackTicks endTicks = TimeToTicksRoundUp(rate, outputEnd);
      TrackTicks ticks = endTicks - startTicks;
      
      StreamTime inputEnd = source->GraphTimeToStreamTime(interval.mEnd);
      TrackTicks inputTrackEndPoint = TRACK_TICKS_MAX;

      if (inputTrack->IsEnded()) {
        TrackTicks inputEndTicks = inputTrack->TimeToTicksRoundDown(inputEnd);
        if (inputTrack->GetEnd() <= inputEndTicks) {
          inputTrackEndPoint = inputTrack->GetEnd();
          *aOutputTrackFinished = true;
        }
      }

      if (interval.mInputIsBlocked) {
        
        segment->AppendNullData(ticks);
        LOG(PR_LOG_DEBUG, ("TrackUnionStream %p appending %lld ticks of null data to track %d",
            this, (long long)ticks, outputTrack->GetID()));
      } else {
        
        
        
        
        
        
        
        TrackTicks inputEndTicks = TimeToTicksRoundUp(rate, inputEnd);
        TrackTicks inputStartTicks = inputEndTicks - ticks;
        segment->AppendSlice(*inputTrack->GetSegment(),
                             NS_MIN(inputTrackEndPoint, inputStartTicks),
                             NS_MIN(inputTrackEndPoint, inputEndTicks));
        LOG(PR_LOG_DEBUG, ("TrackUnionStream %p appending %lld ticks of input data to track %d",
            this, (long long)(NS_MIN(inputTrackEndPoint, inputEndTicks) - NS_MIN(inputTrackEndPoint, inputStartTicks)),
            outputTrack->GetID()));
      }
      for (PRUint32 j = 0; j < mListeners.Length(); ++j) {
        MediaStreamListener* l = mListeners[j];
        l->NotifyQueuedTrackChanges(Graph(), outputTrack->GetID(),
                                    outputTrack->GetRate(), startTicks, 0,
                                    *segment);
      }
      outputTrack->GetSegment()->AppendFrom(segment);
    }
  }

  nsTArray<TrackMapEntry> mTrackMap;
  TrackID mMaxTrackID;
};

}

#endif 
