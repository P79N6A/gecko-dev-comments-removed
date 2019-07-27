




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

  already_AddRefed<TrackMetadataBase> GetMetadata() final override;

  nsresult GetEncodedTrack(EncodedFrameContainer& aData) final override;

protected:
  




  int GetPacketDuration() final override {
    return 1024;
  }

  nsresult Init(int aChannels, int aSamplingRate) final override;

private:
  
  void WriteLacing(nsTArray<uint8_t> *aOutput, int32_t aLacing);

  
  void GetEncodedFrames(EncodedFrameContainer& aData);

  
  
  vorbis_info mVorbisInfo;
  
  vorbis_dsp_state mVorbisDsp;
  
  vorbis_block mVorbisBlock;
};

} 

#endif
