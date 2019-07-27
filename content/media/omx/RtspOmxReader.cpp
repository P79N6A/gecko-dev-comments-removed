





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
    mOmxDecoder = new OmxDecoder(mDecoder->GetResource(), mDecoder);
    if (!mOmxDecoder->Init(mExtractor)) {
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

nsresult RtspOmxReader::Seek(int64_t aTime, int64_t aStartTime,
                             int64_t aEndTime, int64_t aCurrentTime)
{
  
  
  
  
  if (mRtspResource) {
    mRtspResource->SeekTime(aTime);
  }

  
  
  
  
  return MediaOmxReader::Seek(aTime, aStartTime, aEndTime, aCurrentTime);
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
    if (controller) {
      controller->Play();
    }
    mRtspResource->SetSuspend(false);
  }

  
  MediaOmxReader::EnsureActive();
}

} 
