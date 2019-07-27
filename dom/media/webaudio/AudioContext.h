





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
class AudioNodeStream;

namespace dom {

enum class AudioContextState : uint32_t;
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



class StateChangeTask final : public nsRunnable
{
public:
  

  StateChangeTask(AudioContext* aAudioContext, void* aPromise, AudioContextState aNewState);

  

  StateChangeTask(AudioNodeStream* aStream, void* aPromise, AudioContextState aNewState);

  NS_IMETHOD Run() override;

private:
  nsRefPtr<AudioContext> mAudioContext;
  void* mPromise;
  nsRefPtr<AudioNodeStream> mAudioNodeStream;
  AudioContextState mNewState;
};

enum AudioContextOperation { Suspend, Resume, Close };

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
  typedef uint64_t AudioContextId;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioContext,
                                           DOMEventTargetHelper)
  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf)

  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  void Shutdown(); 

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

  AudioContextId Id() const
  {
    return mId;
  }

  double CurrentTime() const;

  AudioListener* Listener();

  AudioContextState State() const;
  
  
  
  
  
  
  
  
  
  already_AddRefed<Promise> Suspend(ErrorResult& aRv);
  already_AddRefed<Promise> Resume(ErrorResult& aRv);
  already_AddRefed<Promise> Close(ErrorResult& aRv);
  IMPL_EVENT_HANDLER(statechange)

  already_AddRefed<AudioBufferSourceNode> CreateBufferSource(ErrorResult& aRv);

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
  CreateStereoPanner(ErrorResult& aRv);

  already_AddRefed<AnalyserNode>
  CreateAnalyser(ErrorResult& aRv);

  already_AddRefed<GainNode>
  CreateGain(ErrorResult& aRv);

  already_AddRefed<WaveShaperNode>
  CreateWaveShaper(ErrorResult& aRv);

  already_AddRefed<MediaElementAudioSourceNode>
  CreateMediaElementSource(HTMLMediaElement& aMediaElement, ErrorResult& aRv);
  already_AddRefed<MediaStreamAudioSourceNode>
  CreateMediaStreamSource(DOMMediaStream& aMediaStream, ErrorResult& aRv);

  already_AddRefed<DelayNode>
  CreateDelay(double aMaxDelayTime, ErrorResult& aRv);

  already_AddRefed<PannerNode>
  CreatePanner(ErrorResult& aRv);

  already_AddRefed<ConvolverNode>
  CreateConvolver(ErrorResult& aRv);

  already_AddRefed<ChannelSplitterNode>
  CreateChannelSplitter(uint32_t aNumberOfOutputs, ErrorResult& aRv);

  already_AddRefed<ChannelMergerNode>
  CreateChannelMerger(uint32_t aNumberOfInputs, ErrorResult& aRv);

  already_AddRefed<DynamicsCompressorNode>
  CreateDynamicsCompressor(ErrorResult& aRv);

  already_AddRefed<BiquadFilterNode>
  CreateBiquadFilter(ErrorResult& aRv);

  already_AddRefed<OscillatorNode>
  CreateOscillator(ErrorResult& aRv);

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

  void OnStateChanged(void* aPromise, AudioContextState aNewState);

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

  bool CheckClosed(ErrorResult& aRv);

private:
  
  
  
  const AudioContextId mId;
  
  
  const float mSampleRate;
  AudioContextState mAudioContextState;
  nsRefPtr<AudioDestinationNode> mDestination;
  nsRefPtr<AudioListener> mListener;
  nsTArray<nsRefPtr<WebAudioDecodeJob> > mDecodeJobs;
  
  
  nsTArray<nsRefPtr<Promise>> mPromiseGripArray;
  
  
  nsTHashtable<nsRefPtrHashKey<AudioNode> > mActiveNodes;
  
  
  nsTHashtable<nsPtrHashKey<PannerNode> > mPannerNodes;
  
  uint32_t mNumberOfChannels;
  
  int32_t mNodeCount;
  bool mIsOffline;
  bool mIsStarted;
  bool mIsShutDown;
  
  bool mCloseCalled;
};

static const dom::AudioContext::AudioContextId NO_AUDIO_CONTEXT = 0;

}
}

#endif

