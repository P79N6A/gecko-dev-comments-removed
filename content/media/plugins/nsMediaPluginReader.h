




#if !defined(nsMediaPluginReader_h_)
#define nsMediaPluginReader_h_

#include "MediaResource.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"

#include "MPAPI.h"

class nsMediaPluginReader : public nsBuiltinDecoderReader
{
  nsCString mType;
  MPAPI::Decoder *mPlugin;
  PRBool mHasAudio;
  PRBool mHasVideo;
  nsIntRect mPicture;
  nsIntSize mInitialFrame;
  int64_t mVideoSeekTimeUs;
  int64_t mAudioSeekTimeUs;
  VideoData *mLastVideoFrame;
public:
  nsMediaPluginReader(nsBuiltinDecoder* aDecoder);
  ~nsMediaPluginReader();

  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();

  virtual bool DecodeAudioData();
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                PRInt64 aTimeThreshold);

  virtual bool HasAudio()
  {
    return mHasAudio;
  }

  virtual bool HasVideo()
  {
    return mHasVideo;
  }

  virtual nsresult ReadMetadata(nsVideoInfo* aInfo,
                                nsHTMLMediaElement::MetadataTags** aTags);
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);
  virtual bool IsSeekableInBufferedRanges() {
    return true;
  }

};

#endif
