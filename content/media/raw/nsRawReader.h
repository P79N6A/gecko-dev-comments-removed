






































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
                                  PRInt64 aTimeThreshold);

  virtual bool HasAudio()
  {
    return false;
  }

  virtual bool HasVideo()
  {
    return true;
  }

  virtual nsresult ReadMetadata(nsVideoInfo* aInfo);
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);

private:
  bool ReadFromResource(MediaResource *aResource, PRUint8 *aBuf, PRUint32 aLength);

  nsRawVideoHeader mMetadata;
  PRUint32 mCurrentFrame;
  double mFrameRate;
  PRUint32 mFrameSize;
  nsIntRect mPicture;
};

#endif
