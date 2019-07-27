





#include "AudioSegment.h"
#include "nsSpeechTask.h"
#include "SpeechSynthesis.h"



#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#undef LOG
#ifdef PR_LOGGING
extern PRLogModuleInfo* GetSpeechSynthLog();
#define LOG(type, msg) PR_LOG(GetSpeechSynthLog(), type, msg)
#else
#define LOG(type, msg)
#endif

namespace mozilla {
namespace dom {

class SynthStreamListener : public MediaStreamListener
{
public:
  explicit SynthStreamListener(nsSpeechTask* aSpeechTask) :
    mSpeechTask(aSpeechTask),
    mStarted(false)
  {
  }

  void DoNotifyStarted()
  {
    if (mSpeechTask) {
      mSpeechTask->DispatchStartImpl();
    }
  }

  void DoNotifyFinished()
  {
    if (mSpeechTask) {
      mSpeechTask->DispatchEndImpl(mSpeechTask->GetCurrentTime(),
                                   mSpeechTask->GetCurrentCharOffset());
    }
  }

  virtual void NotifyEvent(MediaStreamGraph* aGraph,
                           MediaStreamListener::MediaStreamGraphEvent event) override
  {
    switch (event) {
      case EVENT_FINISHED:
        {
          nsCOMPtr<nsIRunnable> runnable =
            NS_NewRunnableMethod(this, &SynthStreamListener::DoNotifyFinished);
          aGraph->DispatchToMainThreadAfterStreamStateUpdate(runnable.forget());
        }
        break;
      case EVENT_REMOVED:
        mSpeechTask = nullptr;
        break;
      default:
        break;
    }
  }

  virtual void NotifyBlockingChanged(MediaStreamGraph* aGraph, Blocking aBlocked) override
  {
    if (aBlocked == MediaStreamListener::UNBLOCKED && !mStarted) {
      mStarted = true;
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(this, &SynthStreamListener::DoNotifyStarted);
      aGraph->DispatchToMainThreadAfterStreamStateUpdate(event.forget());
    }
  }

private:
  
  
  nsSpeechTask* mSpeechTask;

  bool mStarted;
};



NS_IMPL_CYCLE_COLLECTION(nsSpeechTask, mSpeechSynthesis, mUtterance);

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSpeechTask)
  NS_INTERFACE_MAP_ENTRY(nsISpeechTask)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISpeechTask)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSpeechTask)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSpeechTask)

nsSpeechTask::nsSpeechTask(SpeechSynthesisUtterance* aUtterance)
  : mUtterance(aUtterance)
  , mCallback(nullptr)
  , mIndirectAudio(false)
{
  mText = aUtterance->mText;
  mVolume = aUtterance->Volume();
}

nsSpeechTask::nsSpeechTask(float aVolume, const nsAString& aText)
  : mUtterance(nullptr)
  , mVolume(aVolume)
  , mText(aText)
  , mCallback(nullptr)
  , mIndirectAudio(false)
{
}

nsSpeechTask::~nsSpeechTask()
{
  LOG(PR_LOG_DEBUG, ("~nsSpeechTask"));
  if (mStream) {
    if (!mStream->IsDestroyed()) {
      mStream->Destroy();
    }

    mStream = nullptr;
  }

  if (mPort) {
    mPort->Destroy();
    mPort = nullptr;
  }
}

void
nsSpeechTask::BindStream(ProcessedMediaStream* aStream)
{
  mStream = MediaStreamGraph::GetInstance()->CreateSourceStream(nullptr);
  mPort = aStream->AllocateInputPort(mStream, 0);
}

NS_IMETHODIMP
nsSpeechTask::Setup(nsISpeechTaskCallback* aCallback,
                    uint32_t aChannels, uint32_t aRate, uint8_t argc)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  LOG(PR_LOG_DEBUG, ("nsSpeechTask::Setup"));

  mCallback = aCallback;

  if (mIndirectAudio) {
    if (argc > 0) {
      NS_WARNING("Audio info arguments in Setup() are ignored for indirect audio services.");
    }
    return NS_OK;
  }

  
  MOZ_ASSERT(mStream);

  mStream->AddListener(new SynthStreamListener(this));

  
  NS_ENSURE_TRUE(aChannels == 1, NS_ERROR_FAILURE);

  mChannels = aChannels;

  AudioSegment* segment = new AudioSegment();
  mStream->AddAudioTrack(1, aRate, 0, segment);
  mStream->AddAudioOutput(this);
  mStream->SetAudioOutputVolume(this, mVolume);

  return NS_OK;
}

static nsRefPtr<mozilla::SharedBuffer>
makeSamples(int16_t* aData, uint32_t aDataLen)
{
  nsRefPtr<mozilla::SharedBuffer> samples =
    SharedBuffer::Create(aDataLen * sizeof(int16_t));
  int16_t* frames = static_cast<int16_t*>(samples->Data());

  for (uint32_t i = 0; i < aDataLen; i++) {
    frames[i] = aData[i];
  }

  return samples;
}

NS_IMETHODIMP
nsSpeechTask::SendAudio(JS::Handle<JS::Value> aData, JS::Handle<JS::Value> aLandmarks,
                        JSContext* aCx)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  NS_ENSURE_TRUE(mStream, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_FALSE(mStream->IsDestroyed(), NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_TRUE(mChannels, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(aData.isObject(), NS_ERROR_INVALID_ARG);

  if (mIndirectAudio) {
    NS_WARNING("Can't call SendAudio from an indirect audio speech service.");
    return NS_ERROR_FAILURE;
  }

  JS::Rooted<JSObject*> darray(aCx, &aData.toObject());
  JSAutoCompartment ac(aCx, darray);

  JS::Rooted<JSObject*> tsrc(aCx, nullptr);

  
  if (JS_IsInt16Array(darray)) {
    tsrc = darray;
  } else if (JS_IsArrayObject(aCx, darray)) {
    tsrc = JS_NewInt16ArrayFromArray(aCx, darray);
  }

  if (!tsrc) {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }

  uint32_t dataLen = JS_GetTypedArrayLength(tsrc);
  nsRefPtr<mozilla::SharedBuffer> samples;
  {
    JS::AutoCheckCannotGC nogc;
    samples = makeSamples(JS_GetInt16ArrayData(tsrc, nogc), dataLen);
  }
  SendAudioImpl(samples, dataLen);

  return NS_OK;
}

NS_IMETHODIMP
nsSpeechTask::SendAudioNative(int16_t* aData, uint32_t aDataLen)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  NS_ENSURE_TRUE(mStream, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_FALSE(mStream->IsDestroyed(), NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_TRUE(mChannels, NS_ERROR_FAILURE);

  if (mIndirectAudio) {
    NS_WARNING("Can't call SendAudio from an indirect audio speech service.");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<mozilla::SharedBuffer> samples = makeSamples(aData, aDataLen);
  SendAudioImpl(samples, aDataLen);

  return NS_OK;
}

void
nsSpeechTask::SendAudioImpl(nsRefPtr<mozilla::SharedBuffer>& aSamples, uint32_t aDataLen)
{
  if (aDataLen == 0) {
    mStream->EndAllTrackAndFinish();
    return;
  }

  AudioSegment segment;
  nsAutoTArray<const int16_t*, 1> channelData;
  channelData.AppendElement(static_cast<int16_t*>(aSamples->Data()));
  segment.AppendFrames(aSamples.forget(), channelData, aDataLen);
  mStream->AppendToTrack(1, &segment);
  mStream->AdvanceKnownTracksTime(STREAM_TIME_MAX);
}

NS_IMETHODIMP
nsSpeechTask::DispatchStart()
{
  if (!mIndirectAudio) {
    NS_WARNING("Can't call DispatchStart() from a direct audio speech service");
    return NS_ERROR_FAILURE;
  }

  return DispatchStartImpl();
}

nsresult
nsSpeechTask::DispatchStartImpl()
{
  LOG(PR_LOG_DEBUG, ("nsSpeechTask::DispatchStart"));

  MOZ_ASSERT(mUtterance);
  NS_ENSURE_TRUE(mUtterance->mState == SpeechSynthesisUtterance::STATE_PENDING,
                 NS_ERROR_NOT_AVAILABLE);

  mUtterance->mState = SpeechSynthesisUtterance::STATE_SPEAKING;
  mUtterance->DispatchSpeechSynthesisEvent(NS_LITERAL_STRING("start"), 0, 0,
                                           NS_LITERAL_STRING(""));

  return NS_OK;
}

NS_IMETHODIMP
nsSpeechTask::DispatchEnd(float aElapsedTime, uint32_t aCharIndex)
{
  if (!mIndirectAudio) {
    NS_WARNING("Can't call DispatchEnd() from a direct audio speech service");
    return NS_ERROR_FAILURE;
  }

  return DispatchEndImpl(aElapsedTime, aCharIndex);
}

nsresult
nsSpeechTask::DispatchEndImpl(float aElapsedTime, uint32_t aCharIndex)
{
  LOG(PR_LOG_DEBUG, ("nsSpeechTask::DispatchEnd\n"));

  MOZ_ASSERT(mUtterance);
  NS_ENSURE_FALSE(mUtterance->mState == SpeechSynthesisUtterance::STATE_ENDED,
                  NS_ERROR_NOT_AVAILABLE);

  
  if (mStream) {
    mStream->Destroy();
  }

  nsRefPtr<SpeechSynthesisUtterance> utterance = mUtterance;

  if (mSpeechSynthesis) {
    mSpeechSynthesis->OnEnd(this);
  }

  if (utterance->mState == SpeechSynthesisUtterance::STATE_PENDING) {
    utterance->mState = SpeechSynthesisUtterance::STATE_NONE;
  } else {
    utterance->mState = SpeechSynthesisUtterance::STATE_ENDED;
    utterance->DispatchSpeechSynthesisEvent(NS_LITERAL_STRING("end"),
                                            aCharIndex, aElapsedTime,
                                            EmptyString());
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSpeechTask::DispatchPause(float aElapsedTime, uint32_t aCharIndex)
{
  if (!mIndirectAudio) {
    NS_WARNING("Can't call DispatchPause() from a direct audio speech service");
    return NS_ERROR_FAILURE;
  }

  return DispatchPauseImpl(aElapsedTime, aCharIndex);
}

nsresult
nsSpeechTask::DispatchPauseImpl(float aElapsedTime, uint32_t aCharIndex)
{
  LOG(PR_LOG_DEBUG, ("nsSpeechTask::DispatchPause"));
  MOZ_ASSERT(mUtterance);
  NS_ENSURE_FALSE(mUtterance->mPaused, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_FALSE(mUtterance->mState == SpeechSynthesisUtterance::STATE_ENDED,
                  NS_ERROR_NOT_AVAILABLE);

  mUtterance->mPaused = true;
  mUtterance->DispatchSpeechSynthesisEvent(NS_LITERAL_STRING("pause"),
                                           aCharIndex, aElapsedTime,
                                           NS_LITERAL_STRING(""));
  return NS_OK;
}

NS_IMETHODIMP
nsSpeechTask::DispatchResume(float aElapsedTime, uint32_t aCharIndex)
{
  if (!mIndirectAudio) {
    NS_WARNING("Can't call DispatchResume() from a direct audio speech service");
    return NS_ERROR_FAILURE;
  }

  return DispatchResumeImpl(aElapsedTime, aCharIndex);
}

nsresult
nsSpeechTask::DispatchResumeImpl(float aElapsedTime, uint32_t aCharIndex)
{
  LOG(PR_LOG_DEBUG, ("nsSpeechTask::DispatchResume"));
  MOZ_ASSERT(mUtterance);
  NS_ENSURE_TRUE(mUtterance->mPaused, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_FALSE(mUtterance->mState == SpeechSynthesisUtterance::STATE_ENDED,
                  NS_ERROR_NOT_AVAILABLE);

  mUtterance->mPaused = false;
  mUtterance->DispatchSpeechSynthesisEvent(NS_LITERAL_STRING("resume"),
                                           aCharIndex, aElapsedTime,
                                           NS_LITERAL_STRING(""));
  return NS_OK;
}

NS_IMETHODIMP
nsSpeechTask::DispatchError(float aElapsedTime, uint32_t aCharIndex)
{
  if (!mIndirectAudio) {
    NS_WARNING("Can't call DispatchError() from a direct audio speech service");
    return NS_ERROR_FAILURE;
  }

  return DispatchErrorImpl(aElapsedTime, aCharIndex);
}

nsresult
nsSpeechTask::DispatchErrorImpl(float aElapsedTime, uint32_t aCharIndex)
{
  MOZ_ASSERT(mUtterance);
  NS_ENSURE_FALSE(mUtterance->mState == SpeechSynthesisUtterance::STATE_ENDED,
                  NS_ERROR_NOT_AVAILABLE);

  mUtterance->mState = SpeechSynthesisUtterance::STATE_ENDED;
  mUtterance->DispatchSpeechSynthesisEvent(NS_LITERAL_STRING("error"),
                                           aCharIndex, aElapsedTime,
                                           NS_LITERAL_STRING(""));
  return NS_OK;
}

NS_IMETHODIMP
nsSpeechTask::DispatchBoundary(const nsAString& aName,
                               float aElapsedTime, uint32_t aCharIndex)
{
  if (!mIndirectAudio) {
    NS_WARNING("Can't call DispatchBoundary() from a direct audio speech service");
    return NS_ERROR_FAILURE;
  }

  return DispatchBoundaryImpl(aName, aElapsedTime, aCharIndex);
}

nsresult
nsSpeechTask::DispatchBoundaryImpl(const nsAString& aName,
                                   float aElapsedTime, uint32_t aCharIndex)
{
  MOZ_ASSERT(mUtterance);
  NS_ENSURE_TRUE(mUtterance->mState == SpeechSynthesisUtterance::STATE_SPEAKING,
                 NS_ERROR_NOT_AVAILABLE);

  mUtterance->DispatchSpeechSynthesisEvent(NS_LITERAL_STRING("boundary"),
                                           aCharIndex, aElapsedTime,
                                           aName);
  return NS_OK;
}

NS_IMETHODIMP
nsSpeechTask::DispatchMark(const nsAString& aName,
                           float aElapsedTime, uint32_t aCharIndex)
{
  if (!mIndirectAudio) {
    NS_WARNING("Can't call DispatchMark() from a direct audio speech service");
    return NS_ERROR_FAILURE;
  }

  return DispatchMarkImpl(aName, aElapsedTime, aCharIndex);
}

nsresult
nsSpeechTask::DispatchMarkImpl(const nsAString& aName,
                               float aElapsedTime, uint32_t aCharIndex)
{
  MOZ_ASSERT(mUtterance);
  NS_ENSURE_TRUE(mUtterance->mState == SpeechSynthesisUtterance::STATE_SPEAKING,
                 NS_ERROR_NOT_AVAILABLE);

  mUtterance->DispatchSpeechSynthesisEvent(NS_LITERAL_STRING("mark"),
                                           aCharIndex, aElapsedTime,
                                           aName);
  return NS_OK;
}

void
nsSpeechTask::Pause()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  if (mCallback) {
    DebugOnly<nsresult> rv = mCallback->OnPause();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to call onPause() callback");
  }

  if (mStream) {
    mStream->ChangeExplicitBlockerCount(1);
    DispatchPauseImpl(GetCurrentTime(), GetCurrentCharOffset());
  }
}

void
nsSpeechTask::Resume()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  if (mCallback) {
    DebugOnly<nsresult> rv = mCallback->OnResume();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to call onResume() callback");
  }

  if (mStream) {
    mStream->ChangeExplicitBlockerCount(-1);
    DispatchResumeImpl(GetCurrentTime(), GetCurrentCharOffset());
  }
}

void
nsSpeechTask::Cancel()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  LOG(PR_LOG_DEBUG, ("nsSpeechTask::Cancel"));

  if (mCallback) {
    DebugOnly<nsresult> rv = mCallback->OnCancel();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to call onCancel() callback");
  }

  if (mStream) {
    mStream->ChangeExplicitBlockerCount(1);
    DispatchEndImpl(GetCurrentTime(), GetCurrentCharOffset());
  }
}

float
nsSpeechTask::GetCurrentTime()
{
  return mStream ? (float)(mStream->GetCurrentTime() / 1000000.0) : 0;
}

uint32_t
nsSpeechTask::GetCurrentCharOffset()
{
  return mStream && mStream->IsFinished() ? mText.Length() : 0;
}

void
nsSpeechTask::SetSpeechSynthesis(SpeechSynthesis* aSpeechSynthesis)
{
  mSpeechSynthesis = aSpeechSynthesis;
}

} 
} 
