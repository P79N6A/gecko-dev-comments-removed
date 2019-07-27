





#ifndef mozilla_dom_MediaTrack_h
#define mozilla_dom_MediaTrack_h

#include "mozilla/DOMEventTargetHelper.h"

namespace mozilla {
namespace dom {

class MediaTrackList;
class VideoTrack;
class AudioTrack;















class MediaTrack : public DOMEventTargetHelper
{
public:
  MediaTrack(const nsAString& aId,
             const nsAString& aKind,
             const nsAString& aLabel,
             const nsAString& aLanguage);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaTrack, DOMEventTargetHelper)

  enum {
    DEFAULT = 0,
    FIRE_NO_EVENTS = 1 << 0,
  };
  
  
  
  virtual void SetEnabledInternal(bool aEnabled, int aFlags) = 0;

  virtual AudioTrack* AsAudioTrack()
  {
    return nullptr;
  }

  virtual VideoTrack* AsVideoTrack()
  {
    return nullptr;
  }

  const nsString& GetId() const
  {
    return mId;
  }

  
  void GetId(nsAString& aId) const
  {
    aId = mId;
  }
  void GetKind(nsAString& aKind) const
  {
    aKind = mKind;
  }
  void GetLabel(nsAString& aLabel) const
  {
    aLabel = mLabel;
  }
  void GetLanguage(nsAString& aLanguage) const
  {
    aLanguage = mLanguage;
  }

  friend class MediaTrackList;

protected:
  virtual ~MediaTrack();

  void SetTrackList(MediaTrackList* aList);
  void Init(nsPIDOMWindow* aOwnerWindow);

  nsRefPtr<MediaTrackList> mList;
  nsString mId;
  nsString mKind;
  nsString mLabel;
  nsString mLanguage;
};

} 
} 

#endif 
