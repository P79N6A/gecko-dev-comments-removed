





#include "mozilla/DebugOnly.h"

#include "RtspMediaResource.h"

#include "MediaDecoder.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/Monitor.h"
#include "mozilla/Preferences.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStreamingProtocolService.h"
#include "nsServiceManagerUtils.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gRtspMediaResourceLog;
#define LOG(msg, ...) PR_LOG(gRtspMediaResourceLog, PR_LOG_DEBUG, \
                             (msg, ##__VA_ARGS__))

#define RTSPMLOG(msg, ...) \
        LOG("%p [RtspMediaResource]: " msg, this, ##__VA_ARGS__)
#else
#define LOG(msg, ...)
#define RTSPMLOG(msg, ...)
#endif

namespace mozilla {








#define BUFFER_SLOT_NUM 8192
#define BUFFER_SLOT_DEFAULT_SIZE 256
#define BUFFER_SLOT_MAX_SIZE 8192
#define BUFFER_SLOT_INVALID -1
#define BUFFER_SLOT_EMPTY 0

struct BufferSlotData {
  int32_t mLength;
  uint64_t mTime;
};

class RtspTrackBuffer
{
public:
  RtspTrackBuffer(const char *aMonitor, int32_t aTrackIdx, uint32_t aSlotSize)
  : mMonitor(aMonitor)
  , mSlotSize(aSlotSize)
  , mTotalBufferSize(BUFFER_SLOT_NUM * mSlotSize)
  , mFrameType(0)
  , mIsStarted(false) {
    MOZ_COUNT_CTOR(RtspTrackBuffer);
#ifdef PR_LOGGING
    mTrackIdx = aTrackIdx;
#endif
    MOZ_ASSERT(mSlotSize < UINT32_MAX / BUFFER_SLOT_NUM);
    mRingBuffer = new uint8_t[mTotalBufferSize];
    Reset();
  };
  ~RtspTrackBuffer() {
    MOZ_COUNT_DTOR(RtspTrackBuffer);
    mRingBuffer = nullptr;
  };
  void Start() {
    MonitorAutoLock monitor(mMonitor);
    mIsStarted = true;
  }
  void Stop() {
    MonitorAutoLock monitor(mMonitor);
    mIsStarted = false;
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

private:
  
  void SetFrameType(uint32_t aFrameType) {
    MonitorAutoLock monitor(mMonitor);
    mFrameType = mFrameType | aFrameType;
  }

  
  Monitor mMonitor;
#ifdef PR_LOGGING
  
  int32_t mTrackIdx;
#endif
  
  
  
  int32_t mProducerIdx;
  int32_t mConsumerIdx;

  
  
  
  
  
  
  
  BufferSlotData mBufferSlotData[BUFFER_SLOT_NUM];

  
  nsAutoArrayPtr<uint8_t> mRingBuffer;
  
  uint32_t mSlotSize;
  
  uint32_t mTotalBufferSize;
  
  
  
  uint32_t mFrameType;

  
  bool mIsStarted;
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
    if (mBufferSlotData[mConsumerIdx].mLength > 0) {
      
      if ((int32_t)aToBufferSize < mBufferSlotData[mConsumerIdx].mLength) {
        aFrameSize = mBufferSlotData[mConsumerIdx].mLength;
        break;
      }
      uint32_t slots = (mBufferSlotData[mConsumerIdx].mLength / mSlotSize) + 1;
      
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
      
      if (!mIsStarted) {
        return NS_ERROR_FAILURE;
      }
      
      
      
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
  
  bool isMultipleSlots = false;
  
  bool returnToHead = false;
  
  int32_t slots = 1;
  int32_t i;
  RTSPMLOG("WriteBuffer mTrackIdx %d mProducerIdx %d mConsumerIdx %d",
           mTrackIdx, mProducerIdx,mConsumerIdx);
  if (aWriteCount > mSlotSize) {
    isMultipleSlots = true;
    slots = (aWriteCount / mSlotSize) + 1;
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

  memcpy(&(mRingBuffer[mSlotSize * mProducerIdx]), aFromBuffer, aWriteCount);

  if (mProducerIdx <= mConsumerIdx && mConsumerIdx < mProducerIdx + slots
      && mBufferSlotData[mConsumerIdx].mLength > 0) {
    
    RTSPMLOG("overwrite!! %d time %lld"
             ,mTrackIdx,mBufferSlotData[mConsumerIdx].mTime);
    mBufferSlotData[mProducerIdx].mLength = aWriteCount;
    mBufferSlotData[mProducerIdx].mTime = aFrameTime;
    
    if (isMultipleSlots) {
      for (i = mProducerIdx + 1; i < mProducerIdx + slots; ++i) {
        mBufferSlotData[i].mLength = BUFFER_SLOT_INVALID;
      }
    }
    mProducerIdx = (mProducerIdx + slots) % BUFFER_SLOT_NUM;
    
    
    mConsumerIdx = mProducerIdx;
  } else {
    
    mBufferSlotData[mProducerIdx].mLength = aWriteCount;
    mBufferSlotData[mProducerIdx].mTime = aFrameTime;
    
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
  }
  mMonitor.NotifyAll();
}

RtspMediaResource::RtspMediaResource(MediaDecoder* aDecoder,
    nsIChannel* aChannel, nsIURI* aURI, const nsACString& aContentType)
  : BaseMediaResource(aDecoder, aChannel, aURI, aContentType)
  , mIsConnected(false)
  , mRealTime(false)
{
  nsCOMPtr<nsIStreamingProtocolControllerService> mediaControllerService =
    do_GetService(MEDIASTREAMCONTROLLERSERVICE_CONTRACTID);
  MOZ_ASSERT(mediaControllerService);
  if (mediaControllerService) {
    mediaControllerService->Create(mChannel,
                                   getter_AddRefs(mMediaStreamController));
    MOZ_ASSERT(mMediaStreamController);
    mListener = new Listener(this);
    mMediaStreamController->AsyncOpen(mListener);
  }
#ifdef PR_LOGGING
  if (!gRtspMediaResourceLog) {
    gRtspMediaResourceLog = PR_NewLogModule("RtspMediaResource");
  }
#endif
}

RtspMediaResource::~RtspMediaResource()
{
  RTSPMLOG("~RtspMediaResource");
  if (mListener) {
    
    mListener->Revoke();
  }
}

NS_IMPL_ISUPPORTS2(RtspMediaResource::Listener,
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
RtspMediaResource::Listener::OnDisconnected(uint8_t aTrackIdx, uint32_t reason)
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

nsresult
RtspMediaResource::ReadFrameFromTrack(uint8_t* aBuffer, uint32_t aBufferSize,
                                      uint32_t aTrackIdx, uint32_t& aBytes,
                                      uint64_t& aTime, uint32_t& aFrameSize)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");
  NS_ASSERTION(aTrackIdx < mTrackBuffer.Length(),
               "ReadTrack index > mTrackBuffer");
  MOZ_ASSERT(aBuffer);

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
  if (mRealTime) {
    time = 0;
  }
  mTrackBuffer[aTrackIdx]->WriteBuffer(data.BeginReading(), length, time,
                                       frameType);
  return NS_OK;
}

nsresult
RtspMediaResource::OnConnected(uint8_t aTrackIdx,
                               nsIStreamingProtocolMetaData *meta)
{
  if (mIsConnected) {
    return NS_OK;
  }

  uint8_t tracks;
  mMediaStreamController->GetTotalTracks(&tracks);
  uint64_t duration = 0;
  for (int i = 0; i < tracks; ++i) {
    nsCString rtspTrackId("RtspTrack");
    rtspTrackId.AppendInt(i);
    nsCOMPtr<nsIStreamingProtocolMetaData> trackMeta;
    mMediaStreamController->GetTrackMetaData(i, getter_AddRefs(trackMeta));
    MOZ_ASSERT(trackMeta);
    trackMeta->GetDuration(&duration);

    
    
    
    
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

  
  if (duration) {
    
    mRealTime = false;
    bool seekable = true;
    mDecoder->SetInfinite(false);
    mDecoder->SetTransportSeekable(seekable);
    mDecoder->SetDuration(duration);
  } else {
    
    
    if (!Preferences::GetBool("media.realtime_decoder.enabled", false)) {
      
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(mDecoder, &MediaDecoder::DecodeError);
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      return NS_ERROR_FAILURE;
    } else {
      mRealTime = true;
      bool seekable = false;
      mDecoder->SetInfinite(true);
      mDecoder->SetTransportSeekable(seekable);
      mDecoder->SetMediaSeekable(seekable);
    }
  }
  
  
  mDecoder->Progress(false);

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  NS_ENSURE_TRUE(owner, NS_ERROR_FAILURE);
  HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);

  element->FinishDecoderSetup(mDecoder, this);
  mIsConnected = true;

  return NS_OK;
}

nsresult
RtspMediaResource::OnDisconnected(uint8_t aTrackIdx, uint32_t aReason)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  for (uint32_t i = 0 ; i < mTrackBuffer.Length(); ++i) {
    mTrackBuffer[i]->Stop();
    mTrackBuffer[i]->Reset();
  }

  if (aReason == (uint32_t)NS_ERROR_CONNECTION_REFUSED) {
    mDecoder->NetworkError();
  }
  return NS_OK;
}

void RtspMediaResource::Suspend(bool aCloseImmediately)
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  NS_ENSURE_TRUE_VOID(owner);
  HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE_VOID(element);

  mMediaStreamController->Suspend();
  element->DownloadSuspended();
}

void RtspMediaResource::Resume()
{
  NS_ASSERTION(NS_IsMainThread(), "Don't call on non-main thread");

  MediaDecoderOwner* owner = mDecoder->GetMediaOwner();
  NS_ENSURE_TRUE_VOID(owner);
  HTMLMediaElement* element = owner->GetMediaElement();
  NS_ENSURE_TRUE_VOID(element);

  if (mChannel) {
    element->DownloadResumed();
  }
  mMediaStreamController->Resume();
}

nsresult RtspMediaResource::Open(nsIStreamListener **aStreamListener)
{
  return NS_OK;
}

nsresult RtspMediaResource::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  mMediaStreamController->Stop();
  return NS_OK;
}

already_AddRefed<nsIPrincipal> RtspMediaResource::GetCurrentPrincipal()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsCOMPtr<nsIPrincipal> principal;
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  if (!secMan || !mChannel)
    return nullptr;
  secMan->GetChannelPrincipal(mChannel, getter_AddRefs(principal));
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

} 

