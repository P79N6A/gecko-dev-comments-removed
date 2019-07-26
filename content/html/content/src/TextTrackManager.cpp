






#include "mozilla/dom/TextTrackManager.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/HTMLTrackElement.h"
#include "mozilla/dom/TextTrack.h"
#include "mozilla/dom/TextTrackCue.h"
#include "mozilla/ClearOnShutdown.h"
#include "nsComponentManagerUtils.h"
#include "nsVideoFrame.h"
#include "nsIFrame.h"
#include "nsTArrayHelpers.h"
#include "nsIWebVTTParserWrapper.h"

namespace mozilla {
namespace dom {

CompareTextTracks::CompareTextTracks(HTMLMediaElement* aMediaElement)
{
  mMediaElement = aMediaElement;
}

int32_t
CompareTextTracks::TrackChildPosition(TextTrack* aTextTrack) const {
  HTMLTrackElement* trackElement = aTextTrack->GetTrackElement();
  if (!trackElement) {
    return -1;
  }
  return mMediaElement->IndexOf(trackElement);
}

bool
CompareTextTracks::Equals(TextTrack* aOne, TextTrack* aTwo) const {
  
  
  
  
  return false;
}

bool
CompareTextTracks::LessThan(TextTrack* aOne, TextTrack* aTwo) const
{
  TextTrackSource sourceOne = aOne->GetTextTrackSource();
  TextTrackSource sourceTwo = aTwo->GetTextTrackSource();
  if (sourceOne != sourceTwo) {
    return sourceOne == Track ||
           (sourceOne == AddTextTrack && sourceTwo == MediaResourceSpecific);
  }
  switch (sourceOne) {
    case Track: {
      int32_t positionOne = TrackChildPosition(aOne);
      int32_t positionTwo = TrackChildPosition(aTwo);
      
      
      return positionOne != -1 && positionTwo != -1 &&
             positionOne < positionTwo;
    }
    case AddTextTrack:
      
      
      
      return true;
    case MediaResourceSpecific:
      
      break;
  }
  return true;
}

NS_IMPL_CYCLE_COLLECTION_3(TextTrackManager, mTextTracks,
                           mPendingTextTracks, mNewCues)
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(TextTrackManager, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(TextTrackManager, Release)

StaticRefPtr<nsIWebVTTParserWrapper> TextTrackManager::sParserWrapper;

TextTrackManager::TextTrackManager(HTMLMediaElement *aMediaElement)
  : mMediaElement(aMediaElement)
  , performedTrackSelection(false)
{
  MOZ_COUNT_CTOR(TextTrackManager);
  mNewCues = new TextTrackCueList(mMediaElement->OwnerDoc()->GetParentObject());
  mTextTracks = new TextTrackList(mMediaElement->OwnerDoc()->GetParentObject(),
                                  this);
  mPendingTextTracks =
    new TextTrackList(mMediaElement->OwnerDoc()->GetParentObject(), this);

  if (!sParserWrapper) {
    nsCOMPtr<nsIWebVTTParserWrapper> parserWrapper =
      do_CreateInstance(NS_WEBVTTPARSERWRAPPER_CONTRACTID);
    sParserWrapper = parserWrapper;
    ClearOnShutdown(&sParserWrapper);
  }
}

TextTrackManager::~TextTrackManager()
{
  MOZ_COUNT_DTOR(TextTrackManager);
}

TextTrackList*
TextTrackManager::TextTracks() const
{
  return mTextTracks;
}

already_AddRefed<TextTrack>
TextTrackManager::AddTextTrack(TextTrackKind aKind, const nsAString& aLabel,
                               const nsAString& aLanguage,
                               TextTrackMode aMode,
                               TextTrackReadyState aReadyState,
                               TextTrackSource aTextTrackSource)
{
  if (!mMediaElement) {
    return nullptr;
  }
  nsRefPtr<TextTrack> ttrack =
    mTextTracks->AddTextTrack(aKind, aLabel, aLanguage, aMode, aReadyState,
                              aTextTrackSource, CompareTextTracks(mMediaElement));
  AddCues(ttrack);

  if (aTextTrackSource == Track) {
    HonorUserPreferencesForTrackSelection();
  }

  return ttrack.forget();
}

void
TextTrackManager::AddTextTrack(TextTrack* aTextTrack)
{
  if (!mMediaElement) {
    return;
  }
  mTextTracks->AddTextTrack(aTextTrack, CompareTextTracks(mMediaElement));
  AddCues(aTextTrack);
  if (aTextTrack->GetTextTrackSource() == Track) {
    HonorUserPreferencesForTrackSelection();
  }
}

void
TextTrackManager::AddCues(TextTrack* aTextTrack)
{
  TextTrackCueList* cueList = aTextTrack->GetCues();
  if (cueList) {
    bool dummy;
    for (uint32_t i = 0; i < cueList->Length(); ++i) {
      mNewCues->AddCue(*cueList->IndexedGetter(i, dummy));
    }
  }
}

void
TextTrackManager::RemoveTextTrack(TextTrack* aTextTrack, bool aPendingListOnly)
{
  mPendingTextTracks->RemoveTextTrack(aTextTrack);
  if (aPendingListOnly) {
    return;
  }

  mTextTracks->RemoveTextTrack(aTextTrack);
}

void
TextTrackManager::DidSeek()
{
  mTextTracks->DidSeek();
}

void
TextTrackManager::UpdateCueDisplay()
{
  nsIFrame* frame = mMediaElement->GetPrimaryFrame();
  nsVideoFrame* videoFrame = do_QueryFrame(frame);
  if (!videoFrame) {
    return;
  }

  nsCOMPtr<nsIContent> overlay = videoFrame->GetCaptionOverlay();
  if (!overlay) {
    return;
  }

  nsTArray<nsRefPtr<TextTrackCue> > activeCues;
  mTextTracks->UpdateAndGetShowingCues(activeCues);

  if (activeCues.Length() > 0) {
    nsCOMPtr<nsIWritableVariant> jsCues =
      do_CreateInstance("@mozilla.org/variant;1");

    jsCues->SetAsArray(nsIDataType::VTYPE_INTERFACE,
                       &NS_GET_IID(nsIDOMEventTarget),
                       activeCues.Length(),
                       static_cast<void*>(activeCues.Elements()));

    nsPIDOMWindow* window = mMediaElement->OwnerDoc()->GetWindow();
    if (window) {
      sParserWrapper->ProcessCues(window, jsCues, overlay);
    }
  } else if (overlay->Length() > 0) {
    nsContentUtils::SetNodeTextContent(overlay, EmptyString(), true);
  }
}

void
TextTrackManager::AddCue(TextTrackCue& aCue)
{
  mNewCues->AddCue(aCue);
}

void
TextTrackManager::PopulatePendingList()
{
  uint32_t len = mTextTracks->Length();
  bool dummy;
  for (uint32_t index = 0; index < len; ++index) {
    TextTrack* ttrack = mTextTracks->IndexedGetter(index, dummy);
    if (ttrack && ttrack->Mode() != TextTrackMode::Disabled &&
        ttrack->ReadyState() == TextTrackReadyState::Loading) {
      mPendingTextTracks->AddTextTrack(ttrack,
                                       CompareTextTracks(mMediaElement));
    }
  }
}

void
TextTrackManager::HonorUserPreferencesForTrackSelection()
{
  if (performedTrackSelection) {
    return;
  }

  TextTrackKind ttKinds[] = { TextTrackKind::Captions,
                              TextTrackKind::Subtitles };

  
  
  PerformTrackSelection(ttKinds, ArrayLength(ttKinds));
  PerformTrackSelection(TextTrackKind::Descriptions);
  PerformTrackSelection(TextTrackKind::Chapters);

  
  
  for (uint32_t i = 0; i < mTextTracks->Length(); i++) {
    TextTrack* track = (*mTextTracks)[i];
    if (track->Kind() == TextTrackKind::Metadata && TrackIsDefault(track) &&
        track->Mode() == TextTrackMode::Disabled) {
      track->SetMode(TextTrackMode::Hidden);
    }
  }

  performedTrackSelection = true;
}

bool
TextTrackManager::TrackIsDefault(TextTrack* aTextTrack)
{
  HTMLTrackElement* trackElement = aTextTrack->GetTrackElement();
  if (!trackElement) {
    return false;
  }
  return trackElement->Default();
}

void
TextTrackManager::PerformTrackSelection(TextTrackKind aTextTrackKind)
{
  TextTrackKind ttKinds[] = { aTextTrackKind };
  PerformTrackSelection(ttKinds, ArrayLength(ttKinds));
}

void
TextTrackManager::PerformTrackSelection(TextTrackKind aTextTrackKinds[],
                                        uint32_t size)
{
  nsTArray<TextTrack*> candidates;
  GetTextTracksOfKinds(aTextTrackKinds, size, candidates);

  
  for (uint32_t i = 0; i < candidates.Length(); i++) {
    if (candidates[i]->Mode() == TextTrackMode::Showing) {
      return;
    }
  }

  
  
  
  for (uint32_t i = 0; i < candidates.Length(); i++) {
    if (TrackIsDefault(candidates[i]) &&
        candidates[i]->Mode() == TextTrackMode::Disabled) {
      candidates[i]->SetMode(TextTrackMode::Showing);
      return;
    }
  }
}

void
TextTrackManager::GetTextTracksOfKinds(TextTrackKind aTextTrackKinds[],
                                       uint32_t size,
                                       nsTArray<TextTrack*>& aTextTracks)
{
  for (uint32_t i = 0; i < size; i++) {
    GetTextTracksOfKind(aTextTrackKinds[i], aTextTracks);
  }
}

void
TextTrackManager::GetTextTracksOfKind(TextTrackKind aTextTrackKind,
                                      nsTArray<TextTrack*>& aTextTracks)
{
  for (uint32_t i = 0; i < mTextTracks->Length(); i++) {
    TextTrack* textTrack = (*mTextTracks)[i];
    if (textTrack->Kind() == aTextTrackKind) {
      aTextTracks.AppendElement(textTrack);
    }
  }
}

} 
} 
