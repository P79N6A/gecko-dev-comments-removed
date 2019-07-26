




#ifndef OpusTrackEncoder_h_
#define OpusTrackEncoder_h_

#include <stdint.h>
#include <speex/speex_resampler.h>
#include "TrackEncoder.h"

struct OpusEncoder;

namespace mozilla {


class OpusMetadata : public TrackMetadataBase
{
public:
  
  nsTArray<uint8_t> mIdHeader;
  
  nsTArray<uint8_t> mCommentHeader;

  MetadataKind GetKind() const MOZ_OVERRIDE { return METADATA_OPUS; }
};

class OpusTrackEncoder : public AudioTrackEncoder
{
public:
  OpusTrackEncoder();
  virtual ~OpusTrackEncoder();

  already_AddRefed<TrackMetadataBase> GetMetadata() MOZ_OVERRIDE;

  nsresult GetEncodedTrack(EncodedFrameContainer& aData) MOZ_OVERRIDE;

protected:
  int GetPacketDuration();

  nsresult Init(int aChannels, int aSamplingRate) MOZ_OVERRIDE;

  



  int GetOutputSampleRate();

private:
  


  OpusEncoder* mEncoder;

  





  AudioSegment mSourceSegment;

  





  int mLookahead;

  



  SpeexResamplerState* mResampler;

  



  nsTArray<AudioDataValue> mResampledLeftover;
};

}
#endif
