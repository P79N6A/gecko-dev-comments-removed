





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

nsRefPtr<MediaDecoderReader::SeekPromise>
RtspMediaCodecReader::Seek(int64_t aTime, int64_t aEndTime)
{
  
  
  
  
  mRtspResource->SeekTime(aTime);

  return MediaCodecReader::Seek(aTime, aEndTime);
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
                                       int64_t aTimeThreshold,
                                       bool aForceDecodeAhead)
{
  EnsureActive();
  return MediaCodecReader::RequestVideoData(aSkipToNextKeyframe,
                                            aTimeThreshold,
                                            aForceDecodeAhead);
}

nsRefPtr<MediaDecoderReader::MetadataPromise>
RtspMediaCodecReader::AsyncReadMetadata()
{
  mRtspResource->DisablePlayoutDelay();
  EnsureActive();

  nsRefPtr<MediaDecoderReader::MetadataPromise> p =
    MediaCodecReader::AsyncReadMetadata();

  
  
  SetIdle();

  return p;
}

void
RtspMediaCodecReader::HandleResourceAllocated()
{
  EnsureActive();
  MediaCodecReader::HandleResourceAllocated();
  mRtspResource->EnablePlayoutDelay();;
}

} 
