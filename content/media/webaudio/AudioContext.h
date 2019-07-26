





#ifndef AudioContext_h_
#define AudioContext_h_

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"

class JSContext;
class nsIDOMWindow;

namespace mozilla {

class ErrorResult;

namespace dom {

class AudioBuffer;
class AudioBufferSourceNode;
class AudioDestinationNode;
class AudioListener;
class DelayNode;
class GainNode;
class PannerNode;

class AudioContext MOZ_FINAL : public nsWrapperCache,
                               public EnableWebAudioCheck
{
  explicit AudioContext(nsIDOMWindow* aParentWindow);

public:
  virtual ~AudioContext();

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AudioContext)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AudioContext)

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope,
                               bool* aTriedToWrap);

  static already_AddRefed<AudioContext>
  Constructor(nsISupports* aGlobal, ErrorResult& aRv);

  AudioDestinationNode* Destination() const
  {
    return mDestination;
  }

  AudioListener* Listener();

  already_AddRefed<AudioBufferSourceNode> CreateBufferSource();

  already_AddRefed<AudioBuffer>
  CreateBuffer(JSContext* aJSContext, uint32_t aNumberOfChannels,
               uint32_t aLength, float aSampleRate,
               ErrorResult& aRv);

  already_AddRefed<GainNode>
  CreateGain();

  already_AddRefed<DelayNode>
  CreateDelay(float aMaxDelayTime);

  already_AddRefed<PannerNode>
  CreatePanner();

private:
  nsCOMPtr<nsIDOMWindow> mWindow;
  nsRefPtr<AudioDestinationNode> mDestination;
  nsRefPtr<AudioListener> mListener;
};

}
}

#endif

