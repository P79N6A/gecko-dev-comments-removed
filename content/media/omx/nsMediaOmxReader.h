




#if !defined(nsMediaOmxReader_h_)
#define nsMediaOmxReader_h_

#include "base/basictypes.h"
#include "MediaResource.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"
#include "OmxDecoder.h"

#include "MPAPI.h"

class nsMediaOmxReader : public nsBuiltinDecoderReader
{
  nsCString mType;
  android::OmxDecoder *mOmxDecoder;
  bool mHasVideo;
  bool mHasAudio;
  nsIntRect mPicture;
  nsIntSize mInitialFrame;
  int64_t mVideoSeekTimeUs;
  int64_t mAudioSeekTimeUs;
  VideoData *mLastVideoFrame;
public:
  nsMediaOmxReader(nsBuiltinDecoder* aDecoder);
  ~nsMediaOmxReader();

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
