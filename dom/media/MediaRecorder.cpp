





#include "MediaRecorder.h"
#include "AudioNodeEngine.h"
#include "AudioNodeStream.h"
#include "DOMMediaStream.h"
#include "EncodedBufferCache.h"
#include "MediaEncoder.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/AudioStreamTrack.h"
#include "mozilla/dom/BlobEvent.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/RecordErrorEvent.h"
#include "mozilla/dom/VideoStreamTrack.h"
#include "nsError.h"
#include "nsIDocument.h"
#include "nsIPermissionManager.h"
#include "nsIPrincipal.h"
#include "nsMimeTypes.h"
#include "nsProxyRelease.h"
#include "nsTArray.h"
#include "GeckoProfiler.h"

#ifdef LOG
#undef LOG
#endif

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaRecorderLog;
#define LOG(type, msg) PR_LOG(gMediaRecorderLog, type, msg)
#else
#define LOG(type, msg)
#endif

namespace mozilla {

namespace dom {








class MediaRecorderReporter final : public nsIMemoryReporter
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  MediaRecorderReporter() {};
  static MediaRecorderReporter* UniqueInstance();
  void InitMemoryReporter();

  static void AddMediaRecorder(MediaRecorder *aRecorder)
  {
    GetRecorders().AppendElement(aRecorder);
  }

  static void RemoveMediaRecorder(MediaRecorder *aRecorder)
  {
    RecordersArray& recorders = GetRecorders();
    recorders.RemoveElement(aRecorder);
    if (recorders.IsEmpty()) {
      sUniqueInstance = nullptr;
    }
  }

  NS_METHOD
  CollectReports(nsIHandleReportCallback* aHandleReport,
                 nsISupports* aData, bool aAnonymize) override
  {
    int64_t amount = 0;
    RecordersArray& recorders = GetRecorders();
    for (size_t i = 0; i < recorders.Length(); ++i) {
      amount += recorders[i]->SizeOfExcludingThis(MallocSizeOf);
    }

  #define MEMREPORT(_path, _amount, _desc)                                    \
    do {                                                                      \
      nsresult rv;                                                            \
      rv = aHandleReport->Callback(EmptyCString(), NS_LITERAL_CSTRING(_path), \
                                   KIND_HEAP, UNITS_BYTES, _amount,           \
                                   NS_LITERAL_CSTRING(_desc), aData);         \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
    } while (0)

    MEMREPORT("explicit/media/recorder", amount,
              "Memory used by media recorder.");

    return NS_OK;
  }

private:
  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf)
  virtual ~MediaRecorderReporter();
  static StaticRefPtr<MediaRecorderReporter> sUniqueInstance;
  typedef nsTArray<MediaRecorder*> RecordersArray;
  static RecordersArray& GetRecorders()
  {
    return UniqueInstance()->mRecorders;
  }
  RecordersArray mRecorders;
};
NS_IMPL_ISUPPORTS(MediaRecorderReporter, nsIMemoryReporter);

NS_IMPL_CYCLE_COLLECTION_INHERITED(MediaRecorder, DOMEventTargetHelper,
                                   mDOMStream, mAudioNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaRecorder)
  NS_INTERFACE_MAP_ENTRY(nsIDocumentActivity)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(MediaRecorder, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaRecorder, DOMEventTargetHelper)







































class MediaRecorder::Session: public nsIObserver
{
  NS_DECL_THREADSAFE_ISUPPORTS

  
  
  class PushBlobRunnable : public nsRunnable
  {
  public:
    explicit PushBlobRunnable(Session* aSession)
      : mSession(aSession)
    { }

    NS_IMETHODIMP Run()
    {
      LOG(PR_LOG_DEBUG, ("Session.PushBlobRunnable s=(%p)", mSession.get()));
      MOZ_ASSERT(NS_IsMainThread());

      nsRefPtr<MediaRecorder> recorder = mSession->mRecorder;
      if (!recorder) {
        return NS_OK;
      }

      nsresult rv = recorder->CreateAndDispatchBlobEvent(mSession->GetEncodedData());
      if (NS_FAILED(rv)) {
        recorder->NotifyError(rv);
      }

      return NS_OK;
    }

  private:
    nsRefPtr<Session> mSession;
  };

  
  class EncoderErrorNotifierRunnable : public nsRunnable
  {
  public:
    explicit EncoderErrorNotifierRunnable(Session* aSession)
      : mSession(aSession)
    { }

    NS_IMETHODIMP Run()
    {
      LOG(PR_LOG_DEBUG, ("Session.ErrorNotifyRunnable s=(%p)", mSession.get()));
      MOZ_ASSERT(NS_IsMainThread());

      nsRefPtr<MediaRecorder> recorder = mSession->mRecorder;
      if (!recorder) {
        return NS_OK;
      }

      if (mSession->IsEncoderError()) {
        recorder->NotifyError(NS_ERROR_UNEXPECTED);
      }
      return NS_OK;
    }

  private:
    nsRefPtr<Session> mSession;
  };

  
  class DispatchStartEventRunnable : public nsRunnable
  {
  public:
    DispatchStartEventRunnable(Session* aSession, const nsAString & aEventName)
      : mSession(aSession)
      , mEventName(aEventName)
    { }

    NS_IMETHODIMP Run()
    {
      LOG(PR_LOG_DEBUG, ("Session.DispatchStartEventRunnable s=(%p)", mSession.get()));
      MOZ_ASSERT(NS_IsMainThread());

      NS_ENSURE_TRUE(mSession->mRecorder, NS_OK);
      nsRefPtr<MediaRecorder> recorder = mSession->mRecorder;

      recorder->SetMimeType(mSession->mMimeType);
      recorder->DispatchSimpleEvent(mEventName);

      return NS_OK;
    }

  private:
    nsRefPtr<Session> mSession;
    nsString mEventName;
  };

  
  
  class ExtractRunnable : public nsRunnable
  {
  public:
    explicit ExtractRunnable(Session* aSession)
      : mSession(aSession) {}

    ~ExtractRunnable()
    {}

    NS_IMETHODIMP Run()
    {
      MOZ_ASSERT(NS_GetCurrentThread() == mSession->mReadThread);

      LOG(PR_LOG_DEBUG, ("Session.ExtractRunnable shutdown = %d", mSession->mEncoder->IsShutdown()));
      if (!mSession->mEncoder->IsShutdown()) {
        mSession->Extract(false);
        nsCOMPtr<nsIRunnable> event = new ExtractRunnable(mSession);
        if (NS_FAILED(NS_DispatchToCurrentThread(event))) {
          NS_WARNING("Failed to dispatch ExtractRunnable to encoder thread");
        }
      } else {
        
        mSession->Extract(true);
        if (mSession->mIsRegisterProfiler)
           profiler_unregister_thread();
        if (NS_FAILED(NS_DispatchToMainThread(
                        new DestroyRunnable(mSession)))) {
          MOZ_ASSERT(false, "NS_DispatchToMainThread DestroyRunnable failed");
        }
      }
      return NS_OK;
    }

  private:
    nsRefPtr<Session> mSession;
  };

  
  class TracksAvailableCallback : public DOMMediaStream::OnTracksAvailableCallback
  {
  public:
    explicit TracksAvailableCallback(Session *aSession)
     : mSession(aSession) {}
    virtual void NotifyTracksAvailable(DOMMediaStream* aStream)
    {
      uint8_t trackTypes = 0;
      nsTArray<nsRefPtr<mozilla::dom::AudioStreamTrack>> audioTracks;
      aStream->GetAudioTracks(audioTracks);
      if (!audioTracks.IsEmpty()) {
        trackTypes |= ContainerWriter::CREATE_AUDIO_TRACK;
      }

      nsTArray<nsRefPtr<mozilla::dom::VideoStreamTrack>> videoTracks;
      aStream->GetVideoTracks(videoTracks);
      if (!videoTracks.IsEmpty()) {
        trackTypes |= ContainerWriter::CREATE_VIDEO_TRACK;
      }

      LOG(PR_LOG_DEBUG, ("Session.NotifyTracksAvailable track type = (%d)", trackTypes));
      mSession->InitEncoder(trackTypes);
    }
  private:
    nsRefPtr<Session> mSession;
  };
  
  
  class DestroyRunnable : public nsRunnable
  {
  public:
    explicit DestroyRunnable(Session* aSession)
      : mSession(aSession) {}

    NS_IMETHODIMP Run()
    {
      LOG(PR_LOG_DEBUG, ("Session.DestroyRunnable session refcnt = (%d) stopIssued %d s=(%p)",
                         (int)mSession->mRefCnt, mSession->mStopIssued, mSession.get()));
      MOZ_ASSERT(NS_IsMainThread() && mSession.get());
      nsRefPtr<MediaRecorder> recorder = mSession->mRecorder;
      if (!recorder) {
        return NS_OK;
      }
      
      
      
      
      
      
      if (!mSession->mStopIssued) {
        ErrorResult result;
        mSession->mStopIssued = true;
        recorder->Stop(result);
        if (NS_FAILED(NS_DispatchToMainThread(new DestroyRunnable(mSession)))) {
          MOZ_ASSERT(false, "NS_DispatchToMainThread failed");
        }
        return NS_OK;
      }

      
      mSession->mMimeType = NS_LITERAL_STRING("");
      recorder->SetMimeType(mSession->mMimeType);
      recorder->DispatchSimpleEvent(NS_LITERAL_STRING("stop"));
      mSession->BreakCycle();
      return NS_OK;
    }

  private:
    
    nsRefPtr<Session> mSession;
  };

  friend class EncoderErrorNotifierRunnable;
  friend class PushBlobRunnable;
  friend class ExtractRunnable;
  friend class DestroyRunnable;
  friend class TracksAvailableCallback;

public:
  Session(MediaRecorder* aRecorder, int32_t aTimeSlice)
    : mRecorder(aRecorder)
    , mTimeSlice(aTimeSlice)
    , mStopIssued(false)
    , mCanRetrieveData(false)
    , mIsRegisterProfiler(false)
    , mNeedSessionEndTask(true)
  {
    MOZ_ASSERT(NS_IsMainThread());

    uint32_t maxMem = Preferences::GetUint("media.recorder.max_memory",
                                           MAX_ALLOW_MEMORY_BUFFER);
    mEncodedBufferCache = new EncodedBufferCache(maxMem);
    mLastBlobTimeStamp = TimeStamp::Now();
  }

  void Start()
  {
    LOG(PR_LOG_DEBUG, ("Session.Start %p", this));
    MOZ_ASSERT(NS_IsMainThread());

    SetupStreams();
  }

  void Stop()
  {
    LOG(PR_LOG_DEBUG, ("Session.Stop %p", this));
    MOZ_ASSERT(NS_IsMainThread());
    mStopIssued = true;
    CleanupStreams();
    if (mNeedSessionEndTask) {
      LOG(PR_LOG_DEBUG, ("Session.Stop mNeedSessionEndTask %p", this));
      
      DoSessionEndTask(NS_OK);
    }
    nsContentUtils::UnregisterShutdownObserver(this);
  }

  nsresult Pause()
  {
    LOG(PR_LOG_DEBUG, ("Session.Pause"));
    MOZ_ASSERT(NS_IsMainThread());

    NS_ENSURE_TRUE(mTrackUnionStream, NS_ERROR_FAILURE);
    mTrackUnionStream->ChangeExplicitBlockerCount(1);

    return NS_OK;
  }

  nsresult Resume()
  {
    LOG(PR_LOG_DEBUG, ("Session.Resume"));
    MOZ_ASSERT(NS_IsMainThread());

    NS_ENSURE_TRUE(mTrackUnionStream, NS_ERROR_FAILURE);
    mTrackUnionStream->ChangeExplicitBlockerCount(-1);

    return NS_OK;
  }

  nsresult RequestData()
  {
    LOG(PR_LOG_DEBUG, ("Session.RequestData"));
    MOZ_ASSERT(NS_IsMainThread());

    if (NS_FAILED(NS_DispatchToMainThread(new EncoderErrorNotifierRunnable(this))) ||
        NS_FAILED(NS_DispatchToMainThread(new PushBlobRunnable(this)))) {
      MOZ_ASSERT(false, "RequestData NS_DispatchToMainThread failed");
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

  already_AddRefed<nsIDOMBlob> GetEncodedData()
  {
    MOZ_ASSERT(NS_IsMainThread());
    return mEncodedBufferCache->ExtractBlob(mRecorder->GetParentObject(),
                                            mMimeType);
  }

  bool IsEncoderError()
  {
    if (mEncoder && mEncoder->HasError()) {
      return true;
    }
    return false;
  }

  size_t
  SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    size_t amount = mEncoder->SizeOfExcludingThis(aMallocSizeOf);
    return amount;
  }


private:
  
  virtual ~Session()
  {
    LOG(PR_LOG_DEBUG, ("Session.~Session (%p)", this));
    CleanupStreams();
  }
  
  
  
  
  void Extract(bool aForceFlush)
  {
    MOZ_ASSERT(NS_GetCurrentThread() == mReadThread);
    LOG(PR_LOG_DEBUG, ("Session.Extract %p", this));

    if (!mIsRegisterProfiler) {
      char aLocal;
      profiler_register_thread("Media_Encoder", &aLocal);
      mIsRegisterProfiler = true;
    }

    PROFILER_LABEL("MediaRecorder", "Session Extract",
      js::ProfileEntry::Category::OTHER);

    
    nsTArray<nsTArray<uint8_t> > encodedBuf;
    mEncoder->GetEncodedData(&encodedBuf, mMimeType);

    
    for (uint32_t i = 0; i < encodedBuf.Length(); i++) {
      if (!encodedBuf[i].IsEmpty()) {
        mEncodedBufferCache->AppendBuffer(encodedBuf[i]);
        
        if (!mCanRetrieveData) {
          NS_DispatchToMainThread(
            new DispatchStartEventRunnable(this, NS_LITERAL_STRING("start")));
          mCanRetrieveData = true;
        }
      }
    }

    
    
    bool pushBlob = false;
    if ((mTimeSlice > 0) &&
        ((TimeStamp::Now()-mLastBlobTimeStamp).ToMilliseconds() > mTimeSlice)) {
      pushBlob = true;
    }
    if (pushBlob || aForceFlush) {
      if (NS_FAILED(NS_DispatchToMainThread(new EncoderErrorNotifierRunnable(this)))) {
        MOZ_ASSERT(false, "NS_DispatchToMainThread EncoderErrorNotifierRunnable failed");
      }
      if (NS_FAILED(NS_DispatchToMainThread(new PushBlobRunnable(this)))) {
        MOZ_ASSERT(false, "NS_DispatchToMainThread PushBlobRunnable failed");
      } else {
        mLastBlobTimeStamp = TimeStamp::Now();
      }
    }
  }

  
  void SetupStreams()
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    MediaStreamGraph* gm = mRecorder->GetSourceMediaStream()->Graph();
    mTrackUnionStream = gm->CreateTrackUnionStream(nullptr);
    MOZ_ASSERT(mTrackUnionStream, "CreateTrackUnionStream failed");

    mTrackUnionStream->SetAutofinish(true);

    
    mInputPort = mTrackUnionStream->AllocateInputPort(mRecorder->GetSourceMediaStream(),
                                                      MediaInputPort::FLAG_BLOCK_OUTPUT);

    DOMMediaStream* domStream = mRecorder->Stream();
    if (domStream) {
      
      TracksAvailableCallback* tracksAvailableCallback = new TracksAvailableCallback(this);
      domStream->OnTracksAvailable(tracksAvailableCallback);
    } else {
      
      InitEncoder(ContainerWriter::CREATE_AUDIO_TRACK);
    }
  }

  bool Check3gppPermission()
  {
    nsCOMPtr<nsIDocument> doc = mRecorder->GetOwner()->GetExtantDoc();
    if (!doc) {
      return false;
    }

    uint16_t appStatus = nsIPrincipal::APP_STATUS_NOT_INSTALLED;
    doc->NodePrincipal()->GetAppStatus(&appStatus);

    
    if (appStatus == nsIPrincipal::APP_STATUS_CERTIFIED) {
      return true;
    }

    nsCOMPtr<nsIPermissionManager> pm =
       do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);

    if (!pm) {
      return false;
    }

    uint32_t perm = nsIPermissionManager::DENY_ACTION;
    pm->TestExactPermissionFromPrincipal(doc->NodePrincipal(), "audio-capture:3gpp", &perm);
    return perm == nsIPermissionManager::ALLOW_ACTION;
  }

  void InitEncoder(uint8_t aTrackTypes)
  {
    LOG(PR_LOG_DEBUG, ("Session.InitEncoder %p", this));
    MOZ_ASSERT(NS_IsMainThread());

    if (!mRecorder) {
      LOG(PR_LOG_DEBUG, ("Session.InitEncoder failure, mRecorder is null %p", this));
      return;
    }
    
    

    
    if (mRecorder->mMimeType.EqualsLiteral(AUDIO_3GPP) && Check3gppPermission()) {
      mEncoder = MediaEncoder::CreateEncoder(NS_LITERAL_STRING(AUDIO_3GPP), aTrackTypes);
    } else {
      mEncoder = MediaEncoder::CreateEncoder(NS_LITERAL_STRING(""), aTrackTypes);
    }

    if (!mEncoder) {
      LOG(PR_LOG_DEBUG, ("Session.InitEncoder !mEncoder %p", this));
      DoSessionEndTask(NS_ERROR_ABORT);
      return;
    }

    
    
    
    if (!mTrackUnionStream) {
      LOG(PR_LOG_DEBUG, ("Session.InitEncoder !mTrackUnionStream %p", this));
      DoSessionEndTask(NS_OK);
      return;
    }
    mTrackUnionStream->AddListener(mEncoder);
    
    if (!mReadThread) {
      nsresult rv = NS_NewNamedThread("Media_Encoder", getter_AddRefs(mReadThread));
      if (NS_FAILED(rv)) {
        LOG(PR_LOG_DEBUG, ("Session.InitEncoder !mReadThread %p", this));
        DoSessionEndTask(rv);
        return;
      }
    }

    
    
    nsContentUtils::RegisterShutdownObserver(this);

    nsCOMPtr<nsIRunnable> event = new ExtractRunnable(this);
    if (NS_FAILED(mReadThread->Dispatch(event, NS_DISPATCH_NORMAL))) {
      NS_WARNING("Failed to dispatch ExtractRunnable at beginning");
      LOG(PR_LOG_DEBUG, ("Session.InitEncoder !ReadThread->Dispatch %p", this));
      DoSessionEndTask(NS_ERROR_ABORT);
    }
    
    
    
    mNeedSessionEndTask = false;
  }
  
  void DoSessionEndTask(nsresult rv)
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (NS_FAILED(rv)) {
      mRecorder->NotifyError(rv);
    }

    CleanupStreams();
    if (NS_FAILED(NS_DispatchToMainThread(new EncoderErrorNotifierRunnable(this)))) {
      MOZ_ASSERT(false, "NS_DispatchToMainThread EncoderErrorNotifierRunnable failed");
    }
    if (NS_FAILED(NS_DispatchToMainThread(new PushBlobRunnable(this)))) {
      MOZ_ASSERT(false, "NS_DispatchToMainThread PushBlobRunnable failed");
    }
    if (NS_FAILED(NS_DispatchToMainThread(new DestroyRunnable(this)))) {
      MOZ_ASSERT(false, "NS_DispatchToMainThread DestroyRunnable failed");
    }
    mNeedSessionEndTask = false;
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

  NS_IMETHODIMP Observe(nsISupports *aSubject, const char *aTopic, const char16_t *aData) override
  {
    MOZ_ASSERT(NS_IsMainThread());
    LOG(PR_LOG_DEBUG, ("Session.Observe XPCOM_SHUTDOWN %p", this));
    if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
      
      mEncoder->Cancel();
      if (mReadThread) {
        mReadThread->Shutdown();
        mReadThread = nullptr;
      }
      BreakCycle();
      Stop();
    }

    return NS_OK;
  }

  
  void BreakCycle()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (mRecorder) {
      mRecorder->RemoveSession(this);
      mRecorder = nullptr;
    }
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
  
  bool mCanRetrieveData;
  
  bool mIsRegisterProfiler;
  
  
  
  bool mNeedSessionEndTask;
};

NS_IMPL_ISUPPORTS(MediaRecorder::Session, nsIObserver)

MediaRecorder::~MediaRecorder()
{
  if (mPipeStream != nullptr) {
    mInputPort->Destroy();
    mPipeStream->Destroy();
  }
  LOG(PR_LOG_DEBUG, ("~MediaRecorder (%p)", this));
  UnRegisterActivityObserver();
}

MediaRecorder::MediaRecorder(DOMMediaStream& aSourceMediaStream,
                             nsPIDOMWindow* aOwnerWindow)
  : DOMEventTargetHelper(aOwnerWindow)
  , mState(RecordingState::Inactive)
{
  MOZ_ASSERT(aOwnerWindow);
  MOZ_ASSERT(aOwnerWindow->IsInnerWindow());
  mDOMStream = &aSourceMediaStream;
#ifdef PR_LOGGING
  if (!gMediaRecorderLog) {
    gMediaRecorderLog = PR_NewLogModule("MediaRecorder");
  }
#endif
  RegisterActivityObserver();
}

MediaRecorder::MediaRecorder(AudioNode& aSrcAudioNode,
                             uint32_t aSrcOutput,
                             nsPIDOMWindow* aOwnerWindow)
  : DOMEventTargetHelper(aOwnerWindow)
  , mState(RecordingState::Inactive)
{
  MOZ_ASSERT(aOwnerWindow);
  MOZ_ASSERT(aOwnerWindow->IsInnerWindow());

  
  
  
  
  if (aSrcAudioNode.NumberOfOutputs() > 0) {
    AudioContext* ctx = aSrcAudioNode.Context();
    AudioNodeEngine* engine = new AudioNodeEngine(nullptr);
    mPipeStream = ctx->Graph()->CreateAudioNodeStream(engine,
                                                      MediaStreamGraph::EXTERNAL_STREAM,
                                                      ctx->SampleRate());
    mInputPort = mPipeStream->AllocateInputPort(aSrcAudioNode.Stream(),
                                                MediaInputPort::FLAG_BLOCK_INPUT,
                                                0,
                                                aSrcOutput);
  }
  mAudioNode = &aSrcAudioNode;
  #ifdef PR_LOGGING
  if (!gMediaRecorderLog) {
    gMediaRecorderLog = PR_NewLogModule("MediaRecorder");
  }
  #endif
  RegisterActivityObserver();
}

void
MediaRecorder::RegisterActivityObserver()
{
  nsPIDOMWindow* window = GetOwner();
  if (window) {
    nsIDocument* doc = window->GetExtantDoc();
    if (doc) {
      doc->RegisterActivityObserver(
        NS_ISUPPORTS_CAST(nsIDocumentActivity*, this));
    }
  }
}

void
MediaRecorder::UnRegisterActivityObserver()
{
  nsPIDOMWindow* window = GetOwner();
  if (window) {
    nsIDocument* doc = window->GetExtantDoc();
    if (doc) {
      doc->UnregisterActivityObserver(
        NS_ISUPPORTS_CAST(nsIDocumentActivity*, this));
    }
  }
}

void
MediaRecorder::SetMimeType(const nsString &aMimeType)
{
  mMimeType = aMimeType;
}

void
MediaRecorder::GetMimeType(nsString &aMimeType)
{
  aMimeType = mMimeType;
}

void
MediaRecorder::Start(const Optional<int32_t>& aTimeSlice, ErrorResult& aResult)
{
  LOG(PR_LOG_DEBUG, ("MediaRecorder.Start %p", this));
  if (mState != RecordingState::Inactive) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (GetSourceMediaStream()->IsFinished() || GetSourceMediaStream()->IsDestroyed()) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  
  if (mDOMStream && !mDOMStream->GetPrincipal()) {
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
  MediaRecorderReporter::AddMediaRecorder(this);
  mState = RecordingState::Recording;
  
  mSessions.AppendElement();
  mSessions.LastElement() = new Session(this, timeSlice);
  mSessions.LastElement()->Start();
}

void
MediaRecorder::Stop(ErrorResult& aResult)
{
  LOG(PR_LOG_DEBUG, ("MediaRecorder.Stop %p", this));
  MediaRecorderReporter::RemoveMediaRecorder(this);
  if (mState == RecordingState::Inactive) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mState = RecordingState::Inactive;
  MOZ_ASSERT(mSessions.Length() > 0);
  mSessions.LastElement()->Stop();
}

void
MediaRecorder::Pause(ErrorResult& aResult)
{
  LOG(PR_LOG_DEBUG, ("MediaRecorder.Pause"));
  if (mState != RecordingState::Recording) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  MOZ_ASSERT(mSessions.Length() > 0);
  nsresult rv = mSessions.LastElement()->Pause();
  if (NS_FAILED(rv)) {
    NotifyError(rv);
    return;
  }
  mState = RecordingState::Paused;
}

void
MediaRecorder::Resume(ErrorResult& aResult)
{
  LOG(PR_LOG_DEBUG, ("MediaRecorder.Resume"));
  if (mState != RecordingState::Paused) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  MOZ_ASSERT(mSessions.Length() > 0);
  nsresult rv = mSessions.LastElement()->Resume();
  if (NS_FAILED(rv)) {
    NotifyError(rv);
    return;
  }
  mState = RecordingState::Recording;
}

void
MediaRecorder::RequestData(ErrorResult& aResult)
{
  if (mState != RecordingState::Recording) {
    aResult.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  MOZ_ASSERT(mSessions.Length() > 0);
  nsresult rv = mSessions.LastElement()->RequestData();
  if (NS_FAILED(rv)) {
    NotifyError(rv);
  }
}

JSObject*
MediaRecorder::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MediaRecorderBinding::Wrap(aCx, this, aGivenProto);
}

 already_AddRefed<MediaRecorder>
MediaRecorder::Constructor(const GlobalObject& aGlobal,
                           DOMMediaStream& aStream,
                           const MediaRecorderOptions& aInitDict,
                           ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> ownerWindow = do_QueryInterface(aGlobal.GetAsSupports());
  if (!ownerWindow) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<MediaRecorder> object = new MediaRecorder(aStream, ownerWindow);
  object->SetMimeType(aInitDict.mMimeType);
  return object.forget();
}

 already_AddRefed<MediaRecorder>
MediaRecorder::Constructor(const GlobalObject& aGlobal,
                           AudioNode& aSrcAudioNode,
                           uint32_t aSrcOutput,
                           const MediaRecorderOptions& aInitDict,
                           ErrorResult& aRv)
{
  
  if (!Preferences::GetBool("media.recorder.audio_node.enabled", false)) {
    
    NS_NAMED_LITERAL_STRING(argStr, "Argument 1 of MediaRecorder.constructor");
    NS_NAMED_LITERAL_STRING(typeStr, "MediaStream");
    aRv.ThrowTypeError(MSG_DOES_NOT_IMPLEMENT_INTERFACE, &argStr, &typeStr);
    return nullptr;
  }

  nsCOMPtr<nsPIDOMWindow> ownerWindow = do_QueryInterface(aGlobal.GetAsSupports());
  if (!ownerWindow) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  
  if (aSrcAudioNode.NumberOfOutputs() > 0 &&
       aSrcOutput >= aSrcAudioNode.NumberOfOutputs()) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<MediaRecorder> object = new MediaRecorder(aSrcAudioNode,
                                                     aSrcOutput,
                                                     ownerWindow);
  object->SetMimeType(aInitDict.mMimeType);
  return object.forget();
}

nsresult
MediaRecorder::CreateAndDispatchBlobEvent(already_AddRefed<nsIDOMBlob>&& aBlob)
{
  MOZ_ASSERT(NS_IsMainThread(), "Not running on main thread");
  if (!CheckPrincipal()) {
    
    nsRefPtr<nsIDOMBlob> blob = aBlob;
    return NS_ERROR_DOM_SECURITY_ERR;
  }
  BlobEventInit init;
  init.mBubbles = false;
  init.mCancelable = false;

  nsCOMPtr<nsIDOMBlob> blob = aBlob;
  init.mData = static_cast<File*>(blob.get());

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
  MOZ_ASSERT(NS_IsMainThread(), "Not running on main thread");
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
  MOZ_ASSERT(NS_IsMainThread(), "Not running on main thread");
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

  RecordErrorEventInit init;
  init.mBubbles = false;
  init.mCancelable = false;
  init.mName = errorMsg;

  nsRefPtr<RecordErrorEvent> event =
    RecordErrorEvent::Constructor(this, NS_LITERAL_STRING("error"), init);
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
  MOZ_ASSERT(NS_IsMainThread(), "Not running on main thread");
  if (!mDOMStream && !mAudioNode) {
    return false;
  }
  if (!GetOwner())
    return false;
  nsCOMPtr<nsIDocument> doc = GetOwner()->GetExtantDoc();
  if (!doc) {
    return false;
  }
  nsIPrincipal* srcPrincipal = GetSourcePrincipal();
  if (!srcPrincipal) {
    return false;
  }
  bool subsumes;
  if (NS_FAILED(doc->NodePrincipal()->Subsumes(srcPrincipal, &subsumes))) {
    return false;
  }
  return subsumes;
}

void
MediaRecorder::RemoveSession(Session* aSession)
{
  LOG(PR_LOG_DEBUG, ("MediaRecorder.RemoveSession (%p)", aSession));
  mSessions.RemoveElement(aSession);
}

void
MediaRecorder::NotifyOwnerDocumentActivityChanged()
{
  nsPIDOMWindow* window = GetOwner();
  NS_ENSURE_TRUE_VOID(window);
  nsIDocument* doc = window->GetExtantDoc();
  NS_ENSURE_TRUE_VOID(doc);

  LOG(PR_LOG_DEBUG, ("MediaRecorder %p document IsActive %d isVisible %d\n",
                     this, doc->IsActive(), doc->IsVisible()));
  if (!doc->IsActive() || !doc->IsVisible()) {
    
    ErrorResult result;
    Stop(result);
  }
}

MediaStream*
MediaRecorder::GetSourceMediaStream()
{
  if (mDOMStream != nullptr) {
    return mDOMStream->GetStream();
  }
  MOZ_ASSERT(mAudioNode != nullptr);
  return mPipeStream != nullptr ? mPipeStream : mAudioNode->Stream();
}

nsIPrincipal*
MediaRecorder::GetSourcePrincipal()
{
  if (mDOMStream != nullptr) {
    return mDOMStream->GetPrincipal();
  }
  MOZ_ASSERT(mAudioNode != nullptr);
  nsIDocument* doc = mAudioNode->GetOwner()->GetExtantDoc();
  return doc ? doc->NodePrincipal() : nullptr;
}

size_t
MediaRecorder::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t amount = 42;
  for (size_t i = 0; i < mSessions.Length(); ++i) {
    amount += mSessions[i]->SizeOfExcludingThis(aMallocSizeOf);
  }
  return amount;
}

StaticRefPtr<MediaRecorderReporter> MediaRecorderReporter::sUniqueInstance;

MediaRecorderReporter* MediaRecorderReporter::UniqueInstance()
{
  if (!sUniqueInstance) {
    sUniqueInstance = new MediaRecorderReporter();
    sUniqueInstance->InitMemoryReporter();
  }
  return sUniqueInstance;
 }

void MediaRecorderReporter::InitMemoryReporter()
{
  RegisterWeakMemoryReporter(this);
}

MediaRecorderReporter::~MediaRecorderReporter()
{
  UnregisterWeakMemoryReporter(this);
}

}
}
