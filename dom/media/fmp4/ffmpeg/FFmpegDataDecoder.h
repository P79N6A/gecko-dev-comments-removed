





#ifndef __FFmpegDataDecoder_h__
#define __FFmpegDataDecoder_h__

#include "PlatformDecoderModule.h"
#include "FFmpegLibs.h"
#include "mozilla/StaticMutex.h"
#include "mp4_demuxer/mp4_demuxer.h"

namespace mozilla
{

template <int V>
class FFmpegDataDecoder : public MediaDataDecoder
{
};

template <>
class FFmpegDataDecoder<LIBAV_VER> : public MediaDataDecoder
{
public:
  FFmpegDataDecoder(FlushableMediaTaskQueue* aTaskQueue, AVCodecID aCodecID);
  virtual ~FFmpegDataDecoder();

  static bool Link();

  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override = 0;
  virtual nsresult Flush() override;
  virtual nsresult Drain() override = 0;
  virtual nsresult Shutdown() override;

protected:
  AVFrame*        PrepareFrame();

  FlushableMediaTaskQueue* mTaskQueue;
  AVCodecContext* mCodecContext;
  AVFrame*        mFrame;
  nsRefPtr<DataBuffer> mExtraData;

private:
  static bool sFFmpegInitDone;
  static StaticMutex sMonitor;

  AVCodecID mCodecID;
};

} 

#endif 
