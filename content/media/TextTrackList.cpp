




#include "mozilla/dom/TextTrackList.h"
#include "mozilla/dom/TextTrackListBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_2(TextTrackList,
                                     nsDOMEventTargetHelper,
                                     mGlobal,
                                     mTextTracks)

NS_IMPL_ADDREF_INHERITED(TextTrackList, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TextTrackList, nsDOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TextTrackList)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

TextTrackList::TextTrackList(nsISupports* aGlobal) : mGlobal(aGlobal)
{
  SetIsDOMBinding();
}

void
TextTrackList::Update(double aTime)
{
  uint32_t length = Length(), i;
  for (i = 0; i < length; i++) {
    mTextTracks[i]->Update(aTime);
  }
}

JSObject*
TextTrackList::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return TextTrackListBinding::Wrap(aCx, aScope, this);
}

TextTrack*
TextTrackList::IndexedGetter(uint32_t aIndex, bool& aFound)
{
  aFound = aIndex < mTextTracks.Length();
  return aFound ? mTextTracks[aIndex] : nullptr;
}

already_AddRefed<TextTrack>
TextTrackList::AddTextTrack(HTMLMediaElement* aMediaElement,
                            TextTrackKind aKind,
                            const nsAString& aLabel,
                            const nsAString& aLanguage)
{
  nsRefPtr<TextTrack> track = new TextTrack(mGlobal, aMediaElement, aKind,
                                            aLabel, aLanguage);
  mTextTracks.AppendElement(track);
  
  return track.forget();
}

TextTrack*
TextTrackList::GetTrackById(const nsAString& aId)
{
  nsAutoString id;
  for (uint32_t i = 0; i < Length(); i++) {
    mTextTracks[i]->GetId(id);
    if (aId.Equals(id)) {
      return mTextTracks[i];
    }
  }
  return nullptr;
}

void
TextTrackList::RemoveTextTrack(const TextTrack& aTrack)
{
  mTextTracks.RemoveElement(&aTrack);
}

void
TextTrackList::DidSeek()
{
  for (uint32_t i = 0; i < mTextTracks.Length(); i++) {
    mTextTracks[i]->SetDirty();
  }
}

} 
} 
