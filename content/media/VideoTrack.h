





#ifndef mozilla_dom_VideoTrack_h
#define mozilla_dom_VideoTrack_h

#include "MediaTrack.h"

namespace mozilla {
namespace dom {

class VideoTrackList;

class VideoTrack : public MediaTrack
{
public:
  VideoTrack(const nsAString& aId,
             const nsAString& aKind,
             const nsAString& aLabel,
             const nsAString& aLanguage);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual VideoTrack* AsVideoTrack() MOZ_OVERRIDE
  {
    return this;
  }

  
  
  
  
  
  virtual void SetEnabledInternal(bool aEnabled, int aFlags) MOZ_OVERRIDE;

  
  bool Selected() const
  {
    return mSelected;
  }

  
  
  
  void SetSelected(bool aSelected);

private:
  bool mSelected;
};

} 
} 

#endif 
