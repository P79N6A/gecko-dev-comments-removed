





#ifndef __FFmpegH264Decoder_h__
#define __FFmpegH264Decoder_h__

#include "FFmpegDataDecoder.h"

namespace mozilla
{

template <int V>
class FFmpegH264Decoder : public FFmpegDataDecoder<V>
{
};

template <>
class FFmpegH264Decoder<LIBAV_VER> : public FFmpegDataDecoder<LIBAV_VER>
{
  typedef mozilla::layers::Image Image;
  typedef mozilla::layers::ImageContainer ImageContainer;

  enum DecodeResult {
    DECODE_FRAME,
    DECODE_NO_FRAME,
    DECODE_ERROR
  };

public:
  FFmpegH264Decoder(MediaTaskQueue* aTaskQueue,
                    MediaDataDecoderCallback* aCallback,
                    const mp4_demuxer::VideoDecoderConfig& aConfig,
                    ImageContainer* aImageContainer);
  virtual ~FFmpegH264Decoder();

  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;

private:
  void DecodeFrame(mp4_demuxer::MP4Sample* aSample);
  DecodeResult DoDecodeFrame(mp4_demuxer::MP4Sample* aSample);
  void DoDrain();
  void OutputDelayedFrames();

  





  int AllocateYUV420PVideoBuffer(AVCodecContext* aCodecContext,
                                 AVFrame* aFrame);

  static int AllocateBufferCb(AVCodecContext* aCodecContext, AVFrame* aFrame);
  static void ReleaseBufferCb(AVCodecContext* aCodecContext, AVFrame* aFrame);

  MediaDataDecoderCallback* mCallback;
  nsRefPtr<ImageContainer> mImageContainer;
};

} 

#endif 
