





#ifndef __FFmpegH264Decoder_h__
#define __FFmpegH264Decoder_h__

#include "nsTPriorityQueue.h"

#include "FFmpegDataDecoder.h"

namespace mozilla
{

class FFmpegH264Decoder : public FFmpegDataDecoder
{
  typedef mozilla::layers::Image Image;
  typedef mozilla::layers::ImageContainer ImageContainer;

public:
  FFmpegH264Decoder(MediaTaskQueue* aTaskQueue,
                    MediaDataDecoderCallback* aCallback,
                    const mp4_demuxer::VideoDecoderConfig &aConfig,
                    ImageContainer* aImageContainer);
  virtual ~FFmpegH264Decoder();

  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;

private:
  void DecodeFrame(mp4_demuxer::MP4Sample* aSample);
  void OutputDelayedFrames();

  





  int AllocateYUV420PVideoBuffer(AVCodecContext* aCodecContext,
                                 AVFrame* aFrame);

  static int AllocateBufferCb(AVCodecContext* aCodecContext, AVFrame* aFrame);

  mp4_demuxer::VideoDecoderConfig mConfig;
  MediaDataDecoderCallback* mCallback;
  nsRefPtr<ImageContainer> mImageContainer;

  










  nsRefPtr<Image> mCurrentImage;

  struct VideoDataComparator
  {
    bool LessThan(VideoData* const &a, VideoData* const &b) const
    {
      return a->mTime < b->mTime;
    }
  };

  




  nsTPriorityQueue<VideoData*, VideoDataComparator> mDelayedFrames;
};

} 

#endif 
