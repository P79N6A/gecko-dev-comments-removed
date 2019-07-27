





#include "mozilla/DebugOnly.h"

#include "RtspMediaResource.h"

#include "MediaDecoder.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/Monitor.h"
#include "mozilla/Preferences.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStreamingProtocolService.h"
#include "nsServiceManagerUtils.h"
#ifdef NECKO_PROTOCOL_rtsp
#include "mozilla/net/RtspChannelChild.h"
#endif
using namespace mozilla::net;

#ifdef PR_LOGGING
PRLogModuleInfo* gRtspMediaResourceLog;
#define RTSP_LOG(msg, ...) PR_LOG(gRtspMediaResourceLog, PR_LOG_DEBUG, \
                                  (msg, ##__VA_ARGS__))

#define RTSPMLOG(msg, ...) \
        RTSP_LOG("%p [RtspMediaResource]: " msg, this, ##__VA_ARGS__)
#else
#define RTSP_LOG(msg, ...)
#define RTSPMLOG(msg, ...)
#endif

namespace mozilla {








#define BUFFER_SLOT_NUM 8192
#define BUFFER_SLOT_DEFAULT_SIZE 256
#define BUFFER_SLOT_MAX_SIZE 512
#define BUFFER_SLOT_INVALID -1
#define BUFFER_SLOT_EMPTY 0

struct BufferSlotData {
  int32_t mLength;
  uint64_t mTime;
  int32_t  mFrameType;
};


const float kBufferThresholdPerc = 0.8f;

const uint32_t kPlayoutDelayMs = 3000;




class RtspTrackBuffer
{
public:
  RtspTrackBuffer(const char *aMonitor, int32_t aTrackIdx, uint32_t aSlotSize)
  : mMonitor(aMonitor)
  , mSlotSize(aSlotSize)
  , mTotalBufferSize(BUFFER_SLOT_NUM * mSlotSize)
  , mFrameType(0)
  , mIsStarted(false)
  , mDuringPlayoutDelay(false)
  , mPlayoutDelayMs(kPlayoutDelayMs)
  , mPlayoutDelayTimer(nullptr) {
    MOZ_COUNT_CTOR(RtspTrackBuffer);
    mTrackIdx = aTrackIdx;
    MOZ_ASSERT(mSlotSize < UINT32_MAX / BUFFER_SLOT_NUM);
    mRingBuffer = new uint8_t[mTotalBufferSize];
    Reset();
  };
  ~RtspTrackBuffer() {
    MOZ_COUNT_DTOR(RtspTrackBuffer);
    mRingBuffer = nullptr;
  };

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const {
    
    size_t size = aMallocSizeOf(this);

    
    size += mRingBuffer.SizeOfExcludingThis(aMallocSizeOf);

    return size;
  }

  void Start() {
    MonitorAutoLock monitor(mMonitor);
    mIsStarted = true;
    mFrameType = 0;
  }
  void Stop() {
    MonitorAutoLock monitor(mMonitor);
    mIsStarted = false;
    StopPlayoutDelay();
  }

  
  
  
  
  
  nsresult ReadBuffer(uint8_t* aToBuffer, uint32_t aToBufferSize,
                      uint32_t& aReadCount, uint64_t& aFrameTime,
                      uint32_t& aFrameSize);
  
  void WriteBuffer(const char *aFromBuffer, uint32_t aWriteCount,
                   uint64_t aFrameTime, uint32_t aFrameType);
  
  
  void Reset();

  
  
  
  void ResetWithFrameType(uint32_t aFrameType) {
    SetFrameType(aFrameType);
    Reset();
  }

  
  
  
  void StartPlayoutDelay() {
    mDuringPlayoutDelay = true;
  }
  void LockStartPlayoutDelay() {
    MonitorAutoLock monitor(mMonitor);
    StartPlayoutDelay();
  }

  
  void StopPlayoutDelay() {
    if (mPlayoutDelayTimer) {
      mPlayoutDelayTimer->Cancel();
      mPlayoutDelayTimer = nullptr;
    }
    mDuringPlayoutDelay = false;
  }
  void LockStopPlayoutDelay() {
    MonitorAutoLock monitor(mMonitor);
    StopPlayoutDelay();
  }

  bool IsBufferOverThreshold();
  void CreatePlayoutDelayTimer(unsigned long delayMs);
  static void PlayoutDelayTimerCallback(nsITimer *aTimer, void *aClosure);

private:
  
  void SetFrameType(uint32_t aFrameType) {
    MonitorAutoLock monitor(mMonitor);
    mFrameType = mFrameType | aFrameType;
  }

  
  Monitor mMonitor;
  
  int32_t mTrackIdx;
  
  
  
  int32_t mProducerIdx;
  int32_t mConsumerIdx;

  
  
  
  
  
  
  
  BufferSlotData mBufferSlotData[BUFFER_SLOT_NUM];

  
  nsAutoArrayPtr<uint8_t> mRingBuffer;
  
  uint32_t mSlotSize;
  
  uint32_t mTotalBufferSize;
  
  
  
  uint32_t mFrameType;

  
  bool mIsStarted;

  
  bool mDuringPlayoutDelay;
  
  uint32_t mPlayoutDelayMs;
  
  nsCOMPtr<nsITimer> mPlayoutDelayTimer;
};

nsresult RtspTrackBuffer::ReadBuffer(uint8_t* aToBuffer, uint32_t aToBufferSize,
                                     uint32_t& aReadCount, uint64_t& aFrameTime,
                                     uint32_t& aFrameSize)
{
  MonitorAutoLock monitor(mMonitor);
  RTSPMLOG("ReadBuffer mTrackIdx %d mProducerIdx %d mConsumerIdx %d "
           "mBufferSlotData[mConsumerIdx].mLength %d"
           ,mTrackIdx ,mProducerIdx ,mConsumerIdx
           ,mBufferSlotData[mConsumerIdx].mLength);
  
  
  
  
  
  
  while (1) {
    
    
    if (!mIsStarted) {
      RTSPMLOG("ReadBuffer: mIsStarted is false");
      return NS_ERROR_FAILURE;
    }

    
    if (mDuringPlayoutDelay) {
      monitor.Wait();
      continue;
    }

    if (mBufferSlotData[mConsumerIdx].mFrameType & MEDIASTREAM_FRAMETYPE_END_OF_STREAM) {
      return NS_BASE_STREAM_CLOSED;
    }

    if (mBufferSlotData[mConsumerIdx].mLength > 0) {
      
      if ((int32_t)aToBufferSize < mBufferSlotData[mConsumerIdx].mLength) {
        aFrameSize = mBufferSlotData[mConsumerIdx].mLength;
        break;
      }
      uint32_t slots = mBufferSlotData[mConsumerIdx].mLength / mSlotSize;
      if (mBufferSlotData[mConsumerIdx].mLength % mSlotSize > 0) {
        slots++;
      }
      
      MOZ_ASSERT(mBufferSlotData[mConsumerIdx].mLength <=
                 (int32_t)((BUFFER_SLOT_NUM - mConsumerIdx) * mSlotSize));
      memcpy(aToBuffer,
             (void *)(&mRingBuffer[mSlotSize * mConsumerIdx]),
             mBufferSlotData[mConsumerIdx].mLength);

      aFrameSize = aReadCount = mBufferSlotData[mConsumerIdx].mLength;
      aFrameTime = mBufferSlotData[mConsumerIdx].mTime;
      RTSPMLOG("DataLength %d, data time %lld"
               ,mBufferSlotData[mConsumerIdx].mLength
               ,mBufferSlotData[mConsumerIdx].mTime);
      
      
      for (uint32_t i = mConsumerIdx; i < mConsumerIdx + slots; ++i) {
        mBufferSlotData[i].mLength = BUFFER_SLOT_EMPTY;
        mBufferSlotData[i].mTime = BUFFER_SLOT_EMPTY;
      }
      mConsumerIdx = (mConsumerIdx + slots) % BUFFER_SLOT_NUM;
      break;
    } else if (mBufferSlotData[mConsumerIdx].mLength == BUFFER_SLOT_INVALID) {
      mConsumerIdx = (mConsumerIdx + 1) % BUFFER_SLOT_NUM;
      RTSPMLOG("BUFFER_SLOT_INVALID move forward");
    } else {
      
      
      
      RTSPMLOG("monitor.Wait()");
      monitor.Wait();
    }
  }
  return NS_OK;
}



















void RtspTrackBuffer::WriteBuffer(const char *aFromBuffer, uint32_t aWriteCount,
                                  uint64_t aFrameTime, uint32_t aFrameType)
{
  MonitorAutoLock monitor(mMonitor);
  if (!mIsStarted) {
    RTSPMLOG("mIsStarted is false");
    return;
  }
  if (mTotalBufferSize < aWriteCount) {
    RTSPMLOG("mTotalBufferSize < aWriteCount, incoming data is too large");
    return;
  }
  
  
  
  if (aFrameType & MEDIASTREAM_FRAMETYPE_DISCONTINUITY) {
    mFrameType = mFrameType & (~MEDIASTREAM_FRAMETYPE_DISCONTINUITY);
    RTSPMLOG("Clear mFrameType");
    return;
  }
  
  
  
  
  if (mFrameType & MEDIASTREAM_FRAMETYPE_DISCONTINUITY) {
    RTSPMLOG("Return because the mFrameType is set");
    return;
  }

  
  if (mDuringPlayoutDelay && !mPlayoutDelayTimer) {
    CreatePlayoutDelayTimer(mPlayoutDelayMs);
  }

  
  bool isMultipleSlots = false;
  
  bool returnToHead = false;
  
  int32_t slots = aWriteCount / mSlotSize;
  if (aWriteCount % mSlotSize > 0) {
    slots++;
  }
  int32_t i;
  RTSPMLOG("WriteBuffer mTrackIdx %d mProducerIdx %d mConsumerIdx %d",
           mTrackIdx, mProducerIdx,mConsumerIdx);
  if (aWriteCount > mSlotSize) {
    isMultipleSlots = true;
  }
  if (isMultipleSlots &&
      (aWriteCount > (BUFFER_SLOT_NUM - mProducerIdx) * mSlotSize)) {
    returnToHead = true;
  }
  RTSPMLOG("slots %d isMultipleSlots %d returnToHead %d",
           slots, isMultipleSlots, returnToHead);
  if (returnToHead) {
    
    for (i = mProducerIdx; i < BUFFER_SLOT_NUM; ++i) {
      mBufferSlotData[i].mLength = BUFFER_SLOT_INVALID;
    }
    
    
    
    if (mProducerIdx <= mConsumerIdx && mConsumerIdx < mProducerIdx + slots) {
      mConsumerIdx = 0;
      for (i = mConsumerIdx; i < BUFFER_SLOT_NUM; ++i) {
        if (mBufferSlotData[i].mLength > 0) {
          mConsumerIdx = i;
          break;
        }
      }
    }
    mProducerIdx = 0;
  }

  if (!(aFrameType & MEDIASTREAM_FRAMETYPE_END_OF_STREAM)) {
    memcpy(&(mRingBuffer[mSlotSize * mProducerIdx]), aFromBuffer, aWriteCount);
  }

  
  
  if (mDuringPlayoutDelay && IsBufferOverThreshold()) {
    StopPlayoutDelay();
  }

  if (mProducerIdx <= mConsumerIdx && mConsumerIdx < mProducerIdx + slots
      && mBufferSlotData[mConsumerIdx].mLength > 0) {
    
    RTSPMLOG("overwrite!! %d time %lld"
             ,mTrackIdx,mBufferSlotData[mConsumerIdx].mTime);
    if (aFrameType & MEDIASTREAM_FRAMETYPE_END_OF_STREAM) {
      mBufferSlotData[mProducerIdx].mLength = 0;
      mBufferSlotData[mProducerIdx].mTime = 0;
      StopPlayoutDelay();
    } else {
      mBufferSlotData[mProducerIdx].mLength = aWriteCount;
      mBufferSlotData[mProducerIdx].mTime = aFrameTime;
    }
    mBufferSlotData[mProducerIdx].mFrameType = aFrameType;
    
    if (isMultipleSlots) {
      for (i = mProducerIdx + 1; i < mProducerIdx + slots; ++i) {
        mBufferSlotData[i].mLength = BUFFER_SLOT_INVALID;
      }
    }
    mProducerIdx = (mProducerIdx + slots) % BUFFER_SLOT_NUM;
    
    
    mConsumerIdx = mProducerIdx;
  } else {
    
    if (aFrameType & MEDIASTREAM_FRAMETYPE_END_OF_STREAM) {
      mBufferSlotData[mProducerIdx].mLength = 0;
      mBufferSlotData[mProducerIdx].mTime = 0;
      StopPlayoutDelay();
    } else {
      mBufferSlotData[mProducerIdx].mLength = aWriteCount;
      mBufferSlotData[mProducerIdx].mTime = aFrameTime;
    }
    mBufferSlotData[mProducerIdx].mFrameType = aFrameType;
    
    if (isMultipleSlots) {
      for (i = mProducerIdx + 1; i < mProducerIdx + slots; ++i) {
        mBufferSlotData[i].mLength = BUFFER_SLOT_INVALID;
      }
    }
    mProducerIdx = (mProducerIdx + slots) % BUFFER_SLOT_NUM;
  }

  mMonitor.NotifyAll();
}

void RtspTrackBuffer::Reset() {
  MonitorAutoLock monitor(mMonitor);
  mProducerIdx = 0;
  mConsumerIdx = 0;
  for (uint32_t i = 0; i < BUFFER_SLOT_NUM; ++i) {
    mBufferSlotData[i].mLength = BUFFER_SLOT_EMPTY;
    mBufferSlotData[i].mTime = BUFFER_SLOT_EMPTY;
    mBufferSlotData[i].mFrameType = MEDIASTREAM_FRAMETYPE_NORMAL;
  }
  StopPlayoutDelay();
  mMonitor.NotifyAll();
}

bool
RtspTrackBuffer::IsBufferOverThreshold()
{
  static int32_t numSlotsThreshold =
    BUFFER_SLOT_NUM * kBufferThresholdPerc;

  int32_t numSlotsUsed = mProducerIdx - mConsumerIdx;
  if (numSlotsUsed < 0) {  
    numSlotsUsed = (BUFFER_SLOT_NUM - mConsumerIdx) + mProducerIdx;
  }
  if (numSlotsUsed > numSlotsThreshold) {
    return true;
  }

  return false;
}

void
RtspTrackBuffer::CreatePlayoutDelayTimer(unsigned long delayMs)
{
  if (delayMs <= 0) {
    return;
  }
  mPlayoutDelayTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mPlayoutDelayTimer) {
    mPlayoutDelayTimer->InitWithFuncCallback(PlayoutDelayTimerCallback,
                                             this, delayMs,
                                             nsITimer::TYPE_ONE_SHOT);
  }
}


void
RtspTrackBuffer::PlayoutDelayTimerCallback(nsITimer *aTimer,
                                           void *aClosure)
{
  MOZ_ASSERT(aTimer);
  MOZ_ASSERT(aClosure);

  RtspTrackBuffer *self = static_cast<RtspTrackBuffer*>(aClosure);
  MonitorAutoLock lock(self->mMonitor);
  self->StopPlayoutDelay();
  lock.NotifyAll();
}




RtspMediaResource::RtspMediaResource(MediaDecoder* aDecoder,
    nsIChannel* aChannel, nsIURI* aURI, const nsACString& aContentType)
  : BaseMediaResource(aDecoder, aChannel, aURI, aContentType)
  , mIsConnected(false)
  , mIsLiveStream(false)
  , mHasTimestamp(true)
  , mIsSuspend(true)
{
#ifndef NECKO_PROTOCOL_rtsp
  MOZ_CRASH("Should not be called except for B2G platform");
#else
  MOZ_ASSERT(aChannel);
  mMediaStreamController =
    static_cast<RtspChannelChild*>(aChannel)->GetController();
  MOZ_ASSERT(mMediaStreamController);
  mListener = new Listener(this);
  mMediaStreamController->AsyncOpen(mListener);
#ifdef PR_LOGGING
  if (!gRtspMediaResourceLog) {
    gRtspMediaResourceLog = PR_NewLogModule("RtspMediaResource");
  }
#endif
#endif
}

RtspMediaResource::~RtspMediaResource()
{
  RTSPMLOG("~RtspMediaResource");
  if (mListener) {
    
    mListener->Revoke();
  }
}

void RtspMediaResource::SetSuspend(bool aIsSuspend)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");
  RTSPMLOG("SetSuspend %d",aIsSuspend);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethodWithArg<bool>(this, &RtspMediaResource::NotifySuspend,
                                      aIsSuspend);
  NS_DispatchToMainThread(runnable);
}

void RtspMediaResource::NotifySuspend(bool aIsSuspend)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  RTSPMLOG("NotifySuspend %d",aIsSuspend);

  mIsSuspend = aIsSuspend;
  if (mDecoder) {
    mDecoder->NotifySuspendedStatusChanged();
  }
}

size_t
RtspMediaResource::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t size = BaseMediaResource::SizeOfExcludingThis(aMallocSizeOf);
  size += mTrackBuffer.SizeOfExcludingThis(aMallocSizeOf);

  
  for (size_t i = 0; i < mTrackBuffer.Length(); i++) {
    size += mTrackBuffer[i]->SizeOfIncludingThis(aMallocSizeOf);
  }

  
  

  return size;
}




NS_IMPL_ISUPPORTS(RtspMediaResource::Listener,
                  nsIInterfaceRequestor, nsIStreamingProtocolListener);

nsresult
RtspMediaResource::Listener::OnMediaDataAvailable(uint8_t aTrackIdx,
                                                  const nsACString &data,
                                                  uint32_t length,
                                                  uint32_t offset,
                                                  nsIStreamingProtocolMetaData *meta)
{
  if (!mResource)
    return NS_OK;
  return mResource->OnMediaDataAvailable(aTrackIdx, data, length, offset, meta);
}

nsresult
RtspMediaResource::Listener::OnConnected(uint8_t aTrackIdx,
                                         nsIStreamingProtocolMetaData *meta)
{
  if (!mResource)
    return NS_OK;
  return mResource->OnConnected(aTrackIdx, meta);
}

nsresult
RtspMediaResource::Listener::OnDisconnected(uint8_t aTrackIdx, nsresult reason)
{
  if (!mResource)
    return NS_OK;
  return mResource->OnDisconnected(aTrackIdx, reason);
}

nsresult
RtspMediaResource::Listener::GetInterface(const nsIID & aIID, void **aResult)
{
  return QueryInterface(aIID, aResult);
}

void
RtspMediaResource::Listener::Revoke()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");
  if (mResource) {
    mResource = nullptr;
  }
}

nsresult
RtspMediaResource::ReadFrameFromTrack(uint8_t* aBuffer, uint32_t aBufferSize,
                                      uint32_t aTrackIdx, uint32_t& aBytes,
                                      uint64_t& aTime, uint32_t& aFrameSize)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");
  NS_ASSERTION(aTrackIdx < mTrackBuffer.Length(),
               "ReadTrack index > mTrackBuffer");
  MOZ_ASSERT(aBuffer);

  if (!mIsConnected) {
    RTSPMLOG("ReadFrameFromTrack: RTSP not connected");
    return NS_ERROR_FAILURE;
  }

  return mTrackBuffer[aTrackIdx]->ReadBuffer(aBuffer, aBufferSize, aBytes,
                                             aTime, aFrameSize);
}

nsresult
RtspMediaResource::OnMediaDataAvailable(uint8_t aTrackIdx,
                                        const nsACString &data,
                                        uint32_t length,
                                        uint32_t offset,
                                        nsIStreamingProtocolMetaData *meta)
{
  uint64_t time;
  uint32_t frameType;
  meta->GetTimeStamp(&time);
  meta->GetFrameType(&frameType);
  mTrackBuffer[aTrackIdx]->WriteBuffer(data.BeginReading(), length, time,
                                       frameType);
  return NS_OK;
}


bool
RtspMediaResource::IsVideoEnabled()
{
  return Preferences::GetBool("media.rtsp.video.enabled", false);
}

bool
RtspMediaResource::IsVideo(uint8_t tracks, nsIStreamingProtocolMetaData *meta)
{
  bool isVideo = false;
  for (int i = 0; i < tracks; ++i) {
    nsCOMPtr<nsIStreamingProtocolMetaData> trackMeta;
    mMediaStreamController->GetTrackMetaData(i, getter_AddRefs(trackMeta));
    MOZ_ASSERT(trackMeta);
    uint32_t w = 0, h = 0;
    trackMeta->GetWidth(&w);
    trackMeta->GetHeight(&h);
    if (w > 0 || h > 0) {
      isVideo = true;
      break;
    }
  }
  return isVideo;
}

nsresult
RtspMediaResource::OnConnected(uint8_t aTrackIdx,
                               nsIStreamingProtocolMetaData *meta)
{
  if (mIsConnected) {
    for (uint32_t i = 0 ; i < mTrackBuffer.Length(); ++i) {
      mTrackBuffer[i]->Start();
    }
    return NS_OK;
  }

  uint8_t tracks;
  mMediaStreamController->GetTotalTracks(&tracks);

  
  
  if (!IsVideoEnabled() && IsVideo(tracks, meta)) {
    
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::DecodeError);
    NS_DispatchToMainThread(event);
    return NS_ERROR_FAILURE;
  }
  uint64_t durationUs = 0;
  for (int i = 0; i < tracks; ++i) {
    nsCString rtspTrackId("RtspTrack");
    rtspTrackId.AppendInt(i);
    nsCOMPtr<nsIStreamingProtocolMetaData> trackMeta;
    mMediaStreamController->GetTrackMetaData(i, getter_AddRefs(trackMeta));
    MOZ_ASSERT(trackMeta);
    trackMeta->GetDuration(&durationUs);

    
    
    
    
    uint32_t w, h;
    uint32_t slotSize;
    trackMeta->GetWidth(&w);
    trackMeta->GetHeight(&h);
    slotSize = clamped((int32_t)(w * h), BUFFER_SLOT_DEFAULT_SIZE,
                       BUFFER_SLOT_MAX_SIZE);
    mTrackBuffer.AppendElement(new RtspTrackBuffer(rtspTrackId.get(),
                                                   i, slotSize));
    mTrackBuffer[i]->Start();
  }

  if (!mDecoder) {
    return NS_ERROR_FAILURE;
  }

  
  if (durationUs) {
    
    mIsLiveStream = false;
    mDecoder->SetInfinite(false);
    mDecoder->SetDuration((double)(durationUs) / USECS_PER_S);
  } else {
    
    
    if (!Preferences::GetBool("media.realtime_decoder.enabled", false)) {
      
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(mDecoder, &MediaDecoder::DecodeError);
      NS_DispatchToMainThread(event);
      return NS_ERROR_FAILURE;
    } else {
      mIsLiveStream = true;
      bool seekable = false;
      mDecoder->SetInfinite(true);
      mDecoder->SetMediaSeekable(seekable);
    }
  }
  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  NS_ENSURE_TRUE(owner, NS_ERROR_FAILURE);
  
  owner->DownloadProgressed();

  dom::HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);

  element->FinishDecoderSetup(mDecoder, this);
  mIsConnected = true;

  return NS_OK;
}

nsresult
RtspMediaResource::OnDisconnected(uint8_t aTrackIdx, nsresult aReason)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  for (uint32_t i = 0 ; i < mTrackBuffer.Length(); ++i) {
    mTrackBuffer[i]->Stop();
    mTrackBuffer[i]->Reset();
  }

  if (mDecoder) {
    if (aReason == NS_ERROR_NOT_INITIALIZED ||
        aReason == NS_ERROR_CONNECTION_REFUSED ||
        aReason == NS_ERROR_NOT_CONNECTED ||
        aReason == NS_ERROR_NET_TIMEOUT) {
      
      RTSPMLOG("Error in OnDisconnected 0x%x", aReason);
      mIsConnected = false;
      mDecoder->NetworkError();
    } else {
      
      
      mDecoder->ResetConnectionState();
    }
  }

  if (mListener) {
    
    
    mListener->Revoke();
  }

  return NS_OK;
}

void RtspMediaResource::Suspend(bool aCloseImmediately)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  mIsSuspend = true;
  if (NS_WARN_IF(!mDecoder)) {
    return;
  }

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  NS_ENSURE_TRUE_VOID(owner);
  dom::HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE_VOID(element);

  mMediaStreamController->Suspend();
  element->DownloadSuspended();
  mDecoder->NotifySuspendedStatusChanged();
}

void RtspMediaResource::Resume()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  mIsSuspend = false;
  if (NS_WARN_IF(!mDecoder)) {
    return;
  }

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  NS_ENSURE_TRUE_VOID(owner);
  dom::HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE_VOID(element);

  if (mChannel) {
    element->DownloadResumed();
  }
  mMediaStreamController->Resume();
  mDecoder->NotifySuspendedStatusChanged();
}

nsresult RtspMediaResource::Open(nsIStreamListener **aStreamListener)
{
  return NS_OK;
}

nsresult RtspMediaResource::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  mMediaStreamController->Stop();
  
  
  
  if (mDecoder) {
    mDecoder = nullptr;
  }
  return NS_OK;
}

already_AddRefed<nsIPrincipal> RtspMediaResource::GetCurrentPrincipal()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsCOMPtr<nsIPrincipal> principal;
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  if (!secMan || !mChannel)
    return nullptr;
  secMan->GetChannelResultPrincipal(mChannel, getter_AddRefs(principal));
  return principal.forget();
}

nsresult RtspMediaResource::SeekTime(int64_t aOffset)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  RTSPMLOG("Seek requested for aOffset [%lld] for decoder [%p]",
           aOffset, mDecoder);
  
  for(uint32_t i = 0 ; i < mTrackBuffer.Length(); ++i) {
    mTrackBuffer[i]->ResetWithFrameType(MEDIASTREAM_FRAMETYPE_DISCONTINUITY);
  }

  return mMediaStreamController->Seek(aOffset);
}

void
RtspMediaResource::EnablePlayoutDelay()
{
  for (uint32_t i = 0; i < mTrackBuffer.Length(); ++i) {
    mTrackBuffer[i]->LockStartPlayoutDelay();
  }
}

void
RtspMediaResource::DisablePlayoutDelay()
{
  for (uint32_t i = 0; i < mTrackBuffer.Length(); ++i) {
    mTrackBuffer[i]->LockStopPlayoutDelay();
  }
}

} 

