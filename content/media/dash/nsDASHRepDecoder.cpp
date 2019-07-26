













#include "prlog.h"
#include "VideoUtils.h"
#include "SegmentBase.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "nsDASHReader.h"
#include "MediaResource.h"
#include "nsDASHRepDecoder.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(msg, ...) PR_LOG(gBuiltinDecoderLog, PR_LOG_DEBUG, \
                             ("%p [nsDASHRepDecoder] " msg, this, __VA_ARGS__))
#define LOG1(msg) PR_LOG(gBuiltinDecoderLog, PR_LOG_DEBUG, \
                         ("%p [nsDASHRepDecoder] " msg, this))
#else
#define LOG(msg, ...)
#define LOG1(msg)
#endif

nsDecoderStateMachine*
nsDASHRepDecoder::CreateStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  return mDecoderStateMachine;
}

nsresult
nsDASHRepDecoder::SetStateMachine(nsDecoderStateMachine* aSM)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoderStateMachine = aSM;
  return NS_OK;
}

void
nsDASHRepDecoder::SetResource(MediaResource* aResource)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mResource = aResource;
}

void
nsDASHRepDecoder::SetMPDRepresentation(Representation const * aRep)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mMPDRepresentation = aRep;
}

void
nsDASHRepDecoder::SetReader(nsWebMReader* aReader)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mReader = aReader;
}

nsresult
nsDASHRepDecoder::Load(MediaResource* aResource,
                       nsIStreamListener** aListener,
                       nsMediaDecoder* aCloneDonor)
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

  
  
  
  
  
  
  
  
  int64_t delta = NS_MAX(mIndexByteRange.mStart, mInitByteRange.mStart)
                - NS_MIN(mIndexByteRange.mEnd, mInitByteRange.mEnd);
  MediaByteRange byteRange;
  if (delta <= SEEK_VS_READ_THRESHOLD) {
    byteRange.mStart = NS_MIN(mIndexByteRange.mStart, mInitByteRange.mStart);
    byteRange.mEnd = NS_MAX(mIndexByteRange.mEnd, mInitByteRange.mEnd);
    
    mMetadataChunkCount = 1;
  } else {
    byteRange = mInitByteRange;
    
    mMetadataChunkCount = 2;
  }
  mCurrentByteRange = byteRange;
  return mResource->OpenByteRange(nullptr, byteRange);
}

void
nsDASHRepDecoder::NotifyDownloadEnded(nsresult aStatus)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  if (!mMainDecoder) {
    LOG("Error! Main Decoder is reported as null: mMainDecoder [%p]",
        mMainDecoder.get());
    DecodeError();
    return;
  }

  if (NS_SUCCEEDED(aStatus)) {
    
    
    if (mMetadataChunkCount > 0) {
      LOG("Metadata chunk [%d] downloaded: range requested [%d - %d]",
          mMetadataChunkCount,
          mCurrentByteRange.mStart, mCurrentByteRange.mEnd);
      mMetadataChunkCount--;
    } else {
      
      LOG("Byte range downloaded: status [%x] range requested [%d - %d]",
          aStatus, mCurrentByteRange.mStart, mCurrentByteRange.mEnd);
      mMainDecoder->NotifyDownloadEnded(this, aStatus,
                                        mCurrentByteRange);
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
nsDASHRepDecoder::OnReadMetadataCompleted()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  LOG1("Metadata has been read.");
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &nsDASHRepDecoder::LoadNextByteRange);
  nsresult rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    LOG("Error dispatching parse event to main thread: rv[%x]", rv);
    DecodeError();
    return;
  }
}

void
nsDASHRepDecoder::LoadNextByteRange()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (!mResource) {
    LOG1("Error: resource is reported as null!");
    DecodeError();
    return;
  }

  
  nsresult rv;
  if (mByteRanges.IsEmpty()) {
    if (!mReader) {
      LOG1("Error: mReader should not be null!");
      DecodeError();
      return;
    }
    rv = mReader->GetIndexByteRanges(mByteRanges);
    
    if (NS_FAILED(rv) || mByteRanges.IsEmpty()) {
      LOG1("Error getting list of subsegment byte ranges.");
      DecodeError();
      return;
    }
  }

  
  if (mSubsegmentIdx < mByteRanges.Length()) {
    mCurrentByteRange = mByteRanges[mSubsegmentIdx];
  } else {
    mCurrentByteRange.Clear();
    LOG("End of subsegments: index [%d] out of range.", mSubsegmentIdx);
    return;
  }

  
  rv = mResource->OpenByteRange(nullptr, mCurrentByteRange);
  if (NS_FAILED(rv)) {
    LOG("Error opening byte range [%d - %d]: rv [%x].",
        mCurrentByteRange.mStart, mCurrentByteRange.mEnd, rv);
    NetworkError();
    return;
  }
  
  mSubsegmentIdx++;
}

nsresult
nsDASHRepDecoder::GetByteRangeForSeek(int64_t const aOffset,
                                      MediaByteRange& aByteRange)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  for (int i = 0; i < mByteRanges.Length(); i++) {
    NS_ENSURE_FALSE(mByteRanges[i].IsNull(), NS_ERROR_NOT_INITIALIZED);
    if (mByteRanges[i].mStart <= aOffset && aOffset <= mByteRanges[i].mEnd) {
      mCurrentByteRange = aByteRange = mByteRanges[i];
      mSubsegmentIdx = i;
      return NS_OK;
    }
  }
  
  if (mInitByteRange.mStart <= aOffset && aOffset <= mInitByteRange.mEnd) {
    mCurrentByteRange = aByteRange = mInitByteRange;
    mSubsegmentIdx = 0;
    return NS_OK;
  }
  
  if (mIndexByteRange.mStart <= aOffset && aOffset <= mIndexByteRange.mEnd) {
    mCurrentByteRange = aByteRange = mIndexByteRange;
    mSubsegmentIdx = 0;
    return NS_OK;
  }

  aByteRange.Clear();
  if (mByteRanges.IsEmpty()) {
    
    LOG("Can't get range for offset [%d].", aOffset);
    return NS_ERROR_NOT_AVAILABLE;
  } else {
    
    
    LOG("Error! Offset [%d] is in an unknown range!", aOffset);
    return NS_ERROR_ILLEGAL_VALUE;
  }
}

void
nsDASHRepDecoder::NetworkError()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->NetworkError(); }
}

void
nsDASHRepDecoder::SetDuration(double aDuration)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->SetDuration(aDuration); }
}

void
nsDASHRepDecoder::SetInfinite(bool aInfinite)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->SetInfinite(aInfinite); }
}

void
nsDASHRepDecoder::SetSeekable(bool aSeekable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->SetSeekable(aSeekable); }
}

void
nsDASHRepDecoder::Progress(bool aTimer)
{
  if (mMainDecoder) { mMainDecoder->Progress(aTimer); }
}

void
nsDASHRepDecoder::NotifyDataArrived(const char* aBuffer,
                                    uint32_t aLength,
                                    int64_t aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  LOG("Data bytes [%d - %d] arrived via buffer [%p].",
      aOffset, aOffset+aLength, aBuffer);
  
  
  
  if (mReader) {
    mReader->NotifyDataArrived(aBuffer, aLength, aOffset);
  }
  
  if (mMainDecoder) {
    mMainDecoder->NotifyDataArrived(aBuffer, aLength, aOffset);
  }
}

void
nsDASHRepDecoder::NotifyBytesDownloaded()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->NotifyBytesDownloaded(); }
}

void
nsDASHRepDecoder::NotifySuspendedStatusChanged()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (mMainDecoder) { mMainDecoder->NotifySuspendedStatusChanged(); }
}

bool
nsDASHRepDecoder::OnStateMachineThread() const
{
  return (mMainDecoder ? mMainDecoder->OnStateMachineThread() : false);
}

bool
nsDASHRepDecoder::OnDecodeThread() const
{
  return (mMainDecoder ? mMainDecoder->OnDecodeThread() : false);
}

ReentrantMonitor&
nsDASHRepDecoder::GetReentrantMonitor()
{
  return mMainDecoder->GetReentrantMonitor();
}

nsDecoderStateMachine::State
nsDASHRepDecoder::GetDecodeState()
{
  
  return (mMainDecoder ? mMainDecoder->GetDecodeState()
                       : nsDecoderStateMachine::DECODER_STATE_SHUTDOWN);
}

mozilla::layers::ImageContainer*
nsDASHRepDecoder::GetImageContainer()
{
  NS_ASSERTION(mMainDecoder && mMainDecoder->OnDecodeThread(),
               "Should be on decode thread.");
  return (mMainDecoder ? mMainDecoder->GetImageContainer() : nullptr);
}

void
nsDASHRepDecoder::DecodeError()
{
  if (NS_IsMainThread()) {
    nsBuiltinDecoder::DecodeError();
  } else {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsBuiltinDecoder::DecodeError);
    nsresult rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      LOG("Error dispatching DecodeError event to main thread: rv[%x]", rv);
    }
  }
}

void
nsDASHRepDecoder::ReleaseStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");

  
  mReader = nullptr;

  nsBuiltinDecoder::ReleaseStateMachine();
}
