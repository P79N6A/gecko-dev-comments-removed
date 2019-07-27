




#include "MediaStreamGraphImpl.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/unused.h"

#include "AudioSegment.h"
#include "VideoSegment.h"
#include "nsContentUtils.h"
#include "nsIAppShell.h"
#include "nsIObserver.h"
#include "nsPrintfCString.h"
#include "nsServiceManagerUtils.h"
#include "nsWidgetsCID.h"
#include "prerror.h"
#include "prlog.h"
#include "mozilla/Attributes.h"
#include "TrackUnionStream.h"
#include "ImageContainer.h"
#include "AudioChannelService.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "AudioNodeExternalInputStream.h"
#include "webaudio/MediaStreamAudioDestinationNode.h"
#include <algorithm>
#include "DOMMediaStream.h"
#include "GeckoProfiler.h"
#ifdef MOZ_WEBRTC
#include "AudioOutputObserver.h"
#endif

using namespace mozilla::layers;
using namespace mozilla::dom;
using namespace mozilla::gfx;

namespace mozilla {

#ifdef STREAM_LOG
#undef STREAM_LOG
#endif

#ifdef PR_LOGGING
PRLogModuleInfo* gTrackUnionStreamLog;
#define STREAM_LOG(type, msg) PR_LOG(gTrackUnionStreamLog, type, msg)
#else
#define STREAM_LOG(type, msg)
#endif

TrackUnionStream::TrackUnionStream(DOMMediaStream* aWrapper) :
  ProcessedMediaStream(aWrapper)
{
#ifdef PR_LOGGING
  if (!gTrackUnionStreamLog) {
    gTrackUnionStreamLog = PR_NewLogModule("TrackUnionStream");
  }
#endif
}

  void TrackUnionStream::RemoveInput(MediaInputPort* aPort)
  {
    for (int32_t i = mTrackMap.Length() - 1; i >= 0; --i) {
      if (mTrackMap[i].mInputPort == aPort) {
        EndTrack(i);
        mTrackMap.RemoveElementAt(i);
      }
    }
    ProcessedMediaStream::RemoveInput(aPort);
  }
  void TrackUnionStream::ProcessInput(GraphTime aFrom, GraphTime aTo, uint32_t aFlags)
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
    bool allFinished = !mInputs.IsEmpty();
    bool allHaveCurrentData = !mInputs.IsEmpty();
    for (uint32_t i = 0; i < mInputs.Length(); ++i) {
      MediaStream* stream = mInputs[i]->GetSource();
      if (!stream->IsFinishedOnGraphThread()) {
        
        
        
        allFinished = false;
      }
      if (!stream->HasCurrentData()) {
        allHaveCurrentData = false;
      }
      bool trackAdded = false;
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
        if (!found) {
          bool trackFinished = false;
          trackAdded = true;
          uint32_t mapIndex = AddTrack(mInputs[i], tracks.get(), aFrom);
          CopyTrackData(tracks.get(), mapIndex, aFrom, aTo, &trackFinished);
          mappedTracksFinished.AppendElement(trackFinished);
          mappedTracksWithMatchingInputTracks.AppendElement(true);
        }
      }
      if (trackAdded) {
        for (MediaStreamListener* l : mListeners) {
          l->NotifyFinishedTrackCreation(Graph());
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

  
  
  void TrackUnionStream::ForwardTrackEnabled(TrackID aOutputID, bool aEnabled)
  {
    for (int32_t i = mTrackMap.Length() - 1; i >= 0; --i) {
      if (mTrackMap[i].mOutputTrackID == aOutputID) {
        mTrackMap[i].mInputPort->GetSource()->
          SetTrackEnabled(mTrackMap[i].mInputTrackID, aEnabled);
      }
    }
  }

  uint32_t TrackUnionStream::AddTrack(MediaInputPort* aPort, StreamBuffer::Track* aTrack,
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

    
    
    
    StreamTime outputStart = GraphTimeToStreamTime(aFrom);

    nsAutoPtr<MediaSegment> segment;
    segment = aTrack->GetSegment()->CreateEmptyClone();
    for (uint32_t j = 0; j < mListeners.Length(); ++j) {
      MediaStreamListener* l = mListeners[j];
      l->NotifyQueuedTrackChanges(Graph(), id, outputStart,
                                  MediaStreamListener::TRACK_EVENT_CREATED,
                                  *segment);
    }
    segment->AppendNullData(outputStart);
    StreamBuffer::Track* track =
      &mBuffer.AddTrack(id, outputStart, segment.forget());
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

  void TrackUnionStream::EndTrack(uint32_t aIndex)
  {
    StreamBuffer::Track* outputTrack = mBuffer.FindTrack(mTrackMap[aIndex].mOutputTrackID);
    if (!outputTrack || outputTrack->IsEnded())
      return;
    for (uint32_t j = 0; j < mListeners.Length(); ++j) {
      MediaStreamListener* l = mListeners[j];
      StreamTime offset = outputTrack->GetSegment()->GetDuration();
      nsAutoPtr<MediaSegment> segment;
      segment = outputTrack->GetSegment()->CreateEmptyClone();
      l->NotifyQueuedTrackChanges(Graph(), outputTrack->GetID(), offset,
                                  MediaStreamListener::TRACK_EVENT_ENDED,
                                  *segment);
    }
    outputTrack->SetEnded();
  }

  void TrackUnionStream::CopyTrackData(StreamBuffer::Track* aInputTrack,
                     uint32_t aMapIndex, GraphTime aFrom, GraphTime aTo,
                     bool* aOutputTrackFinished)
  {
    TrackMapEntry* map = &mTrackMap[aMapIndex];
    StreamBuffer::Track* outputTrack = mBuffer.FindTrack(map->mOutputTrackID);
    MOZ_ASSERT(outputTrack && !outputTrack->IsEnded(), "Can't copy to ended track");

    MediaSegment* segment = map->mSegment;
    MediaStream* source = map->mInputPort->GetSource();

    GraphTime next;
    *aOutputTrackFinished = false;
    for (GraphTime t = aFrom; t < aTo; t = next) {
      MediaInputPort::InputInterval interval = map->mInputPort->GetNextInputInterval(t);
      interval.mEnd = std::min(interval.mEnd, aTo);
      StreamTime inputEnd = source->GraphTimeToStreamTime(interval.mEnd);
      StreamTime inputTrackEndPoint = STREAM_TIME_MAX;

      if (aInputTrack->IsEnded() &&
          aInputTrack->GetEnd() <= inputEnd) {
        inputTrackEndPoint = aInputTrack->GetEnd();
        *aOutputTrackFinished = true;
      }

      if (interval.mStart >= interval.mEnd) {
        break;
      }
      StreamTime ticks = interval.mEnd - interval.mStart;
      next = interval.mEnd;

      StreamTime outputStart = outputTrack->GetEnd();

      if (interval.mInputIsBlocked) {
        
        segment->AppendNullData(ticks);
        STREAM_LOG(PR_LOG_DEBUG+1, ("TrackUnionStream %p appending %lld ticks of null data to track %d",
                   this, (long long)ticks, outputTrack->GetID()));
      } else if (InMutedCycle()) {
        segment->AppendNullData(ticks);
      } else {
        if (GraphImpl()->StreamSuspended(source)) {
          segment->AppendNullData(aTo - aFrom);
        } else {
          MOZ_ASSERT(outputTrack->GetEnd() == GraphTimeToStreamTime(interval.mStart),
                     "Samples missing");
          StreamTime inputStart = source->GraphTimeToStreamTime(interval.mStart);
          segment->AppendSlice(*aInputTrack->GetSegment(),
                               std::min(inputTrackEndPoint, inputStart),
                               std::min(inputTrackEndPoint, inputEnd));
        }
      }
      ApplyTrackDisabling(outputTrack->GetID(), segment);
      for (uint32_t j = 0; j < mListeners.Length(); ++j) {
        MediaStreamListener* l = mListeners[j];
        l->NotifyQueuedTrackChanges(Graph(), outputTrack->GetID(),
                                    outputStart, 0, *segment);
      }
      outputTrack->GetSegment()->AppendFrom(segment);
    }
  }
}
