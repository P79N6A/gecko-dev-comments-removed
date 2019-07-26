





#ifndef AudioContext_h_
#define AudioContext_h_

#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "EnableWebAudioCheck.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/AudioContextBinding.h"
#include "MediaBufferDecoder.h"
#include "StreamBuffer.h"
#include "MediaStreamGraph.h"
#include "nsIDOMWindow.h"



#ifdef CurrentTime
#undef CurrentTime
#endif

struct JSContext;
class JSObject;
class nsIDOMWindow;

namespace mozilla {

class ErrorResult;
struct WebAudioDecodeJob;

namespace dom {

class AudioBuffer;
class AudioBufferSourceNode;
class AudioDestinationNode;
class AudioListener;
class BiquadFilterNode;
class DelayNode;
class DynamicsCompressorNode;
class GainNode;
class GlobalObject;
class PannerNode;

class AudioContext MOZ_FINAL : public nsWrapperCache,
                               public EnableWebAudioCheck
{
  explicit AudioContext(nsIDOMWindow* aParentWindow);
  ~AudioContext();

public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(AudioContext)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(AudioContext)

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  void Shutdown()
  {
    mDecoder.Shutdown();
  }

  void Suspend();
  void Resume();

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  static already_AddRefed<AudioContext>
  Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

  AudioDestinationNode* Destination() const
  {
    return mDestination;
  }

  float SampleRate() const
  {
    return float(IdealAudioRate());
  }

  double CurrentTime() const;

  AudioListener* Listener();

  already_AddRefed<AudioBufferSourceNode> CreateBufferSource();

  already_AddRefed<AudioBuffer>
  CreateBuffer(JSContext* aJSContext, uint32_t aNumberOfChannels,
               uint32_t aLength, float aSampleRate,
               ErrorResult& aRv);

  already_AddRefed<GainNode>
  CreateGain();

  already_AddRefed<DelayNode>
  CreateDelay(double aMaxDelayTime, ErrorResult& aRv);

  already_AddRefed<PannerNode>
  CreatePanner();

  already_AddRefed<DynamicsCompressorNode>
  CreateDynamicsCompressor();

  already_AddRefed<BiquadFilterNode>
  CreateBiquadFilter();

  void DecodeAudioData(const ArrayBuffer& aBuffer,
                       DecodeSuccessCallback& aSuccessCallback,
                       const Optional<OwningNonNull<DecodeErrorCallback> >& aFailureCallback);

  uint32_t GetRate() const { return IdealAudioRate(); }

  MediaStreamGraph* Graph() const;
  MediaStream* DestinationStream() const;

private:
  void RemoveFromDecodeQueue(WebAudioDecodeJob* aDecodeJob);

  friend struct ::mozilla::WebAudioDecodeJob;

private:
  nsCOMPtr<nsIDOMWindow> mWindow;
  nsRefPtr<AudioDestinationNode> mDestination;
  nsRefPtr<AudioListener> mListener;
  MediaBufferDecoder mDecoder;
  nsTArray<nsAutoPtr<WebAudioDecodeJob> > mDecodeJobs;
};

}
}

#endif

