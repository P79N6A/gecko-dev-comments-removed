





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
  virtual size_t countTracks() MOZ_FINAL MOZ_OVERRIDE;
  virtual android::sp<android::MediaSource> getTrack(size_t index)
    MOZ_FINAL MOZ_OVERRIDE;
  virtual android::sp<android::MetaData> getTrackMetaData(
    size_t index, uint32_t flag = 0) MOZ_FINAL MOZ_OVERRIDE;
  virtual uint32_t flags() const MOZ_FINAL MOZ_OVERRIDE;

  RtspExtractor(RtspMediaResource* aResource)
    : mRtspResource(aResource) {
    MOZ_ASSERT(aResource);
    mController = mRtspResource->GetMediaStreamController();
    MOZ_ASSERT(mController);
  }
  virtual ~RtspExtractor() MOZ_OVERRIDE {}
private:
  
  
  RtspMediaResource* mRtspResource;
  
  
  nsRefPtr<nsIStreamingProtocolController> mController;
};

} 

#endif
