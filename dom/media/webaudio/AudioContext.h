





#ifndef AudioContext_h_
#define AudioContext_h_

#include "mozilla/dom/AudioChannelBinding.h"
#include "MediaBufferDecoder.h"
#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/TypedArray.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"
#include "js/TypeDecls.h"
#include "nsIMemoryReporter.h"



#ifdef CurrentTime
#undef CurrentTime
#endif

class nsPIDOMWindow;

namespace mozilla {

class DOMMediaStream;
class ErrorResult;
class MediaStream;
class MediaStreamGraph;

namespace dom {

class AnalyserNode;
class AudioBuffer;
class AudioBufferSourceNode;
class AudioDestinationNode;
class AudioListener;
class AudioNode;
class BiquadFilterNode;
class ChannelMergerNode;
class ChannelSplitterNode;
class ConvolverNode;
class DelayNode;
class DynamicsCompressorNode;
class GainNode;
class HTMLMediaElement;
class MediaElementAudioSourceNode;
class GlobalObject;
class MediaStreamAudioDestinationNode;
class MediaStreamAudioSourceNode;
class OscillatorNode;
class PannerNode;
class ScriptProcessorNode;
class StereoPannerNode;
class WaveShaperNode;
class PeriodicWave;
class Promise;

class AudioContext final : public DOMEventTargetHelper,
                           public nsIMemoryReporter
{
  AudioContext(nsPIDOMWindow* aParentWindow,
               bool aIsOffline,
               AudioChannel aChannel,
               uint32_t aNumberOfChannels = 0,
               uint32_t aLength = 0,
               float aSampleRate = 0.0f);
  ~AudioContext();

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioContext,
                                           DOMEventTargetHelper)
  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf)

  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  void Shutdown(); 
  void Suspend();
  void Resume();

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  using DOMEventTargetHelper::DispatchTrustedEvent;

  
  static already_AddRefed<AudioContext>
  Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

  
  static already_AddRefed<AudioContext>
  Constructor(const GlobalObject& aGlobal,
              AudioChannel aChannel,
              ErrorResult& aRv);

  
  static already_AddRefed<AudioContext>
  Constructor(const GlobalObject& aGlobal,
              uint32_t aNumberOfChannels,
              uint32_t aLength,
              float aSampleRate,
              ErrorResult& aRv);

  

  AudioDestinationNode* Destination() const
  {
    return mDestination;
  }

  float SampleRate() const
  {
    return mSampleRate;
  }

  double CurrentTime() const;

  AudioListener* Listener();

  already_AddRefed<AudioBufferSourceNode> CreateBufferSource();

  already_AddRefed<AudioBuffer>
  CreateBuffer(JSContext* aJSContext, uint32_t aNumberOfChannels,
               uint32_t aLength, float aSampleRate,
               ErrorResult& aRv);

  already_AddRefed<MediaStreamAudioDestinationNode>
  CreateMediaStreamDestination(ErrorResult& aRv);

  already_AddRefed<ScriptProcessorNode>
  CreateScriptProcessor(uint32_t aBufferSize,
                        uint32_t aNumberOfInputChannels,
                        uint32_t aNumberOfOutputChannels,
                        ErrorResult& aRv);

  already_AddRefed<StereoPannerNode>
  CreateStereoPanner();

  already_AddRefed<AnalyserNode>
  CreateAnalyser();

  already_AddRefed<GainNode>
  CreateGain();

  already_AddRefed<WaveShaperNode>
  CreateWaveShaper();

  already_AddRefed<MediaElementAudioSourceNode>
  CreateMediaElementSource(HTMLMediaElement& aMediaElement, ErrorResult& aRv);
  already_AddRefed<MediaStreamAudioSourceNode>
  CreateMediaStreamSource(DOMMediaStream& aMediaStream, ErrorResult& aRv);

  already_AddRefed<DelayNode>
  CreateDelay(double aMaxDelayTime, ErrorResult& aRv);

  already_AddRefed<PannerNode>
  CreatePanner();

  already_AddRefed<ConvolverNode>
  CreateConvolver();

  already_AddRefed<ChannelSplitterNode>
  CreateChannelSplitter(uint32_t aNumberOfOutputs, ErrorResult& aRv);

  already_AddRefed<ChannelMergerNode>
  CreateChannelMerger(uint32_t aNumberOfInputs, ErrorResult& aRv);

  already_AddRefed<DynamicsCompressorNode>
  CreateDynamicsCompressor();

  already_AddRefed<BiquadFilterNode>
  CreateBiquadFilter();

  already_AddRefed<OscillatorNode>
  CreateOscillator();

  already_AddRefed<PeriodicWave>
  CreatePeriodicWave(const Float32Array& aRealData, const Float32Array& aImagData,
                     ErrorResult& aRv);

  already_AddRefed<Promise>
  DecodeAudioData(const ArrayBuffer& aBuffer,
                  const Optional<OwningNonNull<DecodeSuccessCallback> >& aSuccessCallback,
                  const Optional<OwningNonNull<DecodeErrorCallback> >& aFailureCallback,
                  ErrorResult& aRv);

  
  already_AddRefed<Promise> StartRendering(ErrorResult& aRv);
  IMPL_EVENT_HANDLER(complete)

  bool IsOffline() const { return mIsOffline; }

  MediaStreamGraph* Graph() const;
  MediaStream* DestinationStream() const;

  
  
  
  
  void RegisterActiveNode(AudioNode* aNode);
  
  
  
  
  
  
  void UnregisterActiveNode(AudioNode* aNode);

  void UnregisterAudioBufferSourceNode(AudioBufferSourceNode* aNode);
  void UnregisterPannerNode(PannerNode* aNode);
  void UpdatePannerSource();

  uint32_t MaxChannelCount() const;

  void Mute() const;
  void Unmute() const;

  JSObject* GetGlobalJSObject() const;

  AudioChannel MozAudioChannelType() const;

  AudioChannel TestAudioChannelInAudioNodeStream();

  void UpdateNodeCount(int32_t aDelta);

  double DOMTimeToStreamTime(double aTime) const
  {
    return aTime - ExtraCurrentTime();
  }

  double StreamTimeToDOMTime(double aTime) const
  {
    return aTime + ExtraCurrentTime();
  }

  IMPL_EVENT_HANDLER(mozinterruptbegin)
  IMPL_EVENT_HANDLER(mozinterruptend)

private:
  






  double ExtraCurrentTime() const;

  void RemoveFromDecodeQueue(WebAudioDecodeJob* aDecodeJob);
  void ShutdownDecoder();

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
  NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                            nsISupports* aData, bool aAnonymize) override;

  friend struct ::mozilla::WebAudioDecodeJob;

private:
  
  
  const float mSampleRate;
  nsRefPtr<AudioDestinationNode> mDestination;
  nsRefPtr<AudioListener> mListener;
  nsTArray<nsRefPtr<WebAudioDecodeJob> > mDecodeJobs;
  
  
  nsTHashtable<nsRefPtrHashKey<AudioNode> > mActiveNodes;
  
  
  nsTHashtable<nsPtrHashKey<PannerNode> > mPannerNodes;
  
  uint32_t mNumberOfChannels;
  
  int32_t mNodeCount;
  bool mIsOffline;
  bool mIsStarted;
  bool mIsShutDown;
};

}
}

#endif

