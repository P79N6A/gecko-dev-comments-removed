




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
  virtual nsresult Init(MediaDecoderReader* aCloneDonor) override;
  virtual nsresult ResetDecode() override;
  virtual bool DecodeAudioData() override;

  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                  int64_t aTimeThreshold) override;

  virtual bool HasAudio() override
  {
    return false;
  }

  virtual bool HasVideo() override
  {
    return true;
  }

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) override;
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) override;

  virtual media::TimeIntervals GetBuffered() override;

  virtual bool IsMediaSeekable() override;

private:
  bool ReadFromResource(MediaResource *aResource, uint8_t *aBuf, uint32_t aLength);

  RawVideoHeader mMetadata;
  uint32_t mCurrentFrame;
  double mFrameRate;
  uint32_t mFrameSize;
  nsIntRect mPicture;
};

} 

#endif
