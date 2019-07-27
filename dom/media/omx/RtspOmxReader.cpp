





#include "RtspOmxReader.h"

#include "AbstractMediaDecoder.h"
#include "MediaDecoderStateMachine.h"
#include "OmxDecoder.h"
#include "RtspExtractor.h"
#include "RtspMediaResource.h"
#include "RtspOmxDecoder.h"

using namespace android;

namespace mozilla {

nsresult RtspOmxReader::InitOmxDecoder()
{
  if (!mOmxDecoder.get()) {
    NS_ASSERTION(mDecoder, "RtspOmxReader mDecoder is null.");
    NS_ASSERTION(mDecoder->GetResource(),
                 "RtspOmxReader mDecoder->GetResource() is null.");
    mExtractor = new RtspExtractor(mRtspResource);
    mOmxDecoder = new OmxDecoder(mDecoder);
    if (!mOmxDecoder->Init(mExtractor)) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

nsRefPtr<MediaDecoderReader::SeekPromise>
RtspOmxReader::Seek(int64_t aTime, int64_t aEndTime)
{
  
  
  
  
  if (mRtspResource) {
    mRtspResource->SeekTime(aTime);
    mRtspResource->EnablePlayoutDelay();
  }

  
  
  
  
  mEnsureActiveFromSeek = true;
  return MediaOmxReader::Seek(aTime, aEndTime);
}

void RtspOmxReader::SetIdle() {
  
  MediaOmxReader::SetIdle();

  
  if (mRtspResource) {
    nsIStreamingProtocolController* controller =
        mRtspResource->GetMediaStreamController();
    if (controller) {
      controller->Pause();
    }
    mRtspResource->SetSuspend(true);
  }
}

void RtspOmxReader::EnsureActive() {
  
  if (mRtspResource) {
    nsIStreamingProtocolController* controller =
        mRtspResource->GetMediaStreamController();
    
    
    
    if (controller && !mEnsureActiveFromSeek) {
      controller->Play();
    }
    mEnsureActiveFromSeek = false;
    mRtspResource->SetSuspend(false);
  }

  
  MediaOmxReader::EnsureActive();
}

nsRefPtr<MediaDecoderReader::MetadataPromise>
RtspOmxReader::AsyncReadMetadata()
{
  
  
  mRtspResource->DisablePlayoutDelay();

  nsRefPtr<MediaDecoderReader::MetadataPromise> p =
    MediaOmxReader::AsyncReadMetadata();

  
  
  SetIdle();

  return p;
}

void RtspOmxReader::HandleResourceAllocated()
{
  MediaOmxReader::HandleResourceAllocated();
  mRtspResource->EnablePlayoutDelay();
}

} 
