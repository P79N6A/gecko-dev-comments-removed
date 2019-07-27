




#if !defined(RawReader_h_)
#define RawReader_h_

#include "MediaDecoderReader.h"
#include "RawStructs.h"

namespace mozilla {

class RawReader : public MediaDecoderReader
{
public:
  explicit RawReader(AbstractMediaDecoder* aDecoder);

protected:
  ~RawReader();

public:
  virtual nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE;
  virtual nsresult ResetDecode() MOZ_OVERRIDE;
  virtual bool DecodeAudioData() MOZ_OVERRIDE;

  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                  int64_t aTimeThreshold) MOZ_OVERRIDE;

  virtual bool HasAudio() MOZ_OVERRIDE
  {
    return false;
  }

  virtual bool HasVideo() MOZ_OVERRIDE
  {
    return true;
  }

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) MOZ_OVERRIDE;
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime) MOZ_OVERRIDE;

  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered) MOZ_OVERRIDE;

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

private:
  bool ReadFromResource(MediaResource *aResource, uint8_t *aBuf, uint32_t aLength);

  nsresult SeekInternal(int64_t aTime);

  RawVideoHeader mMetadata;
  uint32_t mCurrentFrame;
  double mFrameRate;
  uint32_t mFrameSize;
  nsIntRect mPicture;
};

} 

#endif
