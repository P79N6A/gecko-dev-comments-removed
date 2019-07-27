





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

  virtual TemporaryRef<MFTDecoder> Init() override;

  virtual HRESULT Input(MediaRawData* aSample) override;

  
  
  
  virtual HRESULT Output(int64_t aStreamOffset,
                         nsRefPtr<MediaData>& aOutput) override;

  virtual void Shutdown() override;

private:

  HRESULT UpdateOutputType();

  
  RefPtr<MFTDecoder> mDecoder;

  uint32_t mAudioChannels;
  uint32_t mAudioRate;
  nsTArray<BYTE> mUserData;

  
  
  int64_t mAudioFrameOffset;
  
  
  int64_t mAudioFrameSum;

  enum StreamType {
    Unknown,
    AAC,
    MP3
  };
  StreamType mStreamType;

  const GUID& GetMFTGUID();
  const GUID& GetMediaSubtypeGUID();

  
  
  
  bool mMustRecaptureAudioPosition;
};

} 

#endif 
