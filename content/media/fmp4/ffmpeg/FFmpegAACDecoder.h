





#ifndef __FFmpegAACDecoder_h__
#define __FFmpegAACDecoder_h__

#include "FFmpegDataDecoder.h"

namespace mozilla
{

class FFmpegAACDecoder : public FFmpegDataDecoder
{
public:
  FFmpegAACDecoder(MediaTaskQueue* aTaskQueue,
                   MediaDataDecoderCallback* aCallback,
                   const mp4_demuxer::AudioDecoderConfig &aConfig);
  virtual ~FFmpegAACDecoder();

  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;

private:
  void DecodePacket(mp4_demuxer::MP4Sample* aSample);

  MediaDataDecoderCallback* mCallback;
  mp4_demuxer::AudioDecoderConfig mConfig;
};

} 

#endif 
