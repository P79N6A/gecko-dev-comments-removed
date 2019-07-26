





#include "mozilla/dom/TextTrack.h"
#include "mozilla/dom/TextTrackBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_3(TextTrack,
                                     nsDOMEventTargetHelper,
                                     mParent,
                                     mCueList,
                                     mActiveCueList)

NS_IMPL_ADDREF_INHERITED(TextTrack, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TextTrack, nsDOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TextTrack)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

TextTrack::TextTrack(nsISupports* aParent,
                     TextTrackKind aKind,
                     const nsAString& aLabel,
                     const nsAString& aLanguage)
  : mParent(aParent)
  , mKind(aKind)
  , mLabel(aLabel)
  , mLanguage(aLanguage)
  , mMode(TextTrackMode::Hidden)
  , mCueList(new TextTrackCueList(aParent))
  , mActiveCueList(new TextTrackCueList(aParent))
{
  SetIsDOMBinding();
}

TextTrack::TextTrack(nsISupports* aParent)
  : mParent(aParent)
  , mKind(TextTrackKind::Subtitles)
  , mMode(TextTrackMode::Disabled)
  , mCueList(new TextTrackCueList(aParent))
  , mActiveCueList(new TextTrackCueList(aParent))
{
  SetIsDOMBinding();
}

void
TextTrack::Update(double aTime)
{
  mCueList->Update(aTime);
}

JSObject*
TextTrack::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return TextTrackBinding::Wrap(aCx, aScope, this);
}

void
TextTrack::SetMode(TextTrackMode aValue)
{
  mMode = aValue;
}

void
TextTrack::AddCue(TextTrackCue& aCue)
{
  
  mCueList->AddCue(aCue);
}

void
TextTrack::RemoveCue(TextTrackCue& aCue)
{
  
  mCueList->RemoveCue(aCue);
}

void
TextTrack::CueChanged(TextTrackCue& aCue)
{
  
}

} 
} 
