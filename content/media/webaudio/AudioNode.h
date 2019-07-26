





#pragma once

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "AudioContext.h"

struct JSContext;

namespace mozilla {
namespace dom {

class AudioNode : public nsISupports,
                  public nsWrapperCache,
                  public EnableWebAudioCheck
{
public:
  explicit AudioNode(AudioContext* aContext);
  virtual ~AudioNode() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(AudioNode)

  AudioContext* GetParentObject() const
  {
    return mContext;
  }

  AudioContext* Context() const
  {
    return mContext;
  }

  void Connect(AudioNode& aDestination, uint32_t aOutput,
               uint32_t aInput)
  {  }

  void Disconnect(uint32_t aOutput)
  {  }

private:
  nsRefPtr<AudioContext> mContext;
};

}
}

