





#ifndef __FFmpegAACDecoder_h__
#define __FFmpegAACDecoder_h__

#include "FFmpegDataDecoder.h"

namespace mozilla
{

template <int V> class FFmpegAudioDecoder
{
};

template <>
class FFmpegAudioDecoder<LIBAV_VER> : public FFmpegDataDecoder<LIBAV_VER>
{
public:
  FFmpegAudioDecoder(FlushableMediaTaskQueue* aTaskQueue,
                     MediaDataDecoderCallback* aCallback,
                     const mp4_demuxer::AudioDecoderConfig& aConfig);
  virtual ~FFmpegAudioDecoder();

  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override;
  virtual nsresult Drain() override;
  static AVCodecID GetCodecId(const nsACString& aMimeType);

private:
  void DecodePacket(MediaRawData* aSample);

  MediaDataDecoderCallback* mCallback;
};

} 

#endif 
