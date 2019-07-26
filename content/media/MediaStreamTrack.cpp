




#include "MediaStreamTrack.h"

#include "DOMMediaStream.h"
#include "nsIUUIDGenerator.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {

MediaStreamTrack::MediaStreamTrack(DOMMediaStream* aStream, TrackID aTrackID)
  : mStream(aStream), mTrackID(aTrackID), mEnded(false)
{
  SetIsDOMBinding();

  memset(&mID, 0, sizeof(mID));

  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidgen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  if (uuidgen) {
    uuidgen->GenerateUUIDInPlace(&mID);
  }
}

MediaStreamTrack::~MediaStreamTrack()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(MediaStreamTrack, nsDOMEventTargetHelper,
                                     mStream)

NS_IMPL_ADDREF_INHERITED(MediaStreamTrack, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaStreamTrack, nsDOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaStreamTrack)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

void
MediaStreamTrack::GetId(nsAString& aID)
{
  char chars[NSID_LENGTH];
  mID.ToProvidedString(chars);
  aID = NS_ConvertASCIItoUTF16(chars);
}

}
}
