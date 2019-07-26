





#include "mozilla/dom/TextTrack.h"
#include "mozilla/dom/TextTrackBinding.h"
#include "mozilla/dom/TextTrackList.h"
#include "mozilla/dom/TextTrackCue.h"
#include "mozilla/dom/TextTrackCueList.h"
#include "mozilla/dom/TextTrackRegion.h"
#include "mozilla/dom/TextTrackRegionList.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/HTMLTrackElement.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_5(TextTrack,
                                     nsDOMEventTargetHelper,
                                     mParent,
                                     mCueList,
                                     mActiveCueList,
                                     mRegionList,
                                     mTextTrackList)

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

TextTrack::TextTrack(nsISupports* aParent,
                     TextTrackKind aKind,
                     const nsAString& aLabel,
                     const nsAString& aLanguage)
  : mParent(aParent)
{
  SetDefaultSettings();
  mKind = aKind;
  mLabel = aLabel;
  mLanguage = aLanguage;
  SetIsDOMBinding();
}

TextTrack::TextTrack(nsISupports* aParent,
                     TextTrackList* aTextTrackList,
                     TextTrackKind aKind,
                     const nsAString& aLabel,
                     const nsAString& aLanguage)
  : mParent(aParent)
  , mTextTrackList(aTextTrackList)
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
  mReadyState = HTMLTrackElement::READY_STATE_NONE;
}

JSObject*
TextTrack::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return TextTrackBinding::Wrap(aCx, aScope, this);
}

void
TextTrack::SetMode(TextTrackMode aValue)
{
  if (mMode != aValue) {
    mMode = aValue;
    if (mTextTrackList) {
      mTextTrackList->CreateAndDispatchChangeEvent();
    }
  }
}

void
TextTrack::AddCue(TextTrackCue& aCue)
{
  mCueList->AddCue(aCue);
  if (mTextTrackList) {
    HTMLMediaElement* mediaElement = mTextTrackList->GetMediaElement();
    if (mediaElement) {
      mediaElement->AddCue(aCue);
    }
  }
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
    aRegion.SetTextTrack(this);
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

void
TextTrack::UpdateActiveCueList()
{
  if (!mTextTrackList) {
    return;
  }

  HTMLMediaElement* mediaElement = mTextTrackList->GetMediaElement();
  if (!mediaElement) {
    return;
  }

  
  
  
  if (mDirty) {
    mCuePos = 0;
    mDirty = false;
    mActiveCueList->RemoveAll();
  }

  double playbackTime = mediaElement->CurrentTime();
  
  
  for (uint32_t i = mActiveCueList->Length(); i > 0; i--) {
    if ((*mActiveCueList)[i - 1]->EndTime() < playbackTime) {
      mActiveCueList->RemoveCueAt(i - 1);
    }
  }
  
  
  
  
  for (; mCuePos < mCueList->Length() &&
         (*mCueList)[mCuePos]->StartTime() <= playbackTime; mCuePos++) {
    if ((*mCueList)[mCuePos]->EndTime() >= playbackTime) {
      mActiveCueList->AddCue(*(*mCueList)[mCuePos]);
    }
  }
}

TextTrackCueList*
TextTrack::GetActiveCues() {
  if (mMode != TextTrackMode::Disabled) {
    UpdateActiveCueList();
    return mActiveCueList;
  }
  return nullptr;
}

void
TextTrack::GetActiveCueArray(nsTArray<nsRefPtr<TextTrackCue> >& aCues)
{
  if (mMode != TextTrackMode::Disabled) {
    UpdateActiveCueList();
    mActiveCueList->GetArray(aCues);
  }
}

uint16_t
TextTrack::ReadyState() const
{
  return mReadyState;
}

void
TextTrack::SetReadyState(uint16_t aState)
{
  mReadyState = aState;

  if (!mTextTrackList) {
    return;
  }

  HTMLMediaElement* mediaElement = mTextTrackList->GetMediaElement();
  if (mediaElement && (mReadyState == HTMLTrackElement::READY_STATE_LOADED ||
      mReadyState == HTMLTrackElement::READY_STATE_ERROR)) {
    mediaElement->RemoveTextTrack(this, true);
  }
}

TextTrackList*
TextTrack::GetTextTrackList()
{
  return mTextTrackList;
}

void
TextTrack::SetTextTrackList(TextTrackList* aTextTrackList)
{
  mTextTrackList = aTextTrackList;
}

} 
} 
