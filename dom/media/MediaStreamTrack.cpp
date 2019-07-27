




#include "MediaStreamTrack.h"

#include "DOMMediaStream.h"
#include "nsIUUIDGenerator.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {

MediaStreamTrack::MediaStreamTrack(DOMMediaStream* aStream, TrackID aTrackID)
  : mStream(aStream), mTrackID(aTrackID), mEnded(false), mEnabled(true)
{

  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidgen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);

  nsID uuid;
  memset(&uuid, 0, sizeof(uuid));
  if (uuidgen) {
    uuidgen->GenerateUUIDInPlace(&uuid);
  }

  char chars[NSID_LENGTH];
  uuid.ToProvidedString(chars);
  mID = NS_ConvertASCIItoUTF16(chars);
}

MediaStreamTrack::~MediaStreamTrack()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(MediaStreamTrack, DOMEventTargetHelper,
                                   mStream)

NS_IMPL_ADDREF_INHERITED(MediaStreamTrack, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaStreamTrack, DOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaStreamTrack)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

void
MediaStreamTrack::GetId(nsAString& aID) const
{
  aID = mID;
}

void
MediaStreamTrack::SetEnabled(bool aEnabled)
{
  mEnabled = aEnabled;
  mStream->SetTrackEnabled(mTrackID, aEnabled);
}

void
MediaStreamTrack::Stop()
{
  mStream->StopTrack(mTrackID);
}

} 
} 
