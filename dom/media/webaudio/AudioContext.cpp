





#include "AudioContext.h"

#include "nsPIDOMWindow.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/AnalyserNode.h"
#include "mozilla/dom/AudioContextBinding.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/OfflineAudioContextBinding.h"
#include "mozilla/dom/OwningNonNull.h"
#include "MediaStreamGraph.h"
#include "AudioChannelService.h"
#include "AudioDestinationNode.h"
#include "AudioBufferSourceNode.h"
#include "AudioBuffer.h"
#include "GainNode.h"
#include "MediaElementAudioSourceNode.h"
#include "MediaStreamAudioSourceNode.h"
#include "DelayNode.h"
#include "PannerNode.h"
#include "AudioListener.h"
#include "DynamicsCompressorNode.h"
#include "BiquadFilterNode.h"
#include "ScriptProcessorNode.h"
#include "StereoPannerNode.h"
#include "ChannelMergerNode.h"
#include "ChannelSplitterNode.h"
#include "MediaStreamAudioDestinationNode.h"
#include "WaveShaperNode.h"
#include "PeriodicWave.h"
#include "ConvolverNode.h"
#include "OscillatorNode.h"
#include "nsNetUtil.h"
#include "AudioStream.h"
#include "mozilla/dom/Promise.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(AudioContext)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(AudioContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDestination)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mListener)
  if (!tmp->mIsStarted) {
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mActiveNodes)
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END_INHERITED(DOMEventTargetHelper)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(AudioContext,
                                                  DOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDestination)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mListener)
  if (!tmp->mIsStarted) {
    MOZ_ASSERT(tmp->mIsOffline,
               "Online AudioContexts should always be started");
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mActiveNodes)
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(AudioContext, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(AudioContext, DOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioContext)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

static float GetSampleRateForAudioContext(bool aIsOffline, float aSampleRate)
{
  if (aIsOffline) {
    return aSampleRate;
  } else {
    CubebUtils::InitPreferredSampleRate();
    return static_cast<float>(CubebUtils::PreferredSampleRate());
  }
}

AudioContext::AudioContext(nsPIDOMWindow* aWindow,
                           bool aIsOffline,
                           AudioChannel aChannel,
                           uint32_t aNumberOfChannels,
                           uint32_t aLength,
                           float aSampleRate)
  : DOMEventTargetHelper(aWindow)
  , mSampleRate(GetSampleRateForAudioContext(aIsOffline, aSampleRate))
  , mNumberOfChannels(aNumberOfChannels)
  , mNodeCount(0)
  , mIsOffline(aIsOffline)
  , mIsStarted(!aIsOffline)
  , mIsShutDown(false)
{
  aWindow->AddAudioContext(this);

  
  
  mDestination = new AudioDestinationNode(this, aIsOffline, aChannel,
                                          aNumberOfChannels, aLength, aSampleRate);
  
  
  
  mDestination->CreateAudioChannelAgent();
  mDestination->SetIsOnlyNodeForContext(true);
}

AudioContext::~AudioContext()
{
  nsPIDOMWindow* window = GetOwner();
  if (window) {
    window->RemoveAudioContext(this);
  }

  UnregisterWeakMemoryReporter(this);
}

JSObject*
AudioContext::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  if (mIsOffline) {
    return OfflineAudioContextBinding::Wrap(aCx, this, aGivenProto);
  } else {
    return AudioContextBinding::Wrap(aCx, this, aGivenProto);
  }
}

 already_AddRefed<AudioContext>
AudioContext::Constructor(const GlobalObject& aGlobal,
                          ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<AudioContext> object =
    new AudioContext(window, false,
                     AudioChannelService::GetDefaultAudioChannel());

  RegisterWeakMemoryReporter(object);

  return object.forget();
}

 already_AddRefed<AudioContext>
AudioContext::Constructor(const GlobalObject& aGlobal,
                          AudioChannel aChannel,
                          ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<AudioContext> object = new AudioContext(window, false, aChannel);

  RegisterWeakMemoryReporter(object);

  return object.forget();
}

 already_AddRefed<AudioContext>
AudioContext::Constructor(const GlobalObject& aGlobal,
                          uint32_t aNumberOfChannels,
                          uint32_t aLength,
                          float aSampleRate,
                          ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (aNumberOfChannels == 0 ||
      aNumberOfChannels > WebAudioUtils::MaxChannelCount ||
      aLength == 0 ||
      aSampleRate < WebAudioUtils::MinSampleRate ||
      aSampleRate > WebAudioUtils::MaxSampleRate) {
    
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  nsRefPtr<AudioContext> object = new AudioContext(window,
                                                   true,
                                                   AudioChannel::Normal,
                                                   aNumberOfChannels,
                                                   aLength,
                                                   aSampleRate);

  RegisterWeakMemoryReporter(object);

  return object.forget();
}

already_AddRefed<AudioBufferSourceNode>
AudioContext::CreateBufferSource()
{
  nsRefPtr<AudioBufferSourceNode> bufferNode =
    new AudioBufferSourceNode(this);
  return bufferNode.forget();
}

already_AddRefed<AudioBuffer>
AudioContext::CreateBuffer(JSContext* aJSContext, uint32_t aNumberOfChannels,
                           uint32_t aLength, float aSampleRate,
                           ErrorResult& aRv)
{
  if (!aNumberOfChannels) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  return AudioBuffer::Create(this, aNumberOfChannels, aLength,
                             aSampleRate, aJSContext, aRv);
}

namespace {

bool IsValidBufferSize(uint32_t aBufferSize) {
  switch (aBufferSize) {
  case 0:       
  case 256:
  case 512:
  case 1024:
  case 2048:
  case 4096:
  case 8192:
  case 16384:
    return true;
  default:
    return false;
  }
}

}

already_AddRefed<MediaStreamAudioDestinationNode>
AudioContext::CreateMediaStreamDestination(ErrorResult& aRv)
{
  if (mIsOffline) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  nsRefPtr<MediaStreamAudioDestinationNode> node =
      new MediaStreamAudioDestinationNode(this);
  return node.forget();
}

already_AddRefed<ScriptProcessorNode>
AudioContext::CreateScriptProcessor(uint32_t aBufferSize,
                                    uint32_t aNumberOfInputChannels,
                                    uint32_t aNumberOfOutputChannels,
                                    ErrorResult& aRv)
{
  if ((aNumberOfInputChannels == 0 && aNumberOfOutputChannels == 0) ||
      aNumberOfInputChannels > WebAudioUtils::MaxChannelCount ||
      aNumberOfOutputChannels > WebAudioUtils::MaxChannelCount ||
      !IsValidBufferSize(aBufferSize)) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<ScriptProcessorNode> scriptProcessor =
    new ScriptProcessorNode(this, aBufferSize, aNumberOfInputChannels,
                            aNumberOfOutputChannels);
  return scriptProcessor.forget();
}

already_AddRefed<AnalyserNode>
AudioContext::CreateAnalyser()
{
  nsRefPtr<AnalyserNode> analyserNode = new AnalyserNode(this);
  return analyserNode.forget();
}

already_AddRefed<StereoPannerNode>
AudioContext::CreateStereoPanner()
{
  nsRefPtr<StereoPannerNode> stereoPannerNode = new StereoPannerNode(this);
  return stereoPannerNode.forget();
}

already_AddRefed<MediaElementAudioSourceNode>
AudioContext::CreateMediaElementSource(HTMLMediaElement& aMediaElement,
                                       ErrorResult& aRv)
{
  if (mIsOffline) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }
#ifdef MOZ_EME
  if (aMediaElement.ContainsRestrictedContent()) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }
#endif
  nsRefPtr<DOMMediaStream> stream = aMediaElement.MozCaptureStream(aRv,
                                                                   mDestination->Stream()->Graph());
  if (aRv.Failed()) {
    return nullptr;
  }
  nsRefPtr<MediaElementAudioSourceNode> mediaElementAudioSourceNode =
    new MediaElementAudioSourceNode(this, stream);
  return mediaElementAudioSourceNode.forget();
}

already_AddRefed<MediaStreamAudioSourceNode>
AudioContext::CreateMediaStreamSource(DOMMediaStream& aMediaStream,
                                      ErrorResult& aRv)
{
  if (mIsOffline) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }
  nsRefPtr<MediaStreamAudioSourceNode> mediaStreamAudioSourceNode =
    new MediaStreamAudioSourceNode(this, &aMediaStream);
  return mediaStreamAudioSourceNode.forget();
}

already_AddRefed<GainNode>
AudioContext::CreateGain()
{
  nsRefPtr<GainNode> gainNode = new GainNode(this);
  return gainNode.forget();
}

already_AddRefed<WaveShaperNode>
AudioContext::CreateWaveShaper()
{
  nsRefPtr<WaveShaperNode> waveShaperNode = new WaveShaperNode(this);
  return waveShaperNode.forget();
}

already_AddRefed<DelayNode>
AudioContext::CreateDelay(double aMaxDelayTime, ErrorResult& aRv)
{
  if (aMaxDelayTime > 0. && aMaxDelayTime < 180.) {
    nsRefPtr<DelayNode> delayNode = new DelayNode(this, aMaxDelayTime);
    return delayNode.forget();
  }
  aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
  return nullptr;
}

already_AddRefed<PannerNode>
AudioContext::CreatePanner()
{
  nsRefPtr<PannerNode> pannerNode = new PannerNode(this);
  mPannerNodes.PutEntry(pannerNode);
  return pannerNode.forget();
}

already_AddRefed<ConvolverNode>
AudioContext::CreateConvolver()
{
  nsRefPtr<ConvolverNode> convolverNode = new ConvolverNode(this);
  return convolverNode.forget();
}

already_AddRefed<ChannelSplitterNode>
AudioContext::CreateChannelSplitter(uint32_t aNumberOfOutputs, ErrorResult& aRv)
{
  if (aNumberOfOutputs == 0 ||
      aNumberOfOutputs > WebAudioUtils::MaxChannelCount) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<ChannelSplitterNode> splitterNode =
    new ChannelSplitterNode(this, aNumberOfOutputs);
  return splitterNode.forget();
}

already_AddRefed<ChannelMergerNode>
AudioContext::CreateChannelMerger(uint32_t aNumberOfInputs, ErrorResult& aRv)
{
  if (aNumberOfInputs == 0 ||
      aNumberOfInputs > WebAudioUtils::MaxChannelCount) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<ChannelMergerNode> mergerNode =
    new ChannelMergerNode(this, aNumberOfInputs);
  return mergerNode.forget();
}

already_AddRefed<DynamicsCompressorNode>
AudioContext::CreateDynamicsCompressor()
{
  nsRefPtr<DynamicsCompressorNode> compressorNode =
    new DynamicsCompressorNode(this);
  return compressorNode.forget();
}

already_AddRefed<BiquadFilterNode>
AudioContext::CreateBiquadFilter()
{
  nsRefPtr<BiquadFilterNode> filterNode =
    new BiquadFilterNode(this);
  return filterNode.forget();
}

already_AddRefed<OscillatorNode>
AudioContext::CreateOscillator()
{
  nsRefPtr<OscillatorNode> oscillatorNode =
    new OscillatorNode(this);
  return oscillatorNode.forget();
}

already_AddRefed<PeriodicWave>
AudioContext::CreatePeriodicWave(const Float32Array& aRealData,
                                 const Float32Array& aImagData,
                                 ErrorResult& aRv)
{
  aRealData.ComputeLengthAndData();
  aImagData.ComputeLengthAndData();

  if (aRealData.Length() != aImagData.Length() ||
      aRealData.Length() == 0 ||
      aRealData.Length() > 4096) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  nsRefPtr<PeriodicWave> periodicWave =
    new PeriodicWave(this, aRealData.Data(), aImagData.Data(),
                     aImagData.Length(), aRv);
  if (aRv.Failed()) {
    return nullptr;
  }
  return periodicWave.forget();
}

AudioListener*
AudioContext::Listener()
{
  if (!mListener) {
    mListener = new AudioListener(this);
  }
  return mListener;
}

already_AddRefed<Promise>
AudioContext::DecodeAudioData(const ArrayBuffer& aBuffer,
                              const Optional<OwningNonNull<DecodeSuccessCallback> >& aSuccessCallback,
                              const Optional<OwningNonNull<DecodeErrorCallback> >& aFailureCallback,
                              ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> parentObject = do_QueryInterface(GetParentObject());
  nsRefPtr<Promise> promise;
  AutoJSAPI jsapi;
  jsapi.Init();
  JSContext* cx = jsapi.cx();
  JSAutoCompartment ac(cx, aBuffer.Obj());

  promise = Promise::Create(parentObject, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  aBuffer.ComputeLengthAndData();

  
  size_t length = aBuffer.Length();
  JS::RootedObject obj(cx, aBuffer.Obj());

  uint8_t* data = static_cast<uint8_t*>(JS_StealArrayBufferContents(cx, obj));

  
  
  nsAutoCString contentType;
  NS_SniffContent(NS_DATA_SNIFFER_CATEGORY, nullptr, data, length, contentType);

  nsRefPtr<DecodeErrorCallback> failureCallback;
  nsRefPtr<DecodeSuccessCallback> successCallback;
  if (aFailureCallback.WasPassed()) {
    failureCallback = &aFailureCallback.Value();
  }
  if (aSuccessCallback.WasPassed()) {
    successCallback = &aSuccessCallback.Value();
  }
  nsRefPtr<WebAudioDecodeJob> job(
    new WebAudioDecodeJob(contentType, this,
                          promise, successCallback, failureCallback));
  AsyncDecodeWebAudio(contentType.get(), data, length, *job);
  
  mDecodeJobs.AppendElement(job.forget());

  return promise.forget();
}

void
AudioContext::RemoveFromDecodeQueue(WebAudioDecodeJob* aDecodeJob)
{
  mDecodeJobs.RemoveElement(aDecodeJob);
}

void
AudioContext::RegisterActiveNode(AudioNode* aNode)
{
  if (!mIsShutDown) {
    mActiveNodes.PutEntry(aNode);
  }
}

void
AudioContext::UnregisterActiveNode(AudioNode* aNode)
{
  mActiveNodes.RemoveEntry(aNode);
}

void
AudioContext::UnregisterAudioBufferSourceNode(AudioBufferSourceNode* aNode)
{
  UpdatePannerSource();
}

void
AudioContext::UnregisterPannerNode(PannerNode* aNode)
{
  mPannerNodes.RemoveEntry(aNode);
  if (mListener) {
    mListener->UnregisterPannerNode(aNode);
  }
}

static PLDHashOperator
FindConnectedSourcesOn(nsPtrHashKey<PannerNode>* aEntry, void* aData)
{
  aEntry->GetKey()->FindConnectedSources();
  return PL_DHASH_NEXT;
}

void
AudioContext::UpdatePannerSource()
{
  mPannerNodes.EnumerateEntries(FindConnectedSourcesOn, nullptr);
}

uint32_t
AudioContext::MaxChannelCount() const
{
  return mIsOffline ? mNumberOfChannels : CubebUtils::MaxNumberOfChannels();
}

MediaStreamGraph*
AudioContext::Graph() const
{
  return Destination()->Stream()->Graph();
}

MediaStream*
AudioContext::DestinationStream() const
{
  if (Destination()) {
    return Destination()->Stream();
  }
  return nullptr;
}

double
AudioContext::CurrentTime() const
{
  MediaStream* stream = Destination()->Stream();
  return StreamTimeToDOMTime(stream->
                             StreamTimeToSeconds(stream->GetCurrentTime()));
}

void
AudioContext::Shutdown()
{
  mIsShutDown = true;

  
  
  
  if (!mIsOffline) {
    Mute();
  }

  
  
  
  mActiveNodes.Clear();

  
  if (mIsOffline && mDestination) {
    mDestination->OfflineShutdown();
  }
}

void
AudioContext::Suspend()
{
  MediaStream* ds = DestinationStream();
  if (ds) {
    ds->ChangeExplicitBlockerCount(1);
  }
}

void
AudioContext::Resume()
{
  MediaStream* ds = DestinationStream();
  if (ds) {
    ds->ChangeExplicitBlockerCount(-1);
  }
}

void
AudioContext::UpdateNodeCount(int32_t aDelta)
{
  bool firstNode = mNodeCount == 0;
  mNodeCount += aDelta;
  MOZ_ASSERT(mNodeCount >= 0);
  
  if (!firstNode && mDestination) {
    mDestination->SetIsOnlyNodeForContext(mNodeCount == 1);
  }
}

JSObject*
AudioContext::GetGlobalJSObject() const
{
  nsCOMPtr<nsIGlobalObject> parentObject = do_QueryInterface(GetParentObject());
  if (!parentObject) {
    return nullptr;
  }

  
  return parentObject->GetGlobalJSObject();
}

already_AddRefed<Promise>
AudioContext::StartRendering(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> parentObject = do_QueryInterface(GetParentObject());

  MOZ_ASSERT(mIsOffline, "This should only be called on OfflineAudioContext");
  if (mIsStarted) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }

  mIsStarted = true;
  nsRefPtr<Promise> promise = Promise::Create(parentObject, aRv);
  mDestination->StartRendering(promise);
  return promise.forget();
}

void
AudioContext::Mute() const
{
  MOZ_ASSERT(!mIsOffline);
  if (mDestination) {
    mDestination->Mute();
  }
}

void
AudioContext::Unmute() const
{
  MOZ_ASSERT(!mIsOffline);
  if (mDestination) {
    mDestination->Unmute();
  }
}

AudioChannel
AudioContext::MozAudioChannelType() const
{
  return mDestination->MozAudioChannelType();
}

AudioChannel
AudioContext::TestAudioChannelInAudioNodeStream()
{
  MediaStream* stream = mDestination->Stream();
  MOZ_ASSERT(stream);

  return stream->AudioChannelType();
}

size_t
AudioContext::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  
  
  

  size_t amount = aMallocSizeOf(this);
  if (mListener) {
    amount += mListener->SizeOfIncludingThis(aMallocSizeOf);
  }
  amount += mDecodeJobs.SizeOfExcludingThis(aMallocSizeOf);
  for (uint32_t i = 0; i < mDecodeJobs.Length(); ++i) {
    amount += mDecodeJobs[i]->SizeOfIncludingThis(aMallocSizeOf);
  }
  amount += mActiveNodes.SizeOfExcludingThis(nullptr, aMallocSizeOf);
  amount += mPannerNodes.SizeOfExcludingThis(nullptr, aMallocSizeOf);
  return amount;
}

NS_IMETHODIMP
AudioContext::CollectReports(nsIHandleReportCallback* aHandleReport,
                             nsISupports* aData, bool aAnonymize)
{
  int64_t amount = SizeOfIncludingThis(MallocSizeOf);
  return MOZ_COLLECT_REPORT("explicit/webaudio/audiocontext", KIND_HEAP, UNITS_BYTES,
                            amount, "Memory used by AudioContext objects (Web Audio).");
}

double
AudioContext::ExtraCurrentTime() const
{
  return mDestination->ExtraCurrentTime();
}

}
}
