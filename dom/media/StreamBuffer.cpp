




#include "StreamBuffer.h"
#include "prlog.h"
#include <algorithm>

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaStreamGraphLog;
#define STREAM_LOG(type, msg) PR_LOG(gMediaStreamGraphLog, type, msg)
#else
#define STREAM_LOG(type, msg)
#endif

#ifdef DEBUG
void
StreamBuffer::DumpTrackInfo() const
{
  STREAM_LOG(PR_LOG_ALWAYS, ("DumpTracks: mTracksKnownTime %lld", mTracksKnownTime));
  for (uint32_t i = 0; i < mTracks.Length(); ++i) {
    Track* track = mTracks[i];
    if (track->IsEnded()) {
      STREAM_LOG(PR_LOG_ALWAYS, ("Track[%d] %d: ended", i, track->GetID()));
    } else {
      STREAM_LOG(PR_LOG_ALWAYS, ("Track[%d] %d: %lld", i, track->GetID(),
                                 track->GetEnd()));
    }
  }
}
#endif

StreamTime
StreamBuffer::GetEnd() const
{
  StreamTime t = mTracksKnownTime;
  for (uint32_t i = 0; i < mTracks.Length(); ++i) {
    Track* track = mTracks[i];
    if (!track->IsEnded()) {
      t = std::min(t, track->GetEnd());
    }
  }
  return t;
}

StreamTime
StreamBuffer::GetAllTracksEnd() const
{
  if (mTracksKnownTime < STREAM_TIME_MAX) {
    
    return STREAM_TIME_MAX;
  }
  StreamTime t = 0;
  for (uint32_t i = 0; i < mTracks.Length(); ++i) {
    Track* track = mTracks[i];
    if (!track->IsEnded()) {
      return STREAM_TIME_MAX;
    }
    t = std::max(t, track->GetEnd());
  }
  return t;
}

StreamBuffer::Track*
StreamBuffer::FindTrack(TrackID aID)
{
  if (aID == TRACK_NONE)
    return nullptr;
  for (uint32_t i = 0; i < mTracks.Length(); ++i) {
    Track* track = mTracks[i];
    if (track->GetID() == aID) {
      return track;
    }
  }
  return nullptr;
}

void
StreamBuffer::ForgetUpTo(StreamTime aTime)
{
  
  
  const StreamTime minChunkSize = 2400;
  if (aTime < mForgottenTime + minChunkSize) {
    return;
  }
  mForgottenTime = aTime;

  for (uint32_t i = 0; i < mTracks.Length(); ++i) {
    Track* track = mTracks[i];
    if (track->IsEnded() && track->GetEnd() <= aTime) {
      mTracks.RemoveElementAt(i);
      --i;
      continue;
    }
    StreamTime forgetTo = std::min(track->GetEnd() - 1, aTime);
    track->ForgetUpTo(forgetTo);
  }
}

}
