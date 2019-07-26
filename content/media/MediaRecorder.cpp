





#include "MediaRecorder.h"
#include "GeneratedEvents.h"
#include "MediaEncoder.h"
#include "nsDOMEventTargetHelper.h"
#include "nsError.h"
#include "nsIDocument.h"
#include "nsIDOMRecordErrorEvent.h"
#include "nsTArray.h"
#include "DOMMediaStream.h"
#include "EncodedBufferCache.h"
#include "nsIDOMFile.h"
#include "mozilla/dom/BlobEvent.h"

namespace mozilla {

namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(MediaRecorder, nsDOMEventTargetHelper,
                                     mStream)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaRecorder)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(MediaRecorder, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaRecorder, nsDOMEventTargetHelper)
































class MediaRecorder::Session
{
  
  
  class PushBlobRunnable : public nsRunnable
  {
  public:
    PushBlobRunnable(Session* aSession)
      : mSession(aSession)
    { }

    NS_IMETHODIMP Run()
    {
      MOZ_ASSERT(NS_IsMainThread());

      MediaRecorder *recorder = mSession->mRecorder;
      nsresult rv = recorder->CreateAndDispatchBlobEvent(mSession);
      if (NS_FAILED(rv)) {
        recorder->NotifyError(rv);
      }

      return NS_OK;
    }

  private:
    Session *mSession;
  };

  
  
  class ExtractRunnable : public nsRunnable
  {
  public:
    ExtractRunnable(Session *aSession)
      : mSession(aSession) {}

    NS_IMETHODIMP Run()
    {
      MOZ_ASSERT(NS_GetCurrentThread() == mSession->mReadThread);

      mSession->Extract();
      return NS_OK;
    }

  private:
    Session *mSession;
  };

  
  
  class DestroyRunnable : public nsRunnable
  {
  public:
    DestroyRunnable(Session *aSession)
      : mSession(aSession) {}

    NS_IMETHODIMP Run()
    {
      MOZ_ASSERT(NS_IsMainThread() && mSession.get());
      MediaRecorder *recorder = mSession->mRecorder;

      
      
      if (recorder->mState != RecordingState::Inactive) {
        ErrorResult result;
        recorder->Stop(result);
        NS_DispatchToMainThread(new DestroyRunnable(mSession.forget()));
        return NS_OK;
      }

      
      recorder->DispatchSimpleEvent(NS_LITERAL_STRING("stop"));
      recorder->SetMimeType(NS_LITERAL_STRING(""));

      
      mSession = nullptr;

      return NS_OK;
    }

  private:
    nsAutoPtr<Session> mSession;
  };

  friend class PushBlobRunnable;
  friend class ExtractRunnable;
  friend class DestroyRunnable;

public:
  Session(MediaRecorder* aRecorder, int32_t aTimeSlice)
    : mRecorder(aRecorder),
      mTimeSlice(aTimeSlice)
  {
    MOZ_ASSERT(NS_IsMainThread());

    mEncodedBufferCache = new EncodedBufferCache(MAX_ALLOW_MEMORY_BUFFER);
  }

  
  ~Session()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (mInputPort.get()) {
      mInputPort->Destroy();
    }

    if (mTrackUnionStream.get()) {
      mTrackUnionStream->Destroy();
    }
  }

  void Start()
  {
    MOZ_ASSERT(NS_IsMainThread());

    SetupStreams();

    
    if (!mReadThread) {
      nsresult rv = NS_NewNamedThread("Media Encoder", getter_AddRefs(mReadThread));
      if (NS_FAILED(rv)) {
        mRecorder->NotifyError(rv);
        return;
      }
    }

    mReadThread->Dispatch(new ExtractRunnable(this), NS_DISPATCH_NORMAL);
  }

  void Stop()
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    if (mInputPort.get())
    {
      mInputPort->Destroy();
      mInputPort = nullptr;
    }

    if (mTrackUnionStream.get())
    {
      mTrackUnionStream->Destroy();
      mTrackUnionStream = nullptr;
    }
  }

  void Pause()
  {
    MOZ_ASSERT(NS_IsMainThread() && mTrackUnionStream);

    mTrackUnionStream->ChangeExplicitBlockerCount(-1);
  }

  void Resume()
  {
    MOZ_ASSERT(NS_IsMainThread() && mTrackUnionStream);

    mTrackUnionStream->ChangeExplicitBlockerCount(1);
  }

  already_AddRefed<nsIDOMBlob> GetEncodedData()
  {
    nsString mimeType;
    mRecorder->GetMimeType(mimeType);
    return mEncodedBufferCache->ExtractBlob(mimeType);
  }

private:

  
  
  void Extract()
  {
    MOZ_ASSERT(NS_GetCurrentThread() == mReadThread);

    TimeStamp lastBlobTimeStamp = TimeStamp::Now();
    
    const bool pushBlob = (mTimeSlice > 0) ? true : false;

    do {
      
      nsTArray<nsTArray<uint8_t> > encodedBuf;
      nsString mimeType;
      mEncoder->GetEncodedData(&encodedBuf, mimeType);

      mRecorder->SetMimeType(mimeType);

      
      for (uint32_t i = 0; i < encodedBuf.Length(); i++) {
        mEncodedBufferCache->AppendBuffer(encodedBuf[i]);
      }

      if (pushBlob) {
        if ((TimeStamp::Now() - lastBlobTimeStamp).ToMilliseconds() > mTimeSlice) {
          NS_DispatchToMainThread(new PushBlobRunnable(this));
          lastBlobTimeStamp = TimeStamp::Now();
        }
      }
    } while (!mEncoder->IsShutdown());

    
    NS_DispatchToMainThread(new PushBlobRunnable(this));

    
    NS_DispatchToMainThread(new DestroyRunnable(this));
  }

  
  void SetupStreams()
  {
    MOZ_ASSERT(NS_IsMainThread());

    MediaStreamGraph* gm = mRecorder->mStream->GetStream()->Graph();
    mTrackUnionStream = gm->CreateTrackUnionStream(nullptr);
    MOZ_ASSERT(mTrackUnionStream, "CreateTrackUnionStream failed");

    mTrackUnionStream->SetAutofinish(true);

    mInputPort = mTrackUnionStream->AllocateInputPort(mRecorder->mStream->GetStream(), MediaInputPort::FLAG_BLOCK_OUTPUT);

    
    mEncoder = MediaEncoder::CreateEncoder(NS_LITERAL_STRING(""));
    MOZ_ASSERT(mEncoder, "CreateEncoder failed");

    if (mEncoder) {
      mTrackUnionStream->AddListener(mEncoder);
    }
  }

private:
  
  
  nsRefPtr<MediaRecorder> mRecorder;

  
  nsRefPtr<ProcessedMediaStream> mTrackUnionStream;
  nsRefPtr<MediaInputPort> mInputPort;

  
  nsCOMPtr<nsIThread> mReadThread;
  
  nsRefPtr<MediaEncoder> mEncoder;
  
  nsAutoPtr<EncodedBufferCache> mEncodedBufferCache;
  
  
  
  
  const int32_t mTimeSlice;
};

MediaRecorder::~MediaRecorder()
{
  MOZ_ASSERT(mSession == nullptr);
}

void
MediaRecorder::Init(nsPIDOMWindow* aOwnerWindow)
{
  MOZ_ASSERT(aOwnerWindow);
  MOZ_ASSERT(aOwnerWindow->IsInnerWindow());
  BindToOwner(aOwnerWindow);
}

MediaRecorder::MediaRecorder(DOMMediaStream& aStream)
  : mState(RecordingState::Inactive),
    mSession(nullptr),
    mMutex("Session.Data.Mutex")
{
  mStream = &aStream;
  SetIsDOMBinding();
}

void
MediaRecorder::SetMimeType(const nsString &aMimeType)
{
  MutexAutoLock lock(mMutex);
  mMimeType = aMimeType;
}

void
MediaRecorder::GetMimeType(nsString &aMimeType)
{
  MutexAutoLock lock(mMutex);
  aMimeType = mMimeType;
}

void
MediaRecorder::Start(const Optional<int32_t>& aTimeSlice, ErrorResult& aResult)
{
  if (mState != RecordingState::Inactive) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (mStream->GetStream()->IsFinished() || mStream->GetStream()->IsDestroyed()) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  int32_t timeSlice = 0;
  if (aTimeSlice.WasPassed()) {
    if (aTimeSlice.Value() < 0) {
      aResult.Throw(NS_ERROR_INVALID_ARG);
      return;
    }

    timeSlice = aTimeSlice.Value();
  }

  if (!CheckPrincipal()) {
    aResult.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  mState = RecordingState::Recording;
  
  mSession = new Session(this, timeSlice);
  mSession->Start();
}

void
MediaRecorder::Stop(ErrorResult& aResult)
{
  if (mState == RecordingState::Inactive) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  mSession->Stop();
  mSession = nullptr;

  mState = RecordingState::Inactive;
}

void
MediaRecorder::Pause(ErrorResult& aResult)
{
  if (mState != RecordingState::Recording) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  mState = RecordingState::Paused;

  MOZ_ASSERT(mSession != nullptr);
  if (mSession) {
    mSession->Pause();
    mState = RecordingState::Paused;
  }
}

void
MediaRecorder::Resume(ErrorResult& aResult)
{
  if (mState != RecordingState::Paused) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  MOZ_ASSERT(mSession != nullptr);
  if (mSession) {
    mSession->Resume();
    mState = RecordingState::Recording;
  }
}

void
MediaRecorder::RequestData(ErrorResult& aResult)
{
  if (mState != RecordingState::Recording) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  NS_DispatchToMainThread(
    NS_NewRunnableMethodWithArg<Session *>(this,
                                           &MediaRecorder::CreateAndDispatchBlobEvent, mSession),
    NS_DISPATCH_NORMAL);
}

JSObject*
MediaRecorder::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MediaRecorderBinding::Wrap(aCx, aScope, this);
}

 already_AddRefed<MediaRecorder>
MediaRecorder::Constructor(const GlobalObject& aGlobal,
                           DOMMediaStream& aStream, ErrorResult& aRv)
{
  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aGlobal.GetAsSupports());
  if (!sgo) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsCOMPtr<nsPIDOMWindow> ownerWindow = do_QueryInterface(aGlobal.GetAsSupports());
  if (!ownerWindow) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<MediaRecorder> object = new MediaRecorder(aStream);
  object->Init(ownerWindow);
  return object.forget();
}

nsresult
MediaRecorder::CreateAndDispatchBlobEvent(Session *aSession)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");

  if (!CheckPrincipal()) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  BlobEventInitInitializer init;
  init.mBubbles = false;
  init.mCancelable = false;
  init.mData = aSession->GetEncodedData();
  nsRefPtr<BlobEvent> event =
    BlobEvent::Constructor(this,
                           NS_LITERAL_STRING("dataavailable"),
                           init);
  event->SetTrusted(true);
  return DispatchDOMEvent(nullptr, event, nullptr, nullptr);
}

void
MediaRecorder::DispatchSimpleEvent(const nsAString & aStr)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMEvent(getter_AddRefs(event), this, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create the error event!!!");
    return;
  }
  rv = event->InitEvent(aStr, false, false);

  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to init the error event!!!");
    return;
  }

  event->SetTrusted(true);

  rv = DispatchDOMEvent(nullptr, event, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to dispatch the event!!!");
    return;
  }
}

void
MediaRecorder::NotifyError(nsresult aRv)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");
  nsresult rv = CheckInnerWindowCorrectness();
  if (NS_FAILED(rv)) {
    return;
  }
  nsString errorMsg;
  switch (aRv) {
  case NS_ERROR_DOM_SECURITY_ERR:
    errorMsg = NS_LITERAL_STRING("SecurityError");
    break;
  case NS_ERROR_OUT_OF_MEMORY:
    errorMsg = NS_LITERAL_STRING("OutOfMemoryError");
    break;
  default:
    errorMsg = NS_LITERAL_STRING("GenericError");
  }

  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMRecordErrorEvent(getter_AddRefs(event), this, nullptr, nullptr);

  nsCOMPtr<nsIDOMRecordErrorEvent> errorEvent = do_QueryInterface(event);
  rv = errorEvent->InitRecordErrorEvent(NS_LITERAL_STRING("error"),
                                        false, false, errorMsg);

  event->SetTrusted(true);
  rv = DispatchDOMEvent(nullptr, event, nullptr, nullptr);
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to dispatch the error event!!!");
    return;
  }
  return;
}

bool MediaRecorder::CheckPrincipal()
{
  nsCOMPtr<nsIPrincipal> principal = mStream->GetPrincipal();
  if (!GetOwner())
    return false;
  nsCOMPtr<nsIDocument> doc = GetOwner()->GetExtantDoc();
  if (!doc || !principal)
    return false;

  bool subsumes;
  if (NS_FAILED(doc->NodePrincipal()->Subsumes(principal, &subsumes)))
    return false;

  return subsumes;
}

}
}
