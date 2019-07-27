





#ifndef __FFmpegAACDecoder_h__
#define __FFmpegAACDecoder_h__

#include "FFmpegDataDecoder.h"
#include "mp4_demuxer/DecoderData.h"

namespace mozilla
{

template <int V> class FFmpegAACDecoder
{
};

template <>
class FFmpegAACDecoder<LIBAV_VER> : public FFmpegDataDecoder<LIBAV_VER>
{
public:
  FFmpegAACDecoder(MediaTaskQueue* aTaskQueue,
                   MediaDataDecoderCallback* aCallback,
                   const mp4_demuxer::AudioDecoderConfig& aConfig);
  virtual ~FFmpegAACDecoder();

  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;

private:
  void DecodePacket(mp4_demuxer::MP4Sample* aSample);

  MediaDataDecoderCallback* mCallback;
};

} 

#endif 
