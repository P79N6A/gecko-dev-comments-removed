




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

class OmxAudioTrackEncoder : public AudioTrackEncoder
{
public:
  OmxAudioTrackEncoder()
    : AudioTrackEncoder()
  {}

  already_AddRefed<TrackMetadataBase> GetMetadata() = 0;

  nsresult GetEncodedTrack(EncodedFrameContainer& aData) MOZ_OVERRIDE;

protected:
  nsresult Init(int aChannels, int aSamplingRate) = 0;

  
  nsresult AppendEncodedFrames(EncodedFrameContainer& aContainer);

  nsAutoPtr<android::OMXAudioEncoder> mEncoder;
};

class OmxAACAudioTrackEncoder MOZ_FINAL : public OmxAudioTrackEncoder
{
public:
  OmxAACAudioTrackEncoder()
    : OmxAudioTrackEncoder()
  {}

  already_AddRefed<TrackMetadataBase> GetMetadata() MOZ_OVERRIDE;

protected:
  nsresult Init(int aChannels, int aSamplingRate) MOZ_OVERRIDE;
};

class OmxAMRAudioTrackEncoder MOZ_FINAL : public OmxAudioTrackEncoder
{
public:
  OmxAMRAudioTrackEncoder()
    : OmxAudioTrackEncoder()
  {}

  enum {
    AMR_NB_SAMPLERATE = 8000,
  };
  already_AddRefed<TrackMetadataBase> GetMetadata() MOZ_OVERRIDE;

protected:
  nsresult Init(int aChannels, int aSamplingRate) MOZ_OVERRIDE;
};

}
#endif
