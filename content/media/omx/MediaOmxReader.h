




#if !defined(MediaOmxReader_h_)
#define MediaOmxReader_h_

#include "base/basictypes.h"
#include "MediaResource.h"
#include "MediaDecoder.h"
#include "MediaDecoderReader.h"
#include "OmxDecoder.h"

#include "MPAPI.h"

namespace mozilla {

class MediaOmxReader : public MediaDecoderReader
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
  MediaOmxReader(MediaDecoder* aDecoder);
  ~MediaOmxReader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor);
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
                                MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, int64_t aStartTime);
  virtual bool IsSeekableInBufferedRanges() {
    return true;
  }

};

} 

#endif
