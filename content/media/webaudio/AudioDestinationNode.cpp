





#include "AudioDestinationNode.h"
#include "mozilla/dom/AudioDestinationNodeBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "AudioChannelAgent.h"
#include "AudioChannelService.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "MediaStreamGraph.h"
#include "OfflineAudioCompletionEvent.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShell.h"
#include "nsIPermissionManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsServiceManagerUtils.h"
#include "nsIAppShell.h"
#include "nsWidgetsCID.h"

namespace mozilla {
namespace dom {

static uint8_t gWebAudioOutputKey;

class OfflineDestinationNodeEngine : public AudioNodeEngine
{
public:
  typedef AutoFallibleTArray<nsAutoArrayPtr<float>, 2> InputChannels;

  OfflineDestinationNodeEngine(AudioDestinationNode* aNode,
                               uint32_t aNumberOfChannels,
                               uint32_t aLength,
                               float aSampleRate)
    : AudioNodeEngine(aNode)
    , mWriteIndex(0)
    , mLength(aLength)
    , mSampleRate(aSampleRate)
  {
    
    
    
    if (mInputChannels.SetLength(aNumberOfChannels)) {
      static const fallible_t fallible = fallible_t();
      for (uint32_t i = 0; i < aNumberOfChannels; ++i) {
        mInputChannels[i] = new(fallible) float[aLength];
        if (!mInputChannels[i]) {
          mInputChannels.Clear();
          break;
        }
      }
    }
  }

  virtual void ProcessBlock(AudioNodeStream* aStream,
                            const AudioChunk& aInput,
                            AudioChunk* aOutput,
                            bool* aFinished) MOZ_OVERRIDE
  {
    
    
    *aOutput = aInput;

    
    if (mInputChannels.IsEmpty()) {
      return;
    }

    if (mWriteIndex >= mLength) {
      NS_ASSERTION(mWriteIndex == mLength, "Overshot length");
      
      return;
    }

    
    MOZ_ASSERT(mWriteIndex < mLength, "How did this happen?");
    const uint32_t duration = std::min(WEBAUDIO_BLOCK_SIZE, mLength - mWriteIndex);
    const uint32_t commonChannelCount = std::min(mInputChannels.Length(),
                                                 aInput.mChannelData.Length());
    
    for (uint32_t i = 0; i < commonChannelCount; ++i) {
      if (aInput.IsNull()) {
        PodZero(mInputChannels[i] + mWriteIndex, duration);
      } else {
        const float* inputBuffer = static_cast<const float*>(aInput.mChannelData[i]);
        if (duration == WEBAUDIO_BLOCK_SIZE) {
          
          AudioBlockCopyChannelWithScale(inputBuffer, aInput.mVolume,
                                         mInputChannels[i] + mWriteIndex);
        } else {
          if (aInput.mVolume == 1.0f) {
            PodCopy(mInputChannels[i] + mWriteIndex, inputBuffer, duration);
          } else {
            for (uint32_t j = 0; j < duration; ++j) {
              mInputChannels[i][mWriteIndex + j] = aInput.mVolume * inputBuffer[j];
            }
          }
        }
      }
    }
    
    for (uint32_t i = commonChannelCount; i < mInputChannels.Length(); ++i) {
      PodZero(mInputChannels[i] + mWriteIndex, duration);
    }
    mWriteIndex += duration;

    if (mWriteIndex >= mLength) {
      NS_ASSERTION(mWriteIndex == mLength, "Overshot length");
      
      
      
      *aFinished = true;
    }
  }

  void FireOfflineCompletionEvent(AudioDestinationNode* aNode)
  {
    AudioContext* context = aNode->Context();
    context->Shutdown();
    
    
    

    
    JSObject* global = context->GetGlobalJSObject();
    if (NS_WARN_IF(!global)) {
      return;
    }

    AutoJSAPI jsapi;
    JSContext* cx = jsapi.cx();
    JSAutoCompartment ac(cx, global);

    
    nsRefPtr<AudioBuffer> renderedBuffer = new AudioBuffer(context,
                                                           mLength,
                                                           mSampleRate);
    if (!renderedBuffer->InitializeBuffers(mInputChannels.Length(), cx)) {
      return;
    }
    for (uint32_t i = 0; i < mInputChannels.Length(); ++i) {
      renderedBuffer->SetRawChannelContents(i, mInputChannels[i]);
    }

    nsRefPtr<OfflineAudioCompletionEvent> event =
        new OfflineAudioCompletionEvent(context, nullptr, nullptr);
    event->InitEvent(renderedBuffer);
    context->DispatchTrustedEvent(event);
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    size_t amount = AudioNodeEngine::SizeOfExcludingThis(aMallocSizeOf);
    amount += mInputChannels.SizeOfExcludingThis(aMallocSizeOf);
    return amount;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  
  
  
  InputChannels mInputChannels;
  
  uint32_t mWriteIndex;
  
  uint32_t mLength;
  float mSampleRate;
};

class DestinationNodeEngine : public AudioNodeEngine
{
public:
  explicit DestinationNodeEngine(AudioDestinationNode* aNode)
    : AudioNodeEngine(aNode)
    , mVolume(1.0f)
  {
  }

  virtual void ProcessBlock(AudioNodeStream* aStream,
                            const AudioChunk& aInput,
                            AudioChunk* aOutput,
                            bool* aFinished) MOZ_OVERRIDE
  {
    *aOutput = aInput;
    aOutput->mVolume *= mVolume;
  }

  virtual void SetDoubleParameter(uint32_t aIndex, double aParam) MOZ_OVERRIDE
  {
    if (aIndex == VOLUME) {
      mVolume = aParam;
    }
  }

  enum Parameters {
    VOLUME,
  };

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  float mVolume;
};

static bool UseAudioChannelService()
{
  return Preferences::GetBool("media.useAudioChannelService");
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(AudioDestinationNode, AudioNode,
                                   mAudioChannelAgent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioDestinationNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIAudioChannelAgentCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(AudioDestinationNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(AudioDestinationNode, AudioNode)

AudioDestinationNode::AudioDestinationNode(AudioContext* aContext,
                                           bool aIsOffline,
                                           AudioChannel aChannel,
                                           uint32_t aNumberOfChannels,
                                           uint32_t aLength,
                                           float aSampleRate)
  : AudioNode(aContext,
              aIsOffline ? aNumberOfChannels : 2,
              ChannelCountMode::Explicit,
              ChannelInterpretation::Speakers)
  , mFramesToProduce(aLength)
  , mAudioChannel(AudioChannel::Normal)
  , mIsOffline(aIsOffline)
  , mHasFinished(false)
  , mExtraCurrentTime(0)
  , mExtraCurrentTimeSinceLastStartedBlocking(0)
  , mExtraCurrentTimeUpdatedSinceLastStableState(false)
{
  MediaStreamGraph* graph = aIsOffline ?
                            MediaStreamGraph::CreateNonRealtimeInstance(aSampleRate) :
                            MediaStreamGraph::GetInstance();
  AudioNodeEngine* engine = aIsOffline ?
                            new OfflineDestinationNodeEngine(this, aNumberOfChannels,
                                                             aLength, aSampleRate) :
                            static_cast<AudioNodeEngine*>(new DestinationNodeEngine(this));

  mStream = graph->CreateAudioNodeStream(engine, MediaStreamGraph::EXTERNAL_STREAM);
  mStream->SetAudioChannelType(aChannel);
  mStream->AddMainThreadListener(this);
  mStream->AddAudioOutput(&gWebAudioOutputKey);

  if (aChannel != AudioChannel::Normal) {
    ErrorResult rv;
    SetMozAudioChannelType(aChannel, rv);
  }

  if (!aIsOffline && UseAudioChannelService()) {
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(GetOwner());
    if (target) {
      target->AddSystemEventListener(NS_LITERAL_STRING("visibilitychange"), this,
                                      true,
                                      false);
    }

    CreateAudioChannelAgent();
  }
}

size_t
AudioDestinationNode::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t amount = AudioNode::SizeOfExcludingThis(aMallocSizeOf);
  
  
  return amount;
}

size_t
AudioDestinationNode::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

void
AudioDestinationNode::DestroyMediaStream()
{
  if (mAudioChannelAgent && !Context()->IsOffline()) {
    mAudioChannelAgent->StopPlaying();
    mAudioChannelAgent = nullptr;

    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(GetOwner());
    NS_ENSURE_TRUE_VOID(target);

    target->RemoveSystemEventListener(NS_LITERAL_STRING("visibilitychange"), this,
                                       true);
  }

  if (!mStream)
    return;

  mStream->RemoveMainThreadListener(this);
  MediaStreamGraph* graph = mStream->Graph();
  if (graph->IsNonRealtime()) {
    MediaStreamGraph::DestroyNonRealtimeInstance(graph);
  }
  AudioNode::DestroyMediaStream();
}

void
AudioDestinationNode::NotifyMainThreadStateChanged()
{
  if (mStream->IsFinished() && !mHasFinished) {
    mHasFinished = true;
    if (mIsOffline) {
      nsCOMPtr<nsIRunnable> runnable =
        NS_NewRunnableMethod(this, &AudioDestinationNode::FireOfflineCompletionEvent);
      NS_DispatchToCurrentThread(runnable);
    }
  }
}

void
AudioDestinationNode::FireOfflineCompletionEvent()
{
  AudioNodeStream* stream = static_cast<AudioNodeStream*>(Stream());
  OfflineDestinationNodeEngine* engine =
    static_cast<OfflineDestinationNodeEngine*>(stream->Engine());
  engine->FireOfflineCompletionEvent(this);
}

uint32_t
AudioDestinationNode::MaxChannelCount() const
{
  return Context()->MaxChannelCount();
}

void
AudioDestinationNode::SetChannelCount(uint32_t aChannelCount, ErrorResult& aRv)
{
  if (aChannelCount > MaxChannelCount()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  AudioNode::SetChannelCount(aChannelCount, aRv);
}

void
AudioDestinationNode::Mute()
{
  MOZ_ASSERT(Context() && !Context()->IsOffline());
  SendDoubleParameterToStream(DestinationNodeEngine::VOLUME, 0.0f);
}

void
AudioDestinationNode::Unmute()
{
  MOZ_ASSERT(Context() && !Context()->IsOffline());
  SendDoubleParameterToStream(DestinationNodeEngine::VOLUME, 1.0f);
}

void
AudioDestinationNode::OfflineShutdown()
{
  MOZ_ASSERT(Context() && Context()->IsOffline(),
             "Should only be called on a valid OfflineAudioContext");

  MediaStreamGraph::DestroyNonRealtimeInstance(mStream->Graph());
  mOfflineRenderingRef.Drop(this);
}

JSObject*
AudioDestinationNode::WrapObject(JSContext* aCx)
{
  return AudioDestinationNodeBinding::Wrap(aCx, this);
}

void
AudioDestinationNode::StartRendering()
{
  mOfflineRenderingRef.Take(this);
  mStream->Graph()->StartNonRealtimeProcessing(TrackRate(Context()->SampleRate()), mFramesToProduce);
}

void
AudioDestinationNode::SetCanPlay(bool aCanPlay)
{
  mStream->SetTrackEnabled(AudioNodeStream::AUDIO_TRACK, aCanPlay);
}

NS_IMETHODIMP
AudioDestinationNode::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString type;
  aEvent->GetType(type);

  if (!type.EqualsLiteral("visibilitychange")) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocShell> docshell = do_GetInterface(GetOwner());
  NS_ENSURE_TRUE(docshell, NS_ERROR_FAILURE);

  bool isActive = false;
  docshell->GetIsActive(&isActive);

  mAudioChannelAgent->SetVisibilityState(isActive);
  return NS_OK;
}

NS_IMETHODIMP
AudioDestinationNode::CanPlayChanged(int32_t aCanPlay)
{
  SetCanPlay(aCanPlay == AudioChannelState::AUDIO_CHANNEL_STATE_NORMAL);
  return NS_OK;
}

NS_IMETHODIMP
AudioDestinationNode::WindowVolumeChanged()
{
  MOZ_ASSERT(mAudioChannelAgent);

  if (!mStream) {
    return NS_OK;
  }

  float volume;
  nsresult rv = mAudioChannelAgent->GetWindowVolume(&volume);
  NS_ENSURE_SUCCESS(rv, rv);

  mStream->SetAudioOutputVolume(&gWebAudioOutputKey, volume);
  return NS_OK;
}

AudioChannel
AudioDestinationNode::MozAudioChannelType() const
{
  return mAudioChannel;
}

void
AudioDestinationNode::SetMozAudioChannelType(AudioChannel aValue, ErrorResult& aRv)
{
  if (Context()->IsOffline()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (aValue != mAudioChannel &&
      CheckAudioChannelPermissions(aValue)) {
    mAudioChannel = aValue;

    if (mAudioChannelAgent) {
      CreateAudioChannelAgent();
    }
  }
}

bool
AudioDestinationNode::CheckAudioChannelPermissions(AudioChannel aValue)
{
  if (!Preferences::GetBool("media.useAudioChannelService")) {
    return true;
  }

  
  if (aValue == AudioChannel::Normal) {
    return true;
  }

  
  if (aValue == AudioChannelService::GetDefaultAudioChannel()) {
    return true;
  }

  nsCOMPtr<nsIPermissionManager> permissionManager =
    services::GetPermissionManager();
  if (!permissionManager) {
    return false;
  }

  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(GetOwner());
  NS_ASSERTION(sop, "Window didn't QI to nsIScriptObjectPrincipal!");
  nsCOMPtr<nsIPrincipal> principal = sop->GetPrincipal();

  uint32_t perm = nsIPermissionManager::UNKNOWN_ACTION;

  nsCString channel;
  channel.AssignASCII(AudioChannelValues::strings[uint32_t(aValue)].value,
                      AudioChannelValues::strings[uint32_t(aValue)].length);
  permissionManager->TestExactPermissionFromPrincipal(principal,
    nsCString(NS_LITERAL_CSTRING("audio-channel-") + channel).get(),
    &perm);

  return perm == nsIPermissionManager::ALLOW_ACTION;
}

void
AudioDestinationNode::CreateAudioChannelAgent()
{
  if (mAudioChannelAgent) {
    mAudioChannelAgent->StopPlaying();
  }

  mAudioChannelAgent = new AudioChannelAgent();
  mAudioChannelAgent->InitWithWeakCallback(GetOwner(),
                                           static_cast<int32_t>(mAudioChannel),
                                           this);

  nsCOMPtr<nsIDocShell> docshell = do_GetInterface(GetOwner());
  if (docshell) {
    bool isActive = false;
    docshell->GetIsActive(&isActive);
    mAudioChannelAgent->SetVisibilityState(isActive);
  }

  int32_t state = 0;
  mAudioChannelAgent->StartPlaying(&state);
  SetCanPlay(state == AudioChannelState::AUDIO_CHANNEL_STATE_NORMAL);
}

void
AudioDestinationNode::NotifyStableState()
{
  mExtraCurrentTimeUpdatedSinceLastStableState = false;
}

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

void
AudioDestinationNode::ScheduleStableStateNotification()
{
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &AudioDestinationNode::NotifyStableState);
    appShell->RunInStableState(event);
  }
}

double
AudioDestinationNode::ExtraCurrentTime()
{
  if (!mStartedBlockingDueToBeingOnlyNode.IsNull() &&
      !mExtraCurrentTimeUpdatedSinceLastStableState) {
    mExtraCurrentTimeUpdatedSinceLastStableState = true;
    mExtraCurrentTimeSinceLastStartedBlocking =
      (TimeStamp::Now() - mStartedBlockingDueToBeingOnlyNode).ToSeconds();
    ScheduleStableStateNotification();
  }
  return mExtraCurrentTime + mExtraCurrentTimeSinceLastStartedBlocking;
}

void
AudioDestinationNode::SetIsOnlyNodeForContext(bool aIsOnlyNode)
{
  if (!mStartedBlockingDueToBeingOnlyNode.IsNull() == aIsOnlyNode) {
    
    return;
  }

  if (!mStream) {
    
    return;
  }

  if (mIsOffline) {
    
    
    
    
    return;
  }

  if (aIsOnlyNode) {
    mStream->ChangeExplicitBlockerCount(1);
    mStartedBlockingDueToBeingOnlyNode = TimeStamp::Now();
    
    mExtraCurrentTimeUpdatedSinceLastStableState = true;
    ScheduleStableStateNotification();
  } else {
    
    ExtraCurrentTime();
    mExtraCurrentTime += mExtraCurrentTimeSinceLastStartedBlocking;
    mExtraCurrentTimeSinceLastStartedBlocking = 0;
    mStream->ChangeExplicitBlockerCount(-1);
    mStartedBlockingDueToBeingOnlyNode = TimeStamp();
  }
}

}

}
