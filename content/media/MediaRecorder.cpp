





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


#include "mozilla/dom/AudioStreamTrack.h"
#include "mozilla/dom/VideoStreamTrack.h"

namespace mozilla {

namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_2(MediaRecorder, nsDOMEventTargetHelper,
                                     mStream, mSession)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaRecorder)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(MediaRecorder, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaRecorder, nsDOMEventTargetHelper)
































class MediaRecorder::Session: public nsIObserver
{
  NS_DECL_THREADSAFE_ISUPPORTS

  
  
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
      if (mSession->IsEncoderError()) {
        recorder->NotifyError(NS_ERROR_UNEXPECTED);
      }
      nsresult rv = recorder->CreateAndDispatchBlobEvent(mSession);
      if (NS_FAILED(rv)) {
        recorder->NotifyError(rv);
      }
      return NS_OK;
    }

  private:
    nsRefPtr<Session> mSession;
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
      if (!mSession->mEncoder->IsShutdown()) {
        NS_DispatchToCurrentThread(new ExtractRunnable(mSession));
      } else {
        
        NS_DispatchToMainThread(new PushBlobRunnable(mSession));
        
        NS_DispatchToMainThread(new DestroyRunnable(already_AddRefed<Session>(mSession)));
      }
      return NS_OK;
    }

  private:
    Session* mSession;
  };

  
  class TracksAvailableCallback : public DOMMediaStream::OnTracksAvailableCallback
  {
  public:
    TracksAvailableCallback(Session *aSession)
     : mSession(aSession) {}
    virtual void NotifyTracksAvailable(DOMMediaStream* aStream)
    {
      uint8_t trackType = aStream->GetHintContents();
      
      if (trackType == 0) {
        nsTArray<nsRefPtr<mozilla::dom::AudioStreamTrack> > audioTracks;
        aStream->GetAudioTracks(audioTracks);
        nsTArray<nsRefPtr<mozilla::dom::VideoStreamTrack> > videoTracks;
        aStream->GetVideoTracks(videoTracks);
        
        if (videoTracks.Length() > 0) {
          trackType |= DOMMediaStream::HINT_CONTENTS_VIDEO;
        }
        if (audioTracks.Length() > 0) {
          trackType |= DOMMediaStream::HINT_CONTENTS_AUDIO;
        }
      }
      mSession->AfterTracksAdded(trackType);
    }
  private:
    nsRefPtr<Session> mSession;
  };

  
  
  class DestroyRunnable : public nsRunnable
  {
  public:
    DestroyRunnable(const already_AddRefed<Session> &aSession)
      : mSession(aSession) {}

    NS_IMETHODIMP Run()
    {
      MOZ_ASSERT(NS_IsMainThread() && mSession.get());
      MediaRecorder *recorder = mSession->mRecorder;

      
      
      
      
      
      
      if (!mSession->mStopIssued) {
        ErrorResult result;
        recorder->Stop(result);
        NS_DispatchToMainThread(new DestroyRunnable(mSession.forget()));

        return NS_OK;
      }

      
      recorder->DispatchSimpleEvent(NS_LITERAL_STRING("stop"));
      mSession->mMimeType = NS_LITERAL_STRING("");
      recorder->SetMimeType(mSession->mMimeType);
      return NS_OK;
    }

  private:
    
    nsRefPtr<Session> mSession;
  };

  friend class PushBlobRunnable;
  friend class ExtractRunnable;
  friend class DestroyRunnable;
  friend class TracksAvailableCallback;

public:
  Session(MediaRecorder* aRecorder, int32_t aTimeSlice)
    : mRecorder(aRecorder),
      mTimeSlice(aTimeSlice),
      mStopIssued(false)
  {
    MOZ_ASSERT(NS_IsMainThread());

    AddRef();
    mEncodedBufferCache = new EncodedBufferCache(MAX_ALLOW_MEMORY_BUFFER);
    mLastBlobTimeStamp = TimeStamp::Now();
  }

  
  virtual ~Session()
  {
    CleanupStreams();
  }

  void Start()
  {
    MOZ_ASSERT(NS_IsMainThread());

    SetupStreams();
  }

  void Stop()
  {
    MOZ_ASSERT(NS_IsMainThread());

    mStopIssued = true;
    CleanupStreams();
    nsContentUtils::UnregisterShutdownObserver(this);
  }

  nsresult Pause()
  {
    NS_ENSURE_TRUE(NS_IsMainThread() && mTrackUnionStream, NS_ERROR_FAILURE);
    mTrackUnionStream->ChangeExplicitBlockerCount(-1);

    return NS_OK;
  }

  nsresult Resume()
  {
    NS_ENSURE_TRUE(NS_IsMainThread() && mTrackUnionStream, NS_ERROR_FAILURE);
    mTrackUnionStream->ChangeExplicitBlockerCount(1);

    return NS_OK;
  }

  already_AddRefed<nsIDOMBlob> GetEncodedData()
  {
    return mEncodedBufferCache->ExtractBlob(mMimeType);
  }

  bool IsEncoderError()
  {
    if (mEncoder && mEncoder->HasError()) {
      return true;
    }
    return false;
  }
private:

  
  
  void Extract()
  {
    MOZ_ASSERT(NS_GetCurrentThread() == mReadThread);

    
    const bool pushBlob = (mTimeSlice > 0) ? true : false;

    
    nsTArray<nsTArray<uint8_t> > encodedBuf;
    mEncoder->GetEncodedData(&encodedBuf, mMimeType);
    mRecorder->SetMimeType(mMimeType);

    
    for (uint32_t i = 0; i < encodedBuf.Length(); i++) {
      mEncodedBufferCache->AppendBuffer(encodedBuf[i]);
    }

    if (pushBlob) {
      if ((TimeStamp::Now() - mLastBlobTimeStamp).ToMilliseconds() > mTimeSlice) {
        NS_DispatchToMainThread(new PushBlobRunnable(this));
        mLastBlobTimeStamp = TimeStamp::Now();
      }
    }
  }

  
  void SetupStreams()
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    MediaStreamGraph* gm = mRecorder->mStream->GetStream()->Graph();
    mTrackUnionStream = gm->CreateTrackUnionStream(nullptr);
    MOZ_ASSERT(mTrackUnionStream, "CreateTrackUnionStream failed");

    mTrackUnionStream->SetAutofinish(true);

    
    mInputPort = mTrackUnionStream->AllocateInputPort(mRecorder->mStream->GetStream(), MediaInputPort::FLAG_BLOCK_OUTPUT);

    
    TracksAvailableCallback* tracksAvailableCallback = new TracksAvailableCallback(mRecorder->mSession);
    mRecorder->mStream->OnTracksAvailable(tracksAvailableCallback);
  }

  void AfterTracksAdded(uint8_t aTrackTypes)
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    
    mEncoder = MediaEncoder::CreateEncoder(NS_LITERAL_STRING(""), aTrackTypes);

    if (!mEncoder) {
      DoSessionEndTask(NS_ERROR_ABORT);
      return;
    }

    
    
    
    if (!mTrackUnionStream) {
      DoSessionEndTask(NS_OK);
      return;
    }
    mTrackUnionStream->AddListener(mEncoder);
    
    if (!mReadThread) {
      nsresult rv = NS_NewNamedThread("Media Encoder", getter_AddRefs(mReadThread));
      if (NS_FAILED(rv)) {
        DoSessionEndTask(rv);
        return;
      }
    }

    
    
    nsContentUtils::RegisterShutdownObserver(this);

    mReadThread->Dispatch(new ExtractRunnable(this), NS_DISPATCH_NORMAL);
  }
  
  void DoSessionEndTask(nsresult rv)
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (NS_FAILED(rv)) {
      mRecorder->NotifyError(rv);
    }
    CleanupStreams();
    
    NS_DispatchToMainThread(new PushBlobRunnable(this));
    NS_DispatchToMainThread(new DestroyRunnable(already_AddRefed<Session>(this)));
  }
  void CleanupStreams()
  {
    if (mInputPort.get()) {
      mInputPort->Destroy();
      mInputPort = nullptr;
    }

    if (mTrackUnionStream.get()) {
      mTrackUnionStream->Destroy();
      mTrackUnionStream = nullptr;
    }
  }

  NS_IMETHODIMP Observe(nsISupports *aSubject, const char *aTopic, const char16_t *aData)
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
      
      Stop();
    }

    return NS_OK;
  }

private:
  
  
  nsRefPtr<MediaRecorder> mRecorder;

  
  
  nsRefPtr<ProcessedMediaStream> mTrackUnionStream;
  nsRefPtr<MediaInputPort> mInputPort;

  
  nsCOMPtr<nsIThread> mReadThread;
  
  nsRefPtr<MediaEncoder> mEncoder;
  
  nsAutoPtr<EncodedBufferCache> mEncodedBufferCache;
  
  nsString mMimeType;
  
  TimeStamp mLastBlobTimeStamp;
  
  
  
  
  const int32_t mTimeSlice;
  
  bool mStopIssued;
};

NS_IMPL_ISUPPORTS1(MediaRecorder::Session, nsIObserver)

MediaRecorder::~MediaRecorder()
{
  MOZ_ASSERT(mSession == nullptr);
}

MediaRecorder::MediaRecorder(DOMMediaStream& aStream, nsPIDOMWindow* aOwnerWindow)
  : nsDOMEventTargetHelper(aOwnerWindow),
    mState(RecordingState::Inactive),
    mSession(nullptr),
    mMutex("Session.Data.Mutex")
{
  MOZ_ASSERT(aOwnerWindow);
  MOZ_ASSERT(aOwnerWindow->IsInnerWindow());
  mStream = &aStream;
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

  if (!mStream->GetPrincipal()) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (!CheckPrincipal()) {
    aResult.Throw(NS_ERROR_DOM_SECURITY_ERR);
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
  mState = RecordingState::Inactive;

  mSession->Stop();
  mSession = nullptr;
}

void
MediaRecorder::Pause(ErrorResult& aResult)
{
  if (mState != RecordingState::Recording) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  MOZ_ASSERT(mSession != nullptr);
  if (mSession) {
    nsresult rv = mSession->Pause();
    if (NS_FAILED(rv)) {
      NotifyError(rv);
      return;
    }
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
    nsresult rv = mSession->Resume();
    if (NS_FAILED(rv)) {
      NotifyError(rv);
      return;
    }
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

  nsRefPtr<MediaRecorder> object = new MediaRecorder(aStream, ownerWindow);
  return object.forget();
}

nsresult
MediaRecorder::CreateAndDispatchBlobEvent(Session *aSession)
{
  NS_ABORT_IF_FALSE(NS_IsMainThread(), "Not running on main thread");

  if (!CheckPrincipal()) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  BlobEventInit init;
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
