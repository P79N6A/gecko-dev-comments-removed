





#include "MediaTrack.h"
#include "MediaTrackList.h"

namespace mozilla {
namespace dom {

MediaTrack::MediaTrack(const nsAString& aId,
                       const nsAString& aKind,
                       const nsAString& aLabel,
                       const nsAString& aLanguage)
  : DOMEventTargetHelper()
  , mId(aId)
  , mKind(aKind)
  , mLabel(aLabel)
  , mLanguage(aLanguage)
{
}

MediaTrack::~MediaTrack()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(MediaTrack, DOMEventTargetHelper, mList)

NS_IMPL_ADDREF_INHERITED(MediaTrack, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaTrack, DOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaTrack)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

void
MediaTrack::SetTrackList(MediaTrackList* aList)
{
  mList = aList;
}

void
MediaTrack::Init(nsPIDOMWindow* aOwnerWindow)
{
  BindToOwner(aOwnerWindow);
}

} 
} 
