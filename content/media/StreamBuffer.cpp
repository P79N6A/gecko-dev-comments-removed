




#include "StreamBuffer.h"

namespace mozilla {

StreamTime
StreamBuffer::GetEnd() const
{
  StreamTime t = mTracksKnownTime;
  for (PRUint32 i = 0; i < mTracks.Length(); ++i) {
    Track* track = mTracks[i];
    if (!track->IsEnded()) {
      t = NS_MIN(t, track->GetEndTimeRoundDown());
    }
  }
  return t;
}

StreamBuffer::Track*
StreamBuffer::FindTrack(TrackID aID)
{
  if (aID == TRACK_NONE)
    return nullptr;
  for (PRUint32 i = 0; i < mTracks.Length(); ++i) {
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
  
  const MediaTime roundTo = MillisecondsToMediaTime(50);
  StreamTime forget = (aTime/roundTo)*roundTo;
  if (forget <= mForgottenTime) {
    return;
  }
  mForgottenTime = forget;

  for (PRUint32 i = 0; i < mTracks.Length(); ++i) {
    Track* track = mTracks[i];
    if (track->IsEnded() && track->GetEndTimeRoundDown() <= forget) {
      mTracks.RemoveElementAt(i);
      --i;
      continue;
    }
    TrackTicks forgetTo = NS_MIN(track->GetEnd() - 1, track->TimeToTicksRoundDown(forget));
    track->ForgetUpTo(forgetTo);
  }
}

}
