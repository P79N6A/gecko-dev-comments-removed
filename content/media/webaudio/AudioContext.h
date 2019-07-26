





#ifndef AudioContext_h_
#define AudioContext_h_

#include "nsDOMEventTargetHelper.h"
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
#include "nsTHashtable.h"



#ifdef CurrentTime
#undef CurrentTime
#endif

struct JSContext;
class JSObject;
class nsPIDOMWindow;

namespace mozilla {

class ErrorResult;
struct WebAudioDecodeJob;

namespace dom {

class AnalyserNode;
class AudioBuffer;
class AudioBufferSourceNode;
class AudioDestinationNode;
class AudioListener;
class BiquadFilterNode;
class ChannelMergerNode;
class ChannelSplitterNode;
class DelayNode;
class DynamicsCompressorNode;
class GainNode;
class GlobalObject;
class PannerNode;
class ScriptProcessorNode;

class AudioContext MOZ_FINAL : public nsDOMEventTargetHelper,
                               public EnableWebAudioCheck
{
  explicit AudioContext(nsPIDOMWindow* aParentWindow);
  ~AudioContext();

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioContext,
                                           nsDOMEventTargetHelper)

  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  void Shutdown();
  void Suspend();
  void Resume();

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

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

  already_AddRefed<AudioBuffer>
  CreateBuffer(JSContext* aJSContext, ArrayBuffer& aBuffer,
               bool aMixToMono, ErrorResult& aRv);

  already_AddRefed<ScriptProcessorNode>
  CreateScriptProcessor(uint32_t aBufferSize,
                        uint32_t aNumberOfInputChannels,
                        uint32_t aNumberOfOutputChannels,
                        ErrorResult& aRv);

  already_AddRefed<ScriptProcessorNode>
  CreateJavaScriptNode(uint32_t aBufferSize,
                       uint32_t aNumberOfInputChannels,
                       uint32_t aNumberOfOutputChannels,
                       ErrorResult& aRv)
  {
    return CreateScriptProcessor(aBufferSize, aNumberOfInputChannels,
                                 aNumberOfOutputChannels, aRv);
  }

  already_AddRefed<AnalyserNode>
  CreateAnalyser();

  already_AddRefed<GainNode>
  CreateGain();

  already_AddRefed<GainNode>
  CreateGainNode()
  {
    return CreateGain();
  }

  already_AddRefed<DelayNode>
  CreateDelay(double aMaxDelayTime, ErrorResult& aRv);

  already_AddRefed<DelayNode>
  CreateDelayNode(double aMaxDelayTime, ErrorResult& aRv)
  {
    return CreateDelay(aMaxDelayTime, aRv);
  }

  already_AddRefed<PannerNode>
  CreatePanner();

  already_AddRefed<ChannelSplitterNode>
  CreateChannelSplitter(uint32_t aNumberOfOutputs, ErrorResult& aRv);

  already_AddRefed<ChannelMergerNode>
  CreateChannelMerger(uint32_t aNumberOfInputs, ErrorResult& aRv);

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
  void UnregisterAudioBufferSourceNode(AudioBufferSourceNode* aNode);
  void UnregisterPannerNode(PannerNode* aNode);
  void UnregisterScriptProcessorNode(ScriptProcessorNode* aNode);
  void UpdatePannerSource();

  JSContext* GetJSContext() const;

private:
  void RemoveFromDecodeQueue(WebAudioDecodeJob* aDecodeJob);

  friend struct ::mozilla::WebAudioDecodeJob;

private:
  nsRefPtr<AudioDestinationNode> mDestination;
  nsRefPtr<AudioListener> mListener;
  MediaBufferDecoder mDecoder;
  nsTArray<nsAutoPtr<WebAudioDecodeJob> > mDecodeJobs;
  
  
  
  nsTHashtable<nsPtrHashKey<PannerNode> > mPannerNodes;
  nsTHashtable<nsPtrHashKey<AudioBufferSourceNode> > mAudioBufferSourceNodes;
  
  
  nsTHashtable<nsPtrHashKey<ScriptProcessorNode> > mScriptProcessorNodes;
};

}
}

#endif

