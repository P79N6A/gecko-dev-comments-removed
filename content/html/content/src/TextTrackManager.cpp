






#include "mozilla/dom/TextTrackManager.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/HTMLTrackElement.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_1(TextTrackManager, mTextTracks)
NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(TextTrackManager, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(TextTrackManager, Release)

TextTrackManager::TextTrackManager(HTMLMediaElement *aMediaElement)
  : mMediaElement(aMediaElement)
{
  MOZ_COUNT_CTOR(TextTrackManager);
  mTextTracks = new TextTrackList(mMediaElement->OwnerDoc()->GetParentObject());
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
                               const nsAString& aLanguage)
{
  nsRefPtr<TextTrack> ttrack =
    mTextTracks->AddTextTrack(mMediaElement, aKind, aLabel, aLanguage);
  ttrack->SetReadyState(HTMLTrackElement::LOADED);
  return ttrack.forget();
}

void
TextTrackManager::AddTextTrack(TextTrack* aTextTrack)
{
  mTextTracks->AddTextTrack(aTextTrack);
}

void
TextTrackManager::RemoveTextTrack(TextTrack* aTextTrack)
{
  mTextTracks->RemoveTextTrack(aTextTrack);
}

void
TextTrackManager::DidSeek()
{
  mTextTracks->DidSeek();
}

void
TextTrackManager::Update(double aTime)
{
  mTextTracks->Update(aTime);
}

} 
} 
