





#if !defined(WMFAudioOutputSource_h_)
#define WMFAudioOutputSource_h_

#include "WMF.h"
#include "MP4Reader.h"
#include "MFTDecoder.h"
#include "mozilla/RefPtr.h"
#include "WMFMediaDataDecoder.h"

namespace mozilla {

class WMFAudioMFTManager : public MFTManager {
public:
  WMFAudioMFTManager(const mp4_demuxer::AudioDecoderConfig& aConfig);
  ~WMFAudioMFTManager();

  virtual TemporaryRef<MFTDecoder> Init() MOZ_OVERRIDE;

  virtual HRESULT Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;

  
  
  
  virtual HRESULT Output(int64_t aStreamOffset,
                         nsAutoPtr<MediaData>& aOutput) MOZ_OVERRIDE;
private:

  HRESULT UpdateOutputType();

  
  RefPtr<MFTDecoder> mDecoder;

  uint32_t mAudioChannels;
  const uint32_t mAudioBytesPerSample;
  uint32_t mAudioRate;
  nsTArray<BYTE> mUserData;

  
  
  int64_t mAudioFrameOffset;
  
  
  int64_t mAudioFrameSum;

  
  
  
  bool mMustRecaptureAudioPosition;
};

} 

#endif 
