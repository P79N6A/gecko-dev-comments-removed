





#include "AudioDestinationNode.h"
#include "mozilla/dom/AudioDestinationNodeBinding.h"
#include "mozilla/Preferences.h"
#include "AudioChannelAgent.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "MediaStreamGraph.h"
#include "OfflineAudioCompletionEvent.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShell.h"
#include "nsIPermissionManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {

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

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
                                 const AudioChunk& aInput,
                                 AudioChunk* aOutput,
                                 bool* aFinished) MOZ_OVERRIDE
  {
    
    
    *aOutput = aInput;

    
    if (mInputChannels.IsEmpty()) {
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

    if (mWriteIndex == mLength) {
      SendBufferToMainThread(aStream);
      *aFinished = true;
    }
  }

  void SendBufferToMainThread(AudioNodeStream* aStream)
  {
    class Command : public nsRunnable
    {
    public:
      Command(AudioNodeStream* aStream,
              InputChannels& aInputChannels,
              uint32_t aLength,
              float aSampleRate)
        : mStream(aStream)
        , mLength(aLength)
        , mSampleRate(aSampleRate)
      {
        mInputChannels.SwapElements(aInputChannels);
      }

      NS_IMETHODIMP Run()
      {
        
        if (!nsContentUtils::IsSafeToRunScript()) {
          nsContentUtils::AddScriptRunner(this);
          return NS_OK;
        }

        nsRefPtr<AudioContext> context;
        {
          MutexAutoLock lock(mStream->Engine()->NodeMutex());
          AudioNode* node = mStream->Engine()->Node();
          if (node) {
            context = node->Context();
            MOZ_ASSERT(context, "node hasn't kept context alive");
          }
        }
        if (!context) {
          return NS_OK;
        }
        context->Shutdown(); 

        AutoPushJSContext cx(context->GetJSContext());
        if (cx) {
          JSAutoRequest ar(cx);

          
          nsRefPtr<AudioBuffer> renderedBuffer = new AudioBuffer(context,
                                                                 mLength,
                                                                 mSampleRate);
          if (!renderedBuffer->InitializeBuffers(mInputChannels.Length(), cx)) {
            return NS_OK;
          }
          for (uint32_t i = 0; i < mInputChannels.Length(); ++i) {
            renderedBuffer->SetRawChannelContents(cx, i, mInputChannels[i]);
          }

          nsRefPtr<OfflineAudioCompletionEvent> event =
              new OfflineAudioCompletionEvent(context, nullptr, nullptr);
          event->InitEvent(renderedBuffer);
          context->DispatchTrustedEvent(event);
        }

        return NS_OK;
      }
    private:
      nsRefPtr<AudioNodeStream> mStream;
      InputChannels mInputChannels;
      uint32_t mLength;
      float mSampleRate;
    };

    
    
    NS_DispatchToMainThread(new Command(aStream, mInputChannels, mLength, mSampleRate));
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

  virtual void ProduceAudioBlock(AudioNodeStream* aStream,
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

private:
  float mVolume;
};

static bool UseAudioChannelService()
{
  return Preferences::GetBool("media.useAudioChannelService");
}

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(AudioDestinationNode, AudioNode,
                                     mAudioChannelAgent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioDestinationNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIAudioChannelAgentCallback)
NS_INTERFACE_MAP_END_INHERITING(AudioNode)

NS_IMPL_ADDREF_INHERITED(AudioDestinationNode, AudioNode)
NS_IMPL_RELEASE_INHERITED(AudioDestinationNode, AudioNode)

AudioDestinationNode::AudioDestinationNode(AudioContext* aContext,
                                           bool aIsOffline,
                                           uint32_t aNumberOfChannels,
                                           uint32_t aLength,
                                           float aSampleRate)
  : AudioNode(aContext,
              aIsOffline ? aNumberOfChannels : 2,
              ChannelCountMode::Explicit,
              ChannelInterpretation::Speakers)
  , mFramesToProduce(aLength)
  , mAudioChannel(AudioChannel::Normal)
{
  MediaStreamGraph* graph = aIsOffline ?
                            MediaStreamGraph::CreateNonRealtimeInstance() :
                            MediaStreamGraph::GetInstance();
  AudioNodeEngine* engine = aIsOffline ?
                            new OfflineDestinationNodeEngine(this, aNumberOfChannels,
                                                             aLength, aSampleRate) :
                            static_cast<AudioNodeEngine*>(new DestinationNodeEngine(this));

  mStream = graph->CreateAudioNodeStream(engine, MediaStreamGraph::EXTERNAL_STREAM);

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

  MediaStreamGraph* graph = mStream->Graph();
  if (graph->IsNonRealtime()) {
    MediaStreamGraph::DestroyNonRealtimeInstance(graph);
  }
  AudioNode::DestroyMediaStream();
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
AudioDestinationNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return AudioDestinationNodeBinding::Wrap(aCx, aScope, this);
}

void
AudioDestinationNode::StartRendering()
{
  mOfflineRenderingRef.Take(this);
  mStream->Graph()->StartNonRealtimeProcessing(mFramesToProduce);
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

  nsCOMPtr<nsIPermissionManager> permissionManager =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
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

  AudioChannelType type = AUDIO_CHANNEL_NORMAL;
  switch(mAudioChannel) {
    case AudioChannel::Normal:
      type = AUDIO_CHANNEL_NORMAL;
      break;

    case AudioChannel::Content:
      type = AUDIO_CHANNEL_CONTENT;
      break;

    case AudioChannel::Notification:
      type = AUDIO_CHANNEL_NOTIFICATION;
      break;

    case AudioChannel::Alarm:
      type = AUDIO_CHANNEL_ALARM;
      break;

    case AudioChannel::Telephony:
      type = AUDIO_CHANNEL_TELEPHONY;
      break;

    case AudioChannel::Ringer:
      type = AUDIO_CHANNEL_RINGER;
      break;

    case AudioChannel::Publicnotification:
      type = AUDIO_CHANNEL_PUBLICNOTIFICATION;
      break;

  }

  mAudioChannelAgent = new AudioChannelAgent();
  mAudioChannelAgent->InitWithWeakCallback(type, this);

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
}

}
