




#ifndef OmxTrackEncoder_h_
#define OmxTrackEncoder_h_

#include "TrackEncoder.h"

namespace android {
class OMXVideoEncoder;
}









namespace mozilla {

class OmxVideoTrackEncoder: public VideoTrackEncoder
{
public:
  OmxVideoTrackEncoder()
    : VideoTrackEncoder()
  {}

  already_AddRefed<TrackMetadataBase> GetMetadata() MOZ_OVERRIDE;

  nsresult GetEncodedTrack(EncodedFrameContainer& aData) MOZ_OVERRIDE;

protected:
  nsresult Init(int aWidth, int aHeight, TrackRate aTrackRate) MOZ_OVERRIDE;

private:
  nsAutoPtr<android::OMXVideoEncoder> mEncoder;
};

}
#endif
