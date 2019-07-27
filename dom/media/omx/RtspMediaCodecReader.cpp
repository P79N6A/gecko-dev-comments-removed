





#include "RtspMediaCodecReader.h"

#include "RtspExtractor.h"
#include "RtspMediaResource.h"
#include "RtspMediaCodecDecoder.h"

using namespace android;

namespace mozilla {

RtspMediaCodecReader::RtspMediaCodecReader(AbstractMediaDecoder* aDecoder)
  : MediaCodecReader(aDecoder)
{
  NS_ASSERTION(mDecoder, "RtspMediaCodecReader mDecoder is null.");
  NS_ASSERTION(mDecoder->GetResource(),
               "RtspMediaCodecReader mDecoder->GetResource() is null.");
  mRtspResource = mDecoder->GetResource()->GetRtspPointer();
  MOZ_ASSERT(mRtspResource);
}

RtspMediaCodecReader::~RtspMediaCodecReader() {}

bool
RtspMediaCodecReader::CreateExtractor()
{
  if (mExtractor != nullptr) {
    return true;
  }

  mExtractor = new RtspExtractor(mRtspResource);

  return mExtractor != nullptr;
}

void
RtspMediaCodecReader::Seek(int64_t aTime, int64_t aStartTime,
                           int64_t aEndTime, int64_t aCurrentTime)
{
  
  
  
  
  mRtspResource->SeekTime(aTime);

  MediaCodecReader::Seek(aTime, aStartTime, aEndTime, aCurrentTime);
}

void
RtspMediaCodecReader::SetIdle()
{
  nsIStreamingProtocolController* controller =
    mRtspResource->GetMediaStreamController();
  if (controller) {
    controller->Pause();
  }
  mRtspResource->SetSuspend(true);
}

void
RtspMediaCodecReader::EnsureActive()
{
  
  nsIStreamingProtocolController* controller =
    mRtspResource->GetMediaStreamController();
  if (controller) {
    controller->Play();
  }
  mRtspResource->SetSuspend(false);
}

nsRefPtr<MediaDecoderReader::AudioDataPromise>
RtspMediaCodecReader::RequestAudioData()
{
  EnsureActive();
  return MediaCodecReader::RequestAudioData();
}

nsRefPtr<MediaDecoderReader::VideoDataPromise>
RtspMediaCodecReader::RequestVideoData(bool aSkipToNextKeyframe,
                                       int64_t aTimeThreshold)
{
  EnsureActive();
  return MediaCodecReader::RequestVideoData(aSkipToNextKeyframe, aTimeThreshold);
}

nsresult
RtspMediaCodecReader::ReadMetadata(MediaInfo* aInfo,
                                   MetadataTags** aTags)
{
  mRtspResource->DisablePlayoutDelay();
  EnsureActive();
  nsresult rv = MediaCodecReader::ReadMetadata(aInfo, aTags);
  SetIdle();

  if (rv == NS_OK && !IsWaitingMediaResources()) {
    mRtspResource->EnablePlayoutDelay();
  }

  return rv;
}

} 
