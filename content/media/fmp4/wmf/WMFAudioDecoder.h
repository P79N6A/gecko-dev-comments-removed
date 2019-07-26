





#if !defined(WMFAudioDecoder_h_)
#define WMFAudioDecoder_h_

#include "wmf.h"
#include "MP4Reader.h"
#include "MFTDecoder.h"

namespace mozilla {

class WMFAudioDecoder : public MediaDataDecoder {
public:
  WMFAudioDecoder();

  nsresult Init(uint32_t aChannelCount,
                uint32_t aSampleRate,
                uint16_t aBitsPerSample,
                const uint8_t* aUserData,
                uint32_t aUserDataLength);

  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  virtual DecoderStatus Input(const uint8_t* aData,
                              uint32_t aLength,
                              Microseconds aDTS,
                              Microseconds aPTS,
                              int64_t aOffsetInStream);

  
  virtual DecoderStatus Output(nsAutoPtr<MediaData>& aOutData);

  virtual DecoderStatus Flush() MOZ_OVERRIDE;

private:


  
  
  
  
  
  
  DecoderStatus OutputNonNegativeTimeSamples(nsAutoPtr<MediaData>& aOutData);

  nsAutoPtr<MFTDecoder> mDecoder;

  uint32_t mAudioChannels;
  uint32_t mAudioBytesPerSample;
  uint32_t mAudioRate;

  
  
  int64_t mLastStreamOffset;

  
  
  int64_t mAudioFrameOffset;
  
  
  int64_t mAudioFrameSum;
  
  
  
  bool mMustRecaptureAudioPosition;
};



} 

#endif
