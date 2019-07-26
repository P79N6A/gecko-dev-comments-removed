





#include "mozilla/dom/TextTrack.h"
#include "mozilla/dom/TextTrackBinding.h"
#include "mozilla/dom/TextTrackCue.h"
#include "mozilla/dom/TextTrackCueList.h"
#include "mozilla/dom/TextTrackRegion.h"
#include "mozilla/dom/TextTrackRegionList.h"
#include "mozilla/dom/HTMLMediaElement.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_5(TextTrack,
                                     nsDOMEventTargetHelper,
                                     mParent,
                                     mMediaElement,
                                     mCueList,
                                     mActiveCueList,
                                     mRegionList)

NS_IMPL_ADDREF_INHERITED(TextTrack, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TextTrack, nsDOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TextTrack)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

TextTrack::TextTrack(nsISupports* aParent)
  : mParent(aParent)
{
  SetDefaultSettings();
  SetIsDOMBinding();
}

TextTrack::TextTrack(nsISupports* aParent, HTMLMediaElement* aMediaElement)
  : mParent(aParent)
  , mMediaElement(aMediaElement)
{
  SetDefaultSettings();
  SetIsDOMBinding();
}

TextTrack::TextTrack(nsISupports* aParent,
                     HTMLMediaElement* aMediaElement,
                     TextTrackKind aKind,
                     const nsAString& aLabel,
                     const nsAString& aLanguage)
  : mParent(aParent)
  , mMediaElement(aMediaElement)
{
  SetDefaultSettings();
  mKind = aKind;
  mLabel = aLabel;
  mLanguage = aLanguage;
  SetIsDOMBinding();
}

void
TextTrack::SetDefaultSettings()
{
  mKind = TextTrackKind::Subtitles;
  mMode = TextTrackMode::Hidden;
  mCueList = new TextTrackCueList(mParent);
  mActiveCueList = new TextTrackCueList(mParent);
  mRegionList = new TextTrackRegionList(mParent);
  mCuePos = 0;
  mDirty = false;
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
  SetDirty();
}

void
TextTrack::RemoveCue(TextTrackCue& aCue, ErrorResult& aRv)
{
  mCueList->RemoveCue(aCue, aRv);
  SetDirty();
}

void
TextTrack::CueChanged(TextTrackCue& aCue)
{
  
}

void
TextTrack::AddRegion(TextTrackRegion& aRegion)
{
  TextTrackRegion* region = mRegionList->GetRegionById(aRegion.Id());
  if (!region) {
    mRegionList->AddTextTrackRegion(&aRegion);
    return;
  }

  region->CopyValues(aRegion);
}

void
TextTrack::RemoveRegion(const TextTrackRegion& aRegion, ErrorResult& aRv)
{
  if (!mRegionList->GetRegionById(aRegion.Id())) {
    aRv.Throw(NS_ERROR_DOM_NOT_FOUND_ERR);
    return;
  }

  mRegionList->RemoveTextTrackRegion(aRegion);
}

TextTrackCueList*
TextTrack::GetActiveCues()
{
  if (mMode == TextTrackMode::Disabled || !mMediaElement) {
    return nullptr;
  }

  
  
  
  if (mDirty) {
    mCuePos = 0;
    mDirty = true;
    mActiveCueList->RemoveAll();
  }

  double playbackTime = mMediaElement->CurrentTime();
  
  
  
  for (uint32_t i = 0; i < mActiveCueList->Length() &&
                       (*mActiveCueList)[i]->EndTime() < playbackTime; i++) {
    mActiveCueList->RemoveCueAt(i);
  }
  
  
  
  
  for (; mCuePos < mCueList->Length(); mCuePos++) {
    TextTrackCue* cue = (*mCueList)[mCuePos];
    if (cue->StartTime() > playbackTime || cue->EndTime() < playbackTime) {
      break;
    }
    mActiveCueList->AddCue(*cue);
  }
  return mActiveCueList;
}

} 
} 
