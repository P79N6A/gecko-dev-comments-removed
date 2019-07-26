




#if !defined(MediaPluginReader_h_)
#define MediaPluginReader_h_

#include "MediaResource.h"
#include "MediaDecoder.h"
#include "MediaDecoderReader.h"

#include "MPAPI.h"

namespace mozilla {

class MediaPluginReader : public MediaDecoderReader
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
  MediaPluginReader(MediaDecoder* aDecoder);
  ~MediaPluginReader();

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
