













#include "prlog.h"
#include "VideoUtils.h"
#include "SegmentBase.h"
#include "MediaDecoderStateMachine.h"
#include "DASHReader.h"
#include "MediaResource.h"
#include "DASHRepDecoder.h"
#include "WebMReader.h"
#include <algorithm>

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define LOG(msg, ...) PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, \
                             ("%p [DASHRepDecoder] " msg, this, __VA_ARGS__))
#define LOG1(msg) PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, \
                         ("%p [DASHRepDecoder] " msg, this))
#else
#define LOG(msg, ...)
#define LOG1(msg)
#endif

MediaDecoderStateMachine*
DASHRepDecoder::CreateStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  return mDecoderStateMachine;
}

nsresult
DASHRepDecoder::SetStateMachine(MediaDecoderStateMachine* aSM)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoderStateMachine = aSM;
  return NS_OK;
}

void
DASHRepDecoder::SetResource(MediaResource* aResource)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mResource = aResource;
}

void
DASHRepDecoder::SetMPDRepresentation(Representation const * aRep)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mMPDRepresentation = aRep;
}

void
DASHRepDecoder::SetReader(WebMReader* aReader)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mReader = aReader;
}

nsresult
DASHRepDecoder::Load(MediaResource* aResource,
                     nsIStreamListener** aListener,
                     MediaDecoder* aCloneDonor)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ENSURE_TRUE(mMPDRepresentation, NS_ERROR_NOT_INITIALIZED);

  
  SegmentBase const * segmentBase = mMPDRepresentation->GetSegmentBase();
  NS_ENSURE_TRUE(segmentBase, NS_ERROR_NULL_POINTER);

  
  segmentBase->GetInitRange(&mInitByteRange.mStart, &mInitByteRange.mEnd);
  NS_ENSURE_TRUE(!mInitByteRange.IsNull(), NS_ERROR_NOT_INITIALIZED);
  mReader->SetInitByteRange(mInitByteRange);

  
  segmentBase->GetIndexRange(&mIndexByteRange.mStart, &mIndexByteRange.mEnd);
  NS_ENSURE_TRUE(!mIndexByteRange.IsNull(), NS_ERROR_NOT_INITIALIZED);
  mReader->SetIndexByteRange(mIndexByteRange);

  
  
  
  
  
  
  
  
  int64_t delta = std::max(mIndexByteRange.mStart, mInitByteRange.mStart)
                - std::min(mIndexByteRange.mEnd, mInitByteRange.mEnd);
  MediaByteRange byteRange;
  if (delta <= SEEK_VS_READ_THRESHOLD) {
    byteRange.mStart = std::min(mIndexByteRange.mStart, mInitByteRange.mStart);
    byteRange.mEnd = std::max(mIndexByteRange.mEnd, mInitByteRange.mEnd);
    
    mMetadataChunkCount = 1;
  } else {
    byteRange = mInitByteRange;
    
    mMetadataChunkCount = 2;
  }
  mCurrentByteRange = byteRange;
  return mResource->OpenByteRange(nullptr, byteRange);
}

void
DASHRepDecoder::NotifyDownloadEnded(nsresult aStatus)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (!mMainDecoder) {
    LOG("Error! Main Decoder is reported as null: mMainDecoder [%p]",
        mMainDecoder.get());
    DecodeError();
    return;
  }

  if (NS_SUCCEEDED(aStatus)) {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    
    
    if (mMetadataChunkCount > 0) {
      LOG("Metadata chunk [%d] downloaded: range requested [%lld - %lld] "
          "subsegmentIdx [%d]",
          mMetadataChunkCount,
          mCurrentByteRange.mStart, mCurrentByteRange.mEnd, mSubsegmentIdx);
      mMetadataChunkCount--;
    } else {
      LOG("Byte range downloaded: status [%x] range requested [%lld - %lld] "
          "subsegmentIdx [%d]",
          aStatus, mCurrentByteRange.mStart, mCurrentByteRange.mEnd,
          mSubsegmentIdx);
      if ((uint32_t)mSubsegmentIdx == mByteRanges.Length()-1) {
        mResource->NotifyLastByteRange();
      }
      
      mMainDecoder->NotifyDownloadEnded(this, aStatus, mSubsegmentIdx);
    }
  } else if (aStatus == NS_BINDING_ABORTED) {
    LOG("MPD download has been cancelled by the user: aStatus [%x].", aStatus);
    if (mMainDecoder) {
      mMainDecoder->LoadAborted();
    }
    return;
  } else if (aStatus != NS_BASE_STREAM_CLOSED) {
    LOG("Network error trying to download MPD: aStatus [%x].", aStatus);
    NetworkError();
  }
}

void
DASHRepDecoder::OnReadMetadataCompleted()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  
  if (mShuttingDown) {
    LOG1("Shutting down! Ignoring OnReadMetadataCompleted().");
    return;
  }

  LOG1("Metadata has been read.");

  
  nsresult rv = PopulateByteRanges();
  if (NS_FAILED(rv) || mByteRanges.IsEmpty()) {
    LOG("Error populating byte ranges [%x]", rv);
    DecodeError();
    return;
  }

  mMainDecoder->OnReadMetadataCompleted(this);
}

nsresult
DASHRepDecoder::PopulateByteRanges()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  
  NS_ENSURE_FALSE(mShuttingDown, NS_ERROR_UNEXPECTED);

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (!mByteRanges.IsEmpty()) {
    return NS_OK;
  }
  NS_ENSURE_TRUE(mReader, NS_ERROR_NULL_POINTER);
  LOG1("Populating byte range array.");
  return mReader->GetSubsegmentByteRanges(mByteRanges);
}

void
DASHRepDecoder::LoadNextByteRange()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ASSERTION(mResource, "Error: resource is reported as null!");

  
  if (mShuttingDown) {
    LOG1("Shutting down! Ignoring LoadNextByteRange().");
    return;
  }

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  NS_ASSERTION(mMainDecoder, "Error: main decoder is null!");
  NS_ASSERTION(mMainDecoder->IsDecoderAllowedToDownloadData(this),
               "Should not be called on non-active decoders!");

  
  if (mByteRanges.IsEmpty()) {
    LOG1("Error getting list of subsegment byte ranges.");
    DecodeError();
    return;
  }

  
  int32_t subsegmentIdx = mMainDecoder->GetSubsegmentIndex(this);
  NS_ASSERTION(0 <= subsegmentIdx,
               "Subsegment index should be >= 0 for active decoders");
  if (subsegmentIdx >= 0 && (uint32_t)subsegmentIdx < mByteRanges.Length()) {
    mCurrentByteRange = mByteRanges[subsegmentIdx];
    mSubsegmentIdx = subsegmentIdx;
  } else {
    mCurrentByteRange.Clear();
    mSubsegmentIdx = -1;
    LOG("End of subsegments: index [%d] out of range.", subsegmentIdx);
    return;
  }

  
  
  
  if (subsegmentIdx == 0) {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mReader->RequestSeekToSubsegment(0);
  }

  
  if (IsSubsegmentCached(mSubsegmentIdx)) {
    LOG("Subsegment [%d] bytes [%lld] to [%lld] already cached. No need to "
        "download.", mSubsegmentIdx,
        mCurrentByteRange.mStart, mCurrentByteRange.mEnd);
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &DASHRepDecoder::DoNotifyDownloadEnded);
    nsresult rv = NS_DispatchToMainThread(event);
    if (NS_FAILED(rv)) {
      LOG("Error notifying subsegment [%d] cached: rv[0x%x].",
          mSubsegmentIdx, rv);
      NetworkError();
    }
    return;
  }

  
  nsresult rv = mResource->OpenByteRange(nullptr, mCurrentByteRange);
  if (NS_FAILED(rv)) {
    LOG("Error opening byte range [%lld - %lld]: subsegmentIdx [%d] rv [%x].",
        mCurrentByteRange.mStart, mCurrentByteRange.mEnd, mSubsegmentIdx, rv);
    NetworkError();
    return;
  }
}

void
DASHRepDecoder::CancelByteRangeLoad()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ASSERTION(mResource, "Error: resource is reported as null!");

  if (mCurrentByteRange.IsNull() || mSubsegmentIdx < 0) {
    LOG1("Canceling current byte range load: none to cancel.");
    return;
  }
  LOG("Canceling current byte range load: [%lld] to [%lld] subsegment "
      "[%lld]", mCurrentByteRange.mStart, mCurrentByteRange.mEnd,
      mSubsegmentIdx);

  mResource->CancelByteRangeOpen();
}

bool
DASHRepDecoder::IsSubsegmentCached(int32_t aSubsegmentIdx)
{
  GetReentrantMonitor().AssertCurrentThreadIn();

  MediaByteRange byteRange = mByteRanges[aSubsegmentIdx];
  int64_t start = mResource->GetNextCachedData(byteRange.mStart);
  int64_t end = mResource->GetCachedDataEnd(byteRange.mStart);
  return (start == byteRange.mStart &&
          end >= byteRange.mEnd);
}

void
DASHRepDecoder::DoNotifyDownloadEnded()
{
  NotifyDownloadEnded(NS_OK);
}

nsresult
DASHRepDecoder::GetByteRangeForSeek(int64_t const aOffset,
                                    MediaByteRange& aByteRange)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());

  for (uint32_t i = 0; i < mByteRanges.Length(); i++) {
    NS_ENSURE_FALSE(mByteRanges[i].IsNull(), NS_ERROR_NOT_INITIALIZED);
    
    if (mByteRanges[i].mStart <= aOffset && aOffset <= mByteRanges[i].mEnd) {
      if (mMainDecoder->IsDecoderAllowedToDownloadSubsegment(this, i)) {
        mCurrentByteRange = aByteRange = mByteRanges[i];
        mSubsegmentIdx = i;
        
        
        mMainDecoder->SetSubsegmentIndex(this, i);
        LOG("Getting DATA range [%d] for seek offset [%lld]: "
            "bytes [%lld] to [%lld]",
            i, aOffset, aByteRange.mStart, aByteRange.mEnd);
        return NS_OK;
      }
      break;
    }
  }
  
  
  bool canDownloadMetadata = mByteRanges.IsEmpty();
  if (canDownloadMetadata) {
    
    if (mInitByteRange.mStart <= aOffset && aOffset <= mInitByteRange.mEnd) {
      mCurrentByteRange = aByteRange = mInitByteRange;
      mSubsegmentIdx = 0;
        LOG("Getting INIT range for seek offset [%lld]: bytes [%lld] to "
            "[%lld]", aOffset, aByteRange.mStart, aByteRange.mEnd);
      return NS_OK;
    }
    
    if (mIndexByteRange.mStart <= aOffset && aOffset <= mIndexByteRange.mEnd) {
      mCurrentByteRange = aByteRange = mIndexByteRange;
      mSubsegmentIdx = 0;
      LOG("Getting INDEXES range for seek offset [%lld]: bytes [%lld] to "
          "[%lld]", aOffset, aByteRange.mStart, aByteRange.mEnd);
      return NS_OK;
    }
  } else {
    LOG1("Metadata should be read; inhibiting further metadata downloads.");
  }

  
  aByteRange.Clear();
  if (mByteRanges.IsEmpty() || !canDownloadMetadata) {
    
    LOG("Data ranges not populated [%s]; metadata download restricted [%s]: "
        "offset[%lld].",
        (mByteRanges.IsEmpty() ? "yes" : "no"),
        (canDownloadMetadata ? "no" : "yes"), aOffset);
    return NS_ERROR_NOT_AVAILABLE;
  } else {
    
    
    LOG("Error! Offset [%lld] is in an unknown range!", aOffset);
    return NS_ERROR_ILLEGAL_VALUE;
  }
}

void
DASHRepDecoder::PrepareForSwitch()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  mResource->FlushCache();
}

void
DASHRepDecoder::NetworkError()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->NetworkError(); }
}

void
DASHRepDecoder::SetDuration(double aDuration)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->SetDuration(aDuration); }
}

void
DASHRepDecoder::SetInfinite(bool aInfinite)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->SetInfinite(aInfinite); }
}

void
DASHRepDecoder::SetMediaSeekable(bool aMediaSeekable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->SetMediaSeekable(aMediaSeekable); }
}

void
DASHRepDecoder::Progress(bool aTimer)
{
  if (mMainDecoder) { mMainDecoder->Progress(aTimer); }
}

void
DASHRepDecoder::NotifyDataArrived(const char* aBuffer,
                                  uint32_t aLength,
                                  int64_t aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  LOG("Data bytes [%lld - %lld] arrived via buffer [%p].",
      aOffset, aOffset+aLength, aBuffer);
  
  
  
  if (mReader) {
    mReader->NotifyDataArrived(aBuffer, aLength, aOffset);
  }
  
  if (mMainDecoder) {
    mMainDecoder->NotifyDataArrived(aBuffer, aLength, aOffset);
  }
}

void
DASHRepDecoder::NotifyBytesDownloaded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->NotifyBytesDownloaded(); }
}

void
DASHRepDecoder::NotifySuspendedStatusChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->NotifySuspendedStatusChanged(); }
}

bool
DASHRepDecoder::OnStateMachineThread() const
{
  return (mMainDecoder ? mMainDecoder->OnStateMachineThread() : false);
}

bool
DASHRepDecoder::OnDecodeThread() const
{
  return (mMainDecoder ? mMainDecoder->OnDecodeThread() : false);
}

ReentrantMonitor&
DASHRepDecoder::GetReentrantMonitor()
{
  NS_ASSERTION(mMainDecoder, "Can't get monitor if main decoder is null!");
  if (mMainDecoder) {
    return mMainDecoder->GetReentrantMonitor();
  } else {
    
    
    
    return MediaDecoder::GetReentrantMonitor();
  }
}

mozilla::layers::ImageContainer*
DASHRepDecoder::GetImageContainer()
{
  return (mMainDecoder ? mMainDecoder->GetImageContainer() : nullptr);
}

void
DASHRepDecoder::DecodeError()
{
  if (NS_IsMainThread()) {
    MediaDecoder::DecodeError();
  } else {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &MediaDecoder::DecodeError);
    nsresult rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      LOG("Error dispatching DecodeError event to main thread: rv[%x]", rv);
    }
  }
}

void
DASHRepDecoder::ReleaseStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");

  
  mReader = nullptr;

  MediaDecoder::ReleaseStateMachine();
}

} 
