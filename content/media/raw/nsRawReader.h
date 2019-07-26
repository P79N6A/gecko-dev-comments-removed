




#if !defined(nsRawReader_h_)
#define nsRawReader_h_

#include "nsBuiltinDecoderReader.h"
#include "nsRawStructs.h"

class nsRawReader : public nsBuiltinDecoderReader
{
public:
  nsRawReader(nsBuiltinDecoder* aDecoder);
  ~nsRawReader();

  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();
  virtual bool DecodeAudioData();

  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                  int64_t aTimeThreshold);

  virtual bool HasAudio()
  {
    return false;
  }

  virtual bool HasVideo()
  {
    return true;
  }

  virtual nsresult ReadMetadata(nsVideoInfo* aInfo,
                                nsHTMLMediaElement::MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, int64_t aStartTime);

  
  bool IsSeekableInBufferedRanges() {
    return true;
  }

private:
  bool ReadFromResource(MediaResource *aResource, uint8_t *aBuf, uint32_t aLength);

  nsRawVideoHeader mMetadata;
  uint32_t mCurrentFrame;
  double mFrameRate;
  uint32_t mFrameSize;
  nsIntRect mPicture;
};

#endif
