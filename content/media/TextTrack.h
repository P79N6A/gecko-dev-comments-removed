





#ifndef mozilla_dom_TextTrack_h
#define mozilla_dom_TextTrack_h

#include "mozilla/dom/TextTrackBinding.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

class TextTrackCue;
class TextTrackCueList;
class TextTrackRegion;
class TextTrackRegionList;
class HTMLMediaElement;

class TextTrack MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TextTrack, nsDOMEventTargetHelper)

  TextTrack(nsISupports* aParent);
  TextTrack(nsISupports* aParent,
            HTMLMediaElement* aMediaElement);
  TextTrack(nsISupports* aParent,
            HTMLMediaElement* aMediaElement,
            TextTrackKind aKind,
            const nsAString& aLabel,
            const nsAString& aLanguage);

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

  TextTrackRegionList* GetRegions() const
  {
    if (mMode != TextTrackMode::Disabled) {
      return mRegionList;
    }
    return nullptr;
  }

  void AddRegion(TextTrackRegion& aRegion);
  void RemoveRegion(const TextTrackRegion& aRegion, ErrorResult& aRv);

  
  void Update(double aTime);

  void AddCue(TextTrackCue& aCue);
  void RemoveCue(TextTrackCue& aCue, ErrorResult& aRv);
  void CueChanged(TextTrackCue& aCue);
  void SetDirty() { mDirty = true; }

  IMPL_EVENT_HANDLER(cuechange)

private:
  nsCOMPtr<nsISupports> mParent;
  nsRefPtr<HTMLMediaElement> mMediaElement;

  TextTrackKind mKind;
  nsString mLabel;
  nsString mLanguage;
  nsString mType;
  nsString mId;
  TextTrackMode mMode;

  nsRefPtr<TextTrackCueList> mCueList;
  nsRefPtr<TextTrackCueList> mActiveCueList;
  nsRefPtr<TextTrackRegionList> mRegionList;

  uint32_t mCuePos;
  bool mDirty;
};

} 
} 

#endif 
