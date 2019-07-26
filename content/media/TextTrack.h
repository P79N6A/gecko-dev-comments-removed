





#ifndef mozilla_dom_TextTrack_h
#define mozilla_dom_TextTrack_h

#include "mozilla/dom/TextTrackBinding.h"
#include "mozilla/dom/TextTrackCue.h"
#include "mozilla/dom/TextTrackCueList.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsString.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class TextTrackCue;
class TextTrackCueList;

class TextTrack MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(TextTrack,
                                                         nsDOMEventTargetHelper)

  TextTrack(nsISupports* aParent);
  TextTrack(nsISupports* aParent,
            TextTrackKind aKind,
            const nsAString& aLabel,
            const nsAString& aLanguage);

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

  TextTrackCueList* GetActiveCues() const
  {
    if (mMode == TextTrackMode::Disabled) {
      return nullptr;
    }
    return mActiveCueList;
  }

  
  void Update(double aTime);

  void AddCue(TextTrackCue& aCue);
  void RemoveCue(TextTrackCue& aCue);
  void CueChanged(TextTrackCue& aCue);

  IMPL_EVENT_HANDLER(cuechange)

private:
  nsCOMPtr<nsISupports> mParent;

  TextTrackKind mKind;
  nsString mLabel;
  nsString mLanguage;
  nsString mType;
  nsString mId;
  TextTrackMode mMode;

  nsRefPtr<TextTrackCueList> mCueList;
  nsRefPtr<TextTrackCueList> mActiveCueList;
};

} 
} 

#endif 
