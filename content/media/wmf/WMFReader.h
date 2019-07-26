




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

namespace dom {
class TimeRanges;
}



class WMFReader : public MediaDecoderReader
{
public:
  WMFReader(AbstractMediaDecoder* aDecoder);

  virtual ~WMFReader();

  nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE;

  bool DecodeAudioData() MOZ_OVERRIDE;
  bool DecodeVideoFrame(bool &aKeyframeSkip,
                        int64_t aTimeThreshold) MOZ_OVERRIDE;

  bool HasAudio() MOZ_OVERRIDE;
  bool HasVideo() MOZ_OVERRIDE;

  nsresult ReadMetadata(MediaInfo* aInfo,
                        MetadataTags** aTags) MOZ_OVERRIDE;

  nsresult Seek(int64_t aTime,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime) MOZ_OVERRIDE;

  nsresult GetBuffered(mozilla::dom::TimeRanges* aBuffered,
                       int64_t aStartTime) MOZ_OVERRIDE;

  void OnDecodeThreadStart() MOZ_OVERRIDE;
  void OnDecodeThreadFinish() MOZ_OVERRIDE;

private:

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
