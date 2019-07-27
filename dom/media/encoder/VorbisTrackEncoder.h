




#ifndef VorbisTrackEncoder_h_
#define VorbisTrackEncoder_h_

#include "TrackEncoder.h"
#include "nsCOMPtr.h"
#include <vorbis/codec.h>

namespace mozilla {

class VorbisTrackEncoder : public AudioTrackEncoder
{
public:
  VorbisTrackEncoder();
  virtual ~VorbisTrackEncoder();

  already_AddRefed<TrackMetadataBase> GetMetadata() MOZ_FINAL MOZ_OVERRIDE;

  nsresult GetEncodedTrack(EncodedFrameContainer& aData) MOZ_FINAL MOZ_OVERRIDE;

protected:
  




  int GetPacketDuration() MOZ_FINAL MOZ_OVERRIDE {
    return 1024;
  }

  nsresult Init(int aChannels, int aSamplingRate) MOZ_FINAL MOZ_OVERRIDE;

private:
  
  void WriteLacing(nsTArray<uint8_t> *aOutput, int32_t aLacing);

  
  void GetEncodedFrames(EncodedFrameContainer& aData);

  
  
  vorbis_info mVorbisInfo;
  
  vorbis_dsp_state mVorbisDsp;
  
  vorbis_block mVorbisBlock;
};

}
#endif
