





#include "RtspControllerChild.h"
#include "RtspMetaData.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/net/NeckoChild.h"
#include "nsITabChild.h"
#include "nsILoadContext.h"
#include "nsNetUtil.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/URIUtils.h"
#include "nsStringStream.h"
#include "prlog.h"

PRLogModuleInfo* gRtspChildLog = nullptr;
#undef LOG
#define LOG(args) PR_LOG(gRtspChildLog, PR_LOG_DEBUG, args)

const uint32_t kRtspTotalTracks = 2;
const unsigned long kRtspCommandDelayMs = 200;

using namespace mozilla::ipc;

namespace mozilla {
namespace net {

NS_IMPL_ADDREF(RtspControllerChild)

NS_IMETHODIMP_(nsrefcnt) RtspControllerChild::Release()
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  
  
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "RtspControllerChild");

  if (mRefCnt == 1 && mIPCOpen) {
    Send__delete__(this);
    return mRefCnt;
  }

  if (mRefCnt == 0) {
    mRefCnt = 1; 
    delete this;
    return 0;
  }
  return mRefCnt;
}

NS_INTERFACE_MAP_BEGIN(RtspControllerChild)
  NS_INTERFACE_MAP_ENTRY(nsIStreamingProtocolController)
  NS_INTERFACE_MAP_ENTRY(nsIStreamingProtocolListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIStreamingProtocolController)
NS_INTERFACE_MAP_END




RtspControllerChild::RtspControllerChild(nsIChannel *channel)
  : mIPCOpen(false)
  , mIPCAllowed(false)
  , mChannel(channel)
  , mTotalTracks(0)
  , mSuspendCount(0)
  , mTimerLock("RtspControllerChild.mTimerLock")
  , mPlayTimer(nullptr)
  , mPauseTimer(nullptr)
{
#if defined(PR_LOGGING)
  if (!gRtspChildLog)
    gRtspChildLog = PR_NewLogModule("nsRtspChild");
#endif
  AddIPDLReference();
  gNeckoChild->SendPRtspControllerConstructor(this);
}

RtspControllerChild::~RtspControllerChild()
{
  LOG(("RtspControllerChild::~RtspControllerChild()"));
}

void
RtspControllerChild::ReleaseChannel()
{
  static_cast<RtspChannelChild*>(mChannel.get())->ReleaseController();
}

bool
RtspControllerChild::OKToSendIPC()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mIPCOpen == false) {
    return false;
  }
  return mIPCAllowed;
}

void
RtspControllerChild::AllowIPC()
{
  MOZ_ASSERT(NS_IsMainThread());
  mIPCAllowed = true;
}

void
RtspControllerChild::DisallowIPC()
{
  MOZ_ASSERT(NS_IsMainThread());
  mIPCAllowed = false;
}

void
RtspControllerChild::StopPlayAndPauseTimer()
{
  MutexAutoLock lock(mTimerLock);
  if (mPlayTimer) {
    mPlayTimer->Cancel();
    mPlayTimer = nullptr;
  }
  if (mPauseTimer) {
    mPauseTimer->Cancel();
    mPauseTimer = nullptr;
  }
}




bool
RtspControllerChild::RecvOnMediaDataAvailable(
                       const uint8_t& index,
                       const nsCString& data,
                       const uint32_t& length,
                       const uint32_t& offset,
                       InfallibleTArray<RtspMetadataParam>&& metaArray)
{
  nsRefPtr<RtspMetaData> meta = new RtspMetaData();
  nsresult rv = meta->DeserializeRtspMetaData(metaArray);
  NS_ENSURE_SUCCESS(rv, true);

  if (mListener) {
    mListener->OnMediaDataAvailable(index, data, length, offset, meta.get());
  }
  return true;
}

void
RtspControllerChild::AddMetaData(
                       already_AddRefed<nsIStreamingProtocolMetaData>&& meta)
{
  mMetaArray.AppendElement(mozilla::Move(meta));
}

int
RtspControllerChild::GetMetaDataLength()
{
  return mMetaArray.Length();
}

bool
RtspControllerChild::RecvOnConnected(
                       const uint8_t& index,
                       InfallibleTArray<RtspMetadataParam>&& metaArray)
{
  
  nsRefPtr<RtspMetaData> meta = new RtspMetaData();
  nsresult rv = meta->DeserializeRtspMetaData(metaArray);
  NS_ENSURE_SUCCESS(rv, true);
  meta->GetTotalTracks(&mTotalTracks);
  if (mTotalTracks <= 0) {
    LOG(("RtspControllerChild::RecvOnConnected invalid tracks %d", mTotalTracks));
    
    mTotalTracks = kRtspTotalTracks;
  }
  AddMetaData(meta.forget().downcast<nsIStreamingProtocolMetaData>());

  
  if ((static_cast<uint32_t>(index) + 1) == mTotalTracks) {
    
    if (mListener) {
      mListener->OnConnected(index, nullptr);
    }
  }
  return true;
}

bool
RtspControllerChild::RecvOnDisconnected(
                       const uint8_t& index,
                       const nsresult& reason)
{
  StopPlayAndPauseTimer();
  DisallowIPC();
  LOG(("RtspControllerChild::RecvOnDisconnected for track %d reason = 0x%x", index, reason));
  if (mListener) {
    mListener->OnDisconnected(index, reason);
  }
  ReleaseChannel();
  return true;
}

bool
RtspControllerChild::RecvAsyncOpenFailed(const nsresult& reason)
{
  StopPlayAndPauseTimer();
  DisallowIPC();
  LOG(("RtspControllerChild::RecvAsyncOpenFailed reason = 0x%x", reason));
  if (mListener) {
    mListener->OnDisconnected(0, NS_ERROR_CONNECTION_REFUSED);
  }
  ReleaseChannel();
  return true;
}

void
RtspControllerChild::AddIPDLReference()
{
  MOZ_ASSERT(!mIPCOpen,
             "Attempt to retain more than one IPDL reference");
  mIPCOpen = true;
  AllowIPC();
  AddRef();
}

void
RtspControllerChild::ReleaseIPDLReference()
{
  MOZ_ASSERT(mIPCOpen, "Attempt to release nonexistent IPDL reference");
  mIPCOpen = false;
  DisallowIPC();
  Release();
}

NS_IMETHODIMP
RtspControllerChild::GetTrackMetaData(
                       uint8_t index,
                       nsIStreamingProtocolMetaData **result)
{
  if (GetMetaDataLength() <= 0 || index >= GetMetaDataLength()) {
    LOG(("RtspControllerChild:: meta data is not available"));
    return NS_ERROR_NOT_INITIALIZED;
  }
  LOG(("RtspControllerChild::GetTrackMetaData() %d", index));
  NS_IF_ADDREF(*result = mMetaArray[index]);
  return NS_OK;
}

enum IPCEvent
{
  SendNoneEvent = 0,
  SendPlayEvent,
  SendPauseEvent,
  SendSeekEvent,
  SendStopEvent,
  SendPlaybackEndedEvent
};

class SendIPCEvent : public nsRunnable
{
public:
  SendIPCEvent(RtspControllerChild *aController, IPCEvent aEvent)
    : mController(aController)
    , mEvent(aEvent)
    , mSeekTime(0)
  {
  }

  SendIPCEvent(RtspControllerChild *aController,
               IPCEvent aEvent,
               uint64_t aSeekTime)
    : mController(aController)
    , mEvent(aEvent)
    , mSeekTime(aSeekTime)
  {
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    if (mController->OKToSendIPC() == false) {
      
      
      return NS_ERROR_FAILURE;
    }
    bool rv = true;

    if (mEvent == SendPlayEvent) {
      rv = mController->SendPlay();
    } else if (mEvent == SendPauseEvent) {
      rv = mController->SendPause();
    } else if (mEvent == SendSeekEvent) {
      rv = mController->SendSeek(mSeekTime);
    } else if (mEvent == SendStopEvent) {
      rv = mController->SendStop();
    } else if (mEvent == SendPlaybackEndedEvent) {
      rv = mController->SendPlaybackEnded();
    } else {
      LOG(("RtspControllerChild::SendIPCEvent"));
    }
    if (!rv) {
      return NS_ERROR_FAILURE;
    }
    return NS_OK;
  }
private:
  nsRefPtr<RtspControllerChild> mController;
  IPCEvent mEvent;
  uint64_t mSeekTime;
};




NS_IMETHODIMP
RtspControllerChild::Play(void)
{
  LOG(("RtspControllerChild::Play()"));

  MutexAutoLock lock(mTimerLock);
  
  
  if (mPauseTimer) {
    mPauseTimer->Cancel();
    mPauseTimer = nullptr;
  }

  
  if (!mPlayTimer) {
    mPlayTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mPlayTimer) {
      return NS_ERROR_NOT_INITIALIZED;
    }
    
    
    
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    mPlayTimer->SetTarget(mainThread);
    mPlayTimer->InitWithFuncCallback(
                  RtspControllerChild::PlayTimerCallback,
                  this, kRtspCommandDelayMs,
                  nsITimer::TYPE_ONE_SHOT);
  }

  return NS_OK;
}

NS_IMETHODIMP
RtspControllerChild::Pause(void)
{
  LOG(("RtspControllerChild::Pause()"));

  MutexAutoLock lock(mTimerLock);
  
  
  if (mPlayTimer) {
    mPlayTimer->Cancel();
    mPlayTimer = nullptr;
  }

  
  if (!mPauseTimer) {
    mPauseTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mPauseTimer) {
      return NS_ERROR_NOT_INITIALIZED;
    }
    
    
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    mPauseTimer->SetTarget(mainThread);
    mPauseTimer->InitWithFuncCallback(
                  RtspControllerChild::PauseTimerCallback,
                  this, kRtspCommandDelayMs,
                  nsITimer::TYPE_ONE_SHOT);
  }

  return NS_OK;
}

NS_IMETHODIMP
RtspControllerChild::Resume(void)
{
  LOG(("RtspControllerChild::Resume()"));
  NS_ENSURE_TRUE(mSuspendCount > 0, NS_ERROR_UNEXPECTED);

  if (!--mSuspendCount) {
    return Play();
  }

  return NS_OK;
}

NS_IMETHODIMP
RtspControllerChild::Suspend(void)
{
  LOG(("RtspControllerChild::Suspend()"));

  if (!mSuspendCount++) {
    return Pause();
  }

  return NS_OK;
}

NS_IMETHODIMP
RtspControllerChild::Seek(uint64_t seekTimeUs)
{
  LOG(("RtspControllerChild::Seek() %llu", seekTimeUs));

  if (NS_IsMainThread()) {
    if (!OKToSendIPC() || !SendSeek(seekTimeUs)) {
      return NS_ERROR_FAILURE;
    }
  } else {
    nsresult rv = NS_DispatchToMainThread(
                    new SendIPCEvent(this, SendSeekEvent, seekTimeUs));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
RtspControllerChild::Stop()
{
  LOG(("RtspControllerChild::Stop()"));
  StopPlayAndPauseTimer();

  if (NS_IsMainThread()) {
    if (!OKToSendIPC() || !SendStop()) {
      return NS_ERROR_FAILURE;
    }
    DisallowIPC();
  } else {
    nsresult rv = NS_DispatchToMainThread(
                    new SendIPCEvent(this, SendStopEvent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
RtspControllerChild::GetTotalTracks(uint8_t *aTracks)
{
  NS_ENSURE_ARG_POINTER(aTracks);
  *aTracks = kRtspTotalTracks;
  if (mTotalTracks) {
    *aTracks = mTotalTracks;
  }
  LOG(("RtspControllerChild::GetTracks() %d", *aTracks));
  return NS_OK;
}

NS_IMETHODIMP
RtspControllerChild::PlaybackEnded()
{
  LOG(("RtspControllerChild::PlaybackEnded"));

  StopPlayAndPauseTimer();

  if (NS_IsMainThread()) {
    if (!OKToSendIPC() || !SendPlaybackEnded()) {
      return NS_ERROR_FAILURE;
    }
  } else {
    nsresult rv = NS_DispatchToMainThread(
                    new SendIPCEvent(this, SendPlaybackEndedEvent));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}




NS_IMETHODIMP
RtspControllerChild::OnMediaDataAvailable(uint8_t index,
                                          const nsACString & data,
                                          uint32_t length,
                                          uint32_t offset,
                                          nsIStreamingProtocolMetaData *meta)
{
  LOG(("RtspControllerChild::OnMediaDataAvailable()"));
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RtspControllerChild::OnConnected(uint8_t index,
                                 nsIStreamingProtocolMetaData *meta)

{
  LOG(("RtspControllerChild::OnConnected()"));
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
RtspControllerChild::OnDisconnected(uint8_t index,
                                    nsresult reason)
{
  LOG(("RtspControllerChild::OnDisconnected() reason = 0x%x", reason));
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP
RtspControllerChild::Init(nsIURI *aURI)
{
  nsresult rv;

  if (!aURI) {
    LOG(("RtspControllerChild::Init() - invalid URI"));
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsAutoCString host;
  int32_t port = -1;

  rv = aURI->GetAsciiHost(host);
  if (NS_FAILED(rv)) return rv;

  
  if (host.IsEmpty())
    return NS_ERROR_MALFORMED_URI;

  rv = aURI->GetPort(&port);
  if (NS_FAILED(rv)) return rv;

  rv = aURI->GetAsciiSpec(mSpec);
  if (NS_FAILED(rv)) return rv;

  if (!strncmp(mSpec.get(), "rtsp:", 5) == 0)
    return NS_ERROR_UNEXPECTED;

  mURI = aURI;

  return NS_OK;
}

NS_IMETHODIMP
RtspControllerChild::AsyncOpen(nsIStreamingProtocolListener *aListener)
{
  LOG(("RtspControllerChild::AsyncOpen()"));
  if (!aListener) {
    LOG(("RtspControllerChild::AsyncOpen() - invalid listener"));
    return NS_ERROR_NOT_INITIALIZED;
  }
  mListener = aListener;

  if (!mChannel) {
    LOG(("RtspControllerChild::AsyncOpen() - invalid URI"));
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsCOMPtr<nsIURI> uri;
  URIParams uriParams;
  mChannel->GetURI(getter_AddRefs(uri));
  if (!uri) {
    LOG(("RtspControllerChild::AsyncOpen() - invalid URI"));
    return NS_ERROR_NOT_INITIALIZED;
  }
  SerializeURI(uri, uriParams);

  if (!OKToSendIPC() || !SendAsyncOpen(uriParams)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}





void
RtspControllerChild::PlayTimerCallback(nsITimer *aTimer, void *aClosure)
{
  MOZ_ASSERT(aTimer);
  MOZ_ASSERT(aClosure);
  MOZ_ASSERT(NS_IsMainThread());

  RtspControllerChild *self = static_cast<RtspControllerChild*>(aClosure);

  MutexAutoLock lock(self->mTimerLock);
  if (!self->mPlayTimer || !self->OKToSendIPC()) {
    return;
  }
  self->SendPlay();
  self->mPlayTimer = nullptr;
}


void
RtspControllerChild::PauseTimerCallback(nsITimer *aTimer, void *aClosure)
{
  MOZ_ASSERT(aTimer);
  MOZ_ASSERT(aClosure);
  MOZ_ASSERT(NS_IsMainThread());

  RtspControllerChild *self = static_cast<RtspControllerChild*>(aClosure);

  MutexAutoLock lock(self->mTimerLock);
  if (!self->mPauseTimer || !self->OKToSendIPC()) {
    return;
  }
  self->SendPause();
  self->mPauseTimer = nullptr;
}

} 
} 
