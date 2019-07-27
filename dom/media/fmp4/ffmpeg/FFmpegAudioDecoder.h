





#ifndef __FFmpegAACDecoder_h__
#define __FFmpegAACDecoder_h__

#include "FFmpegDataDecoder.h"
#include "mp4_demuxer/DecoderData.h"

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
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) override;
  virtual nsresult Drain() override;
  static AVCodecID GetCodecId(const char* aMimeType);

private:
  void DecodePacket(mp4_demuxer::MP4Sample* aSample);

  MediaDataDecoderCallback* mCallback;
};

} 

#endif 
