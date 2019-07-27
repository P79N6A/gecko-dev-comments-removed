





#ifndef mozilla_dom_AudioTrack_h
#define mozilla_dom_AudioTrack_h

#include "MediaTrack.h"

namespace mozilla {
namespace dom {

class AudioTrackList;

class AudioTrack : public MediaTrack
{
public:
  AudioTrack(const nsAString& aId,
             const nsAString& aKind,
             const nsAString& aLabel,
             const nsAString& aLanguage,
             bool aEnabled);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual AudioTrack* AsAudioTrack() MOZ_OVERRIDE
  {
    return this;
  }

  virtual void SetEnabledInternal(bool aEnabled, int aFlags) MOZ_OVERRIDE;

  
  bool Enabled() const
  {
    return mEnabled;
  }

  void SetEnabled(bool aEnabled);

private:
  bool mEnabled;
};

} 
} 

#endif 
