




#ifndef OpusTrackEncoder_h_
#define OpusTrackEncoder_h_

#include <stdint.h>
#include <speex/speex_resampler.h>
#include "TrackEncoder.h"
#include "nsCOMPtr.h"

struct OpusEncoder;

namespace mozilla {

class OpusTrackEncoder : public AudioTrackEncoder
{
public:
  OpusTrackEncoder();
  virtual ~OpusTrackEncoder();

  nsresult GetHeader(nsTArray<uint8_t>* aOutput) MOZ_OVERRIDE;

  nsresult GetEncodedTrack(nsTArray<uint8_t>* aOutput, int &aOutputDuration) MOZ_OVERRIDE;

protected:
  int GetPacketDuration() MOZ_OVERRIDE;

  nsresult Init(int aChannels, int aSamplingRate) MOZ_OVERRIDE;

private:
  enum {
    ID_HEADER,
    COMMENT_HEADER,
    DATA
  } mEncoderState;

  



  int GetOutputSampleRate();

  


  OpusEncoder* mEncoder;

  




  nsAutoPtr<AudioSegment> mSourceSegment;

  





  int mLookahead;

  



  SpeexResamplerState* mResampler;
};

}
#endif
