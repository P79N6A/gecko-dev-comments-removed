




#ifndef OmxTrackEncoder_h_
#define OmxTrackEncoder_h_

#include "TrackEncoder.h"

namespace android {
class OMXVideoEncoder;
class OMXAudioEncoder;
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
  nsresult Init(int aWidth, int aHeight,
                int aDisplayWidth, int aDisplayHeight,
                TrackRate aTrackRate) MOZ_OVERRIDE;

private:
  nsAutoPtr<android::OMXVideoEncoder> mEncoder;
};

class OmxAudioTrackEncoder MOZ_FINAL : public AudioTrackEncoder
{
public:
  OmxAudioTrackEncoder()
    : AudioTrackEncoder()
  {}

  already_AddRefed<TrackMetadataBase> GetMetadata() MOZ_OVERRIDE;

  nsresult GetEncodedTrack(EncodedFrameContainer& aData) MOZ_OVERRIDE;

protected:
  nsresult Init(int aChannels, int aSamplingRate) MOZ_OVERRIDE;

private:
  
  nsresult AppendEncodedFrames(EncodedFrameContainer& aContainer);

  nsAutoPtr<android::OMXAudioEncoder> mEncoder;
};

}
#endif
