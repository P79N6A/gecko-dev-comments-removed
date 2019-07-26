





#ifndef AudioListener_h_
#define AudioListener_h_

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "ThreeDPoint.h"
#include "AudioContext.h"

struct JSContext;

namespace mozilla {

namespace dom {

class AudioContext;

class AudioListener MOZ_FINAL : public nsWrapperCache,
                                public EnableWebAudioCheck
{
public:
  explicit AudioListener(AudioContext* aContext);

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AudioListener)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AudioListener)

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  float DopplerFactor() const
  {
    return mDopplerFactor;
  }
  void SetDopplerFactor(float aDopplerFactor)
  {
    mDopplerFactor = aDopplerFactor;
  }

  float SpeedOfSound() const
  {
    return mSpeedOfSound;
  }
  void SetSpeedOfSound(float aSpeedOfSound)
  {
    mSpeedOfSound = aSpeedOfSound;
  }

  void SetPosition(float aX, float aY, float aZ)
  {
    mPosition.x = aX;
    mPosition.y = aY;
    mPosition.z = aZ;
  }

  void SetOrientation(float aX, float aY, float aZ,
                      float aXUp, float aYUp, float aZUp)
  {
    mOrientation.x = aX;
    mOrientation.y = aY;
    mOrientation.z = aZ;
    mUpVector.x = aXUp;
    mUpVector.y = aYUp;
    mUpVector.z = aZUp;
  }

  void SetVelocity(float aX, float aY, float aZ)
  {
    mVelocity.x = aX;
    mVelocity.y = aY;
    mVelocity.z = aZ;
  }

private:
  nsRefPtr<AudioContext> mContext;
  ThreeDPoint mPosition;
  ThreeDPoint mOrientation;
  ThreeDPoint mUpVector;
  ThreeDPoint mVelocity;
  float mDopplerFactor;
  float mSpeedOfSound;
};

}
}

#endif

