




#if !defined(WMFReader_h_)
#define WMFReader_h_

#include "WMF.h"
#include "MediaDecoderReader.h"
#include "nsAutoPtr.h"
#include "mozilla/RefPtr.h"
#include "nsRect.h"

namespace mozilla {

class WMFByteStream;
class WMFSourceReaderCallback;
class DXVA2Manager;



class WMFReader : public MediaDecoderReader
{
public:
  WMFReader(AbstractMediaDecoder* aDecoder);

  virtual ~WMFReader();

  nsresult Init(MediaDecoderReader* aCloneDonor) override;

  bool DecodeAudioData() override;
  bool DecodeVideoFrame(bool &aKeyframeSkip,
                        int64_t aTimeThreshold) override;

  bool HasAudio() override;
  bool HasVideo() override;

  nsresult ReadMetadata(MediaInfo* aInfo,
                        MetadataTags** aTags) override;

  nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) override;

  bool IsMediaSeekable() override;
  
private:

  HRESULT CreateSourceReader();
  HRESULT ConfigureAudioDecoder();
  HRESULT ConfigureVideoDecoder();
  HRESULT ConfigureVideoFrameGeometry(IMFMediaType* aMediaType);
  void GetSupportedAudioCodecs(const GUID** aCodecs, uint32_t* aNumCodecs);

  HRESULT CreateBasicVideoFrame(IMFSample* aSample,
                                int64_t aTimestampUsecs,
                                int64_t aDurationUsecs,
                                int64_t aOffsetBytes,
                                VideoData** aOutVideoData);

  HRESULT CreateD3DVideoFrame(IMFSample* aSample,
                              int64_t aTimestampUsecs,
                              int64_t aDurationUsecs,
                              int64_t aOffsetBytes,
                              VideoData** aOutVideoData);

  
  bool InitializeDXVA();  

  nsresult SeekInternal(int64_t aTime);

  RefPtr<IMFSourceReader> mSourceReader;
  RefPtr<WMFByteStream> mByteStream;
  RefPtr<WMFSourceReaderCallback> mSourceReaderCallback;
  nsAutoPtr<DXVA2Manager> mDXVA2Manager;

  
  
  nsIntRect mPictureRegion;

  uint32_t mAudioChannels;
  uint32_t mAudioBytesPerSample;
  uint32_t mAudioRate;

  uint32_t mVideoWidth;
  uint32_t mVideoHeight;
  uint32_t mVideoStride;

  
  
  int64_t mAudioFrameOffset;
  
  
  int64_t mAudioFrameSum;
  
  
  
  bool mMustRecaptureAudioPosition;

  bool mHasAudio;
  bool mHasVideo;
  bool mUseHwAccel;

  
  
  
  const bool mIsMP3Enabled;

  bool mCOMInitialized;
};

} 

#endif
