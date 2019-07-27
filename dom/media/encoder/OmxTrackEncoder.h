




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
  OmxVideoTrackEncoder();
  ~OmxVideoTrackEncoder();

  already_AddRefed<TrackMetadataBase> GetMetadata() override;

  nsresult GetEncodedTrack(EncodedFrameContainer& aData) override;

protected:
  nsresult Init(int aWidth, int aHeight,
                int aDisplayWidth, int aDisplayHeight,
                TrackRate aTrackRate) override;

private:
  nsAutoPtr<android::OMXVideoEncoder> mEncoder;
};

class OmxAudioTrackEncoder : public AudioTrackEncoder
{
public:
  OmxAudioTrackEncoder();
  ~OmxAudioTrackEncoder();

  already_AddRefed<TrackMetadataBase> GetMetadata() = 0;

  nsresult GetEncodedTrack(EncodedFrameContainer& aData) override;

protected:
  nsresult Init(int aChannels, int aSamplingRate) = 0;

  
  nsresult AppendEncodedFrames(EncodedFrameContainer& aContainer);

  nsAutoPtr<android::OMXAudioEncoder> mEncoder;
};

class OmxAACAudioTrackEncoder final : public OmxAudioTrackEncoder
{
public:
  OmxAACAudioTrackEncoder()
    : OmxAudioTrackEncoder()
  {}

  already_AddRefed<TrackMetadataBase> GetMetadata() override;

protected:
  nsresult Init(int aChannels, int aSamplingRate) override;
};

class OmxAMRAudioTrackEncoder final : public OmxAudioTrackEncoder
{
public:
  OmxAMRAudioTrackEncoder()
    : OmxAudioTrackEncoder()
  {}

  enum {
    AMR_NB_SAMPLERATE = 8000,
  };
  already_AddRefed<TrackMetadataBase> GetMetadata() override;

protected:
  nsresult Init(int aChannels, int aSamplingRate) override;
};

}
#endif
