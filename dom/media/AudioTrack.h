





#ifndef mozilla_dom_AudioTrack_h
#define mozilla_dom_AudioTrack_h

#include "MediaTrack.h"

namespace mozilla {
namespace dom {

class AudioTrack : public MediaTrack
{
public:
  AudioTrack(const nsAString& aId,
             const nsAString& aKind,
             const nsAString& aLabel,
             const nsAString& aLanguage,
             bool aEnabled);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual AudioTrack* AsAudioTrack() override
  {
    return this;
  }

  virtual void SetEnabledInternal(bool aEnabled, int aFlags) override;

  
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
