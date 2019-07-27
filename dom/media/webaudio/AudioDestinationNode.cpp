





#include "AudioDestinationNode.h"
#include "AudioContext.h"
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
#include "mozilla/dom/Promise.h"

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
    , mNumberOfChannels(aNumberOfChannels)
    , mLength(aLength)
    , mSampleRate(aSampleRate)
    , mBufferAllocated(false)
  {
  }

  virtual void ProcessBlock(AudioNodeStream* aStream,
                            const AudioChunk& aInput,
                            AudioChunk* aOutput,
                            bool* aFinished) override
  {
    
    
    *aOutput = aInput;

    
    if (!mBufferAllocated) {
      
      
      
      if (mInputChannels.SetLength(mNumberOfChannels)) {
        for (uint32_t i = 0; i < mNumberOfChannels; ++i) {
          mInputChannels[i] = new (fallible) float[mLength];
          if (!mInputChannels[i]) {
            mInputChannels.Clear();
            break;
          }
        }
      }

      mBufferAllocated = true;
    }

    
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

  class OnCompleteTask final : public nsRunnable
  {
  public:
    OnCompleteTask(AudioContext* aAudioContext, AudioBuffer* aRenderedBuffer)
      : mAudioContext(aAudioContext)
      , mRenderedBuffer(aRenderedBuffer)
    {}

    NS_IMETHOD Run()
    {
      nsRefPtr<OfflineAudioCompletionEvent> event =
          new OfflineAudioCompletionEvent(mAudioContext, nullptr, nullptr);
      event->InitEvent(mRenderedBuffer);
      mAudioContext->DispatchTrustedEvent(event);

      return NS_OK;
    }
  private:
    nsRefPtr<AudioContext> mAudioContext;
    nsRefPtr<AudioBuffer> mRenderedBuffer;
  };

  void FireOfflineCompletionEvent(AudioDestinationNode* aNode)
  {
    AudioContext* context = aNode->Context();
    context->Shutdown();
    
    
    

    AutoJSAPI jsapi;
    if (NS_WARN_IF(!jsapi.Init(aNode->GetOwner()))) {
      return;
    }
    JSContext* cx = jsapi.cx();

    
    ErrorResult rv;
    nsRefPtr<AudioBuffer> renderedBuffer =
      AudioBuffer::Create(context, mInputChannels.Length(),
                          mLength, mSampleRate, cx, rv);
    if (rv.Failed()) {
      return;
    }
    for (uint32_t i = 0; i < mInputChannels.Length(); ++i) {
      renderedBuffer->SetRawChannelContents(i, mInputChannels[i]);
    }

    aNode->ResolvePromise(renderedBuffer);

    nsRefPtr<OnCompleteTask> onCompleteTask =
      new OnCompleteTask(context, renderedBuffer);
    NS_DispatchToMainThread(onCompleteTask);

    context->OnStateChanged(nullptr, AudioContextState::Closed);
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    size_t amount = AudioNodeEngine::SizeOfExcludingThis(aMallocSizeOf);
    amount += mInputChannels.SizeOfExcludingThis(aMallocSizeOf);
    return amount;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  
  
  
  InputChannels mInputChannels;
  
  uint32_t mWriteIndex;
  uint32_t mNumberOfChannels;
  
  uint32_t mLength;
  float mSampleRate;
  bool mBufferAllocated;
};

class InputMutedRunnable : public nsRunnable
{
public:
  InputMutedRunnable(AudioNodeStream* aStream,
                     bool aInputMuted)
    : mStream(aStream)
    , mInputMuted(aInputMuted)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    nsRefPtr<AudioNode> node = mStream->Engine()->NodeMainThread();

    if (node) {
      nsRefPtr<AudioDestinationNode> destinationNode =
        static_cast<AudioDestinationNode*>(node.get());
      destinationNode->InputMuted(mInputMuted);
    }
    return NS_OK;
  }

private:
  nsRefPtr<AudioNodeStream> mStream;
  bool mInputMuted;
};

class DestinationNodeEngine : public AudioNodeEngine
{
public:
  explicit DestinationNodeEngine(AudioDestinationNode* aNode)
    : AudioNodeEngine(aNode)
    , mVolume(1.0f)
    , mLastInputMuted(false)
  {
    MOZ_ASSERT(aNode);
  }

  virtual void ProcessBlock(AudioNodeStream* aStream,
                            const AudioChunk& aInput,
                            AudioChunk* aOutput,
                            bool* aFinished) override
  {
    *aOutput = aInput;
    aOutput->mVolume *= mVolume;

    bool newInputMuted = aInput.IsNull() || aInput.IsMuted();
    if (newInputMuted != mLastInputMuted) {
      mLastInputMuted = newInputMuted;

      nsRefPtr<InputMutedRunnable> runnable =
        new InputMutedRunnable(aStream, newInputMuted);
      aStream->Graph()->
        DispatchToMainThreadAfterStreamStateUpdate(runnable.forget());
    }
  }

  virtual void SetDoubleParameter(uint32_t aIndex, double aParam) override
  {
    if (aIndex == VOLUME) {
      mVolume = aParam;
    }
  }

  enum Parameters {
    VOLUME,
  };

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  float mVolume;
  bool mLastInputMuted;
};

static bool UseAudioChannelService()
{
  return Preferences::GetBool("media.useAudioChannelService");
}

class EventProxyHandler final : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS

  explicit EventProxyHandler(nsIDOMEventListener* aNode)
  {
    MOZ_ASSERT(aNode);
    mWeakNode = do_GetWeakReference(aNode);
  }

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) override
  {
    nsCOMPtr<nsIDOMEventListener> listener = do_QueryReferent(mWeakNode);
    if (!listener) {
      return NS_OK;
    }

    auto node = static_cast<AudioDestinationNode*>(listener.get());
    return node->HandleEvent(aEvent);
  }

private:
  ~EventProxyHandler()
  { }

  nsWeakPtr mWeakNode;
};

NS_IMPL_ISUPPORTS(EventProxyHandler, nsIDOMEventListener)

NS_IMPL_CYCLE_COLLECTION_INHERITED(AudioDestinationNode, AudioNode,
                                   mAudioChannelAgent, mEventProxyHelper,
                                   mOfflineRenderingPromise)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(AudioDestinationNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIAudioChannelAgentCallback)
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
  , mAudioChannelAgentPlaying(false)
  , mExtraCurrentTime(0)
  , mExtraCurrentTimeSinceLastStartedBlocking(0)
  , mExtraCurrentTimeUpdatedSinceLastStableState(false)
{
  bool startWithAudioDriver = true;
  MediaStreamGraph* graph = aIsOffline ?
                            MediaStreamGraph::CreateNonRealtimeInstance(aSampleRate) :
                            MediaStreamGraph::GetInstance(startWithAudioDriver, aChannel);
  AudioNodeEngine* engine = aIsOffline ?
                            new OfflineDestinationNodeEngine(this, aNumberOfChannels,
                                                             aLength, aSampleRate) :
                            static_cast<AudioNodeEngine*>(new DestinationNodeEngine(this));

  mStream = graph->CreateAudioNodeStream(engine, MediaStreamGraph::EXTERNAL_STREAM);
  mStream->AddMainThreadListener(this);
  mStream->AddAudioOutput(&gWebAudioOutputKey);

  if (!aIsOffline) {
    graph->NotifyWhenGraphStarted(mStream->AsAudioNodeStream());
  }

  if (aChannel != AudioChannel::Normal) {
    ErrorResult rv;
    SetMozAudioChannelType(aChannel, rv);
  }
}

AudioDestinationNode::~AudioDestinationNode()
{
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

    target->RemoveSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                      mEventProxyHelper,
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

void
AudioDestinationNode::ResolvePromise(AudioBuffer* aRenderedBuffer)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mIsOffline);
  mOfflineRenderingPromise->MaybeResolve(aRenderedBuffer);
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
AudioDestinationNode::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return AudioDestinationNodeBinding::Wrap(aCx, this, aGivenProto);
}

void
AudioDestinationNode::StartRendering(Promise* aPromise)
{
  mOfflineRenderingPromise = aPromise;
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
  bool playing = aCanPlay == AudioChannelState::AUDIO_CHANNEL_STATE_NORMAL;
  if (playing == mAudioChannelAgentPlaying) {
    return NS_OK;
  }

  mAudioChannelAgentPlaying = playing;
  SetCanPlay(playing);

  Context()->DispatchTrustedEvent(
    playing ? NS_LITERAL_STRING("mozinterruptend")
            : NS_LITERAL_STRING("mozinterruptbegin"));

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

    if (mStream) {
      mStream->SetAudioChannelType(mAudioChannel);
    }

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
  if (mIsOffline || !UseAudioChannelService()) {
    return;
  }

  if (!mEventProxyHelper) {
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(GetOwner());
    if (target) {
      
      
      
      mEventProxyHelper = new EventProxyHandler(this);
      target->AddSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                     mEventProxyHelper,
                                      true,
                                      false);
    }
  }

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

    
    
    InputMuted(false);
  }
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

void
AudioDestinationNode::InputMuted(bool aMuted)
{
  MOZ_ASSERT(Context() && !Context()->IsOffline());

  if (!mAudioChannelAgent) {
    return;
  }

  if (aMuted) {
    mAudioChannelAgent->StopPlaying();
    return;
  }

  int32_t state = 0;
  nsresult rv = mAudioChannelAgent->StartPlaying(&state);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  CanPlayChanged(state);
}

} 
} 
