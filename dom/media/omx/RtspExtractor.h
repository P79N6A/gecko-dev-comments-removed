





#if !defined(RtspExtractor_h_)
#define RtspExtractor_h_

#include "RtspMediaResource.h"

#include <stagefright/MediaBufferGroup.h>
#include <stagefright/MediaExtractor.h>
#include <stagefright/MediaSource.h>
#include <stagefright/MetaData.h>

namespace mozilla {







class RtspExtractor: public android::MediaExtractor
{
public:
  virtual size_t countTracks() final override;
  virtual android::sp<android::MediaSource> getTrack(size_t index)
    final override;
  virtual android::sp<android::MetaData> getTrackMetaData(
    size_t index, uint32_t flag = 0) final override;
  virtual uint32_t flags() const final override;

  RtspExtractor(RtspMediaResource* aResource)
    : mRtspResource(aResource) {
    MOZ_ASSERT(aResource);
    mController = mRtspResource->GetMediaStreamController();
    MOZ_ASSERT(mController);
  }
  virtual ~RtspExtractor() override {}
private:
  
  
  RtspMediaResource* mRtspResource;
  
  
  nsRefPtr<nsIStreamingProtocolController> mController;
};

} 

#endif
