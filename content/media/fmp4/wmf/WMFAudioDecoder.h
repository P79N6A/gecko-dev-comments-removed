





#if !defined(WMFAudioDecoder_h_)
#define WMFAudioDecoder_h_

#include "WMF.h"
#include "MP4Reader.h"
#include "MFTDecoder.h"

namespace mozilla {

class WMFAudioDecoder : public MediaDataDecoder {
public:
  WMFAudioDecoder(uint32_t aChannelCount,
                  uint32_t aSampleRate,
                  uint16_t aBitsPerSample,
                  const uint8_t* aUserData,
                  uint32_t aUserDataLength);

  virtual nsresult Init() MOZ_OVERRIDE;

  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  virtual DecoderStatus Input(nsAutoPtr<mp4_demuxer::MP4Sample>& aSample);

  
  virtual DecoderStatus Output(nsAutoPtr<MediaData>& aOutData);

  virtual DecoderStatus Flush() MOZ_OVERRIDE;

private:


  
  
  
  
  
  
  DecoderStatus OutputNonNegativeTimeSamples(nsAutoPtr<MediaData>& aOutData);

  nsAutoPtr<MFTDecoder> mDecoder;

  uint32_t mAudioChannels;
  uint32_t mAudioBytesPerSample;
  uint32_t mAudioRate;
  nsTArray<BYTE> mUserData;

  
  
  int64_t mLastStreamOffset;

  
  
  int64_t mAudioFrameOffset;
  
  
  int64_t mAudioFrameSum;
  
  
  
  bool mMustRecaptureAudioPosition;
};



} 

#endif
