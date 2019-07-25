




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
  bool mHasAudio;
  bool mHasVideo;
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
                                int64_t aTimeThreshold);

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
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, int64_t aStartTime);
  virtual bool IsSeekableInBufferedRanges() {
    return true;
  }

};

#endif
