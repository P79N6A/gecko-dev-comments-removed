




#if !defined(WMFReader_h_)
#define WMFReader_h_

#include "WMF.h"
#include "MediaDecoderReader.h"

namespace mozilla {

class WMFByteStream;
class WMFSourceReaderCallback;



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

  nsresult ReadMetadata(VideoInfo* aInfo,
                        MetadataTags** aTags) MOZ_OVERRIDE;

  nsresult Seek(int64_t aTime,
                int64_t aStartTime,
                int64_t aEndTime,
                int64_t aCurrentTime) MOZ_OVERRIDE;

  nsresult GetBuffered(nsTimeRanges* aBuffered,
                       int64_t aStartTime) MOZ_OVERRIDE;

  void OnDecodeThreadStart() MOZ_OVERRIDE;
  void OnDecodeThreadFinish() MOZ_OVERRIDE;

private:

  HRESULT ConfigureAudioDecoder();
  HRESULT ConfigureVideoDecoder();
  HRESULT ConfigureVideoFrameGeometry(IMFMediaType* aMediaType);

  RefPtr<IMFSourceReader> mSourceReader;
  RefPtr<WMFByteStream> mByteStream;
  RefPtr<WMFSourceReaderCallback> mSourceReaderCallback;

  
  
  nsIntRect mPictureRegion;

  uint32_t mAudioChannels;
  uint32_t mAudioBytesPerSample;
  uint32_t mAudioRate;

  uint32_t mVideoHeight;
  uint32_t mVideoStride;

  bool mHasAudio;
  bool mHasVideo;
  bool mCanSeek;
};

} 

#endif
