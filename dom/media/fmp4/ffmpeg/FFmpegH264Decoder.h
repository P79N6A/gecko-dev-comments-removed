





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
  FFmpegH264Decoder(FlushableMediaTaskQueue* aTaskQueue,
                    MediaDataDecoderCallback* aCallback,
                    const VideoInfo& aConfig,
                    ImageContainer* aImageContainer);
  virtual ~FFmpegH264Decoder();

  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override;
  virtual nsresult Drain() override;
  virtual nsresult Flush() override;
  static AVCodecID GetCodecId(const nsACString& aMimeType);

private:
  void DecodeFrame(MediaRawData* aSample);
  DecodeResult DoDecodeFrame(MediaRawData* aSample);
  void DoDrain();
  void OutputDelayedFrames();

  





  int AllocateYUV420PVideoBuffer(AVCodecContext* aCodecContext,
                                 AVFrame* aFrame);

  static int AllocateBufferCb(AVCodecContext* aCodecContext, AVFrame* aFrame);
  static void ReleaseBufferCb(AVCodecContext* aCodecContext, AVFrame* aFrame);

  MediaDataDecoderCallback* mCallback;
  nsRefPtr<ImageContainer> mImageContainer;
  uint32_t mDisplayWidth;
  uint32_t mDisplayHeight;
};

} 

#endif 
