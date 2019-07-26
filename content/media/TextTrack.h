





#ifndef mozilla_dom_TextTrack_h
#define mozilla_dom_TextTrack_h

#include "mozilla/dom/TextTrackBinding.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class TextTrackList;
class TextTrackCue;
class TextTrackCueList;
class TextTrackRegion;
class HTMLTrackElement;

enum TextTrackSource {
  Track,
  AddTextTrack,
  MediaResourceSpecific
};

class TextTrack MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TextTrack, nsDOMEventTargetHelper)

  TextTrack(nsISupports* aParent,
            TextTrackKind aKind,
            const nsAString& aLabel,
            const nsAString& aLanguage,
            TextTrackMode aMode,
            TextTrackSource aTextTrackSource);
  TextTrack(nsISupports* aParent,
            TextTrackList* aTextTrackList,
            TextTrackKind aKind,
            const nsAString& aLabel,
            const nsAString& aLanguage,
            TextTrackMode aMode,
            TextTrackSource aTextTrackSource);

  void SetDefaultSettings();

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  TextTrackKind Kind() const
  {
    return mKind;
  }
  void GetLabel(nsAString& aLabel) const
  {
    aLabel = mLabel;
  }
  void GetLanguage(nsAString& aLanguage) const
  {
    aLanguage = mLanguage;
  }
  void GetInBandMetadataTrackDispatchType(nsAString& aType) const
  {
    aType = mType;
  }
  void GetId(nsAString& aId) const
  {
    aId = mId;
  }

  TextTrackMode Mode() const
  {
    return mMode;
  }
  void SetMode(TextTrackMode aValue);

  TextTrackCueList* GetCues() const
  {
    if (mMode == TextTrackMode::Disabled) {
      return nullptr;
    }
    return mCueList;
  }

  TextTrackCueList* GetActiveCues();
  void GetActiveCueArray(nsTArray<nsRefPtr<TextTrackCue> >& aCues);

  
  enum {
    READY_STATE_NONE = 0U,
    READY_STATE_LOADING = 1U,
    READY_STATE_LOADED = 2U,
    READY_STATE_ERROR = 3U
  };
  uint16_t ReadyState() const;
  void SetReadyState(uint16_t aState);

  void AddCue(TextTrackCue& aCue);
  void RemoveCue(TextTrackCue& aCue, ErrorResult& aRv);
  void CueChanged(TextTrackCue& aCue);
  void SetDirty() { mDirty = true; }

  TextTrackList* GetTextTrackList();
  void SetTextTrackList(TextTrackList* aTextTrackList);

  IMPL_EVENT_HANDLER(cuechange)

  HTMLTrackElement* GetTrackElement();
  void SetTrackElement(HTMLTrackElement* aTrackElement);

  TextTrackSource GetTextTrackSource() {
    return mTextTrackSource;
  }

private:
  void UpdateActiveCueList();

  nsCOMPtr<nsISupports> mParent;
  nsRefPtr<TextTrackList> mTextTrackList;

  TextTrackKind mKind;
  nsString mLabel;
  nsString mLanguage;
  nsString mType;
  nsString mId;
  TextTrackMode mMode;

  nsRefPtr<TextTrackCueList> mCueList;
  nsRefPtr<TextTrackCueList> mActiveCueList;
  nsRefPtr<HTMLTrackElement> mTrackElement;

  uint32_t mCuePos;
  uint16_t mReadyState;
  bool mDirty;

  
  TextTrackSource mTextTrackSource;
};

} 
} 

#endif 
