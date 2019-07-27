





#ifndef __FFmpegDataDecoder_h__
#define __FFmpegDataDecoder_h__

#include "PlatformDecoderModule.h"
#include "FFmpegLibs.h"
#include "mozilla/StaticMutex.h"

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
  FFmpegDataDecoder(FlushableTaskQueue* aTaskQueue, AVCodecID aCodecID);
  virtual ~FFmpegDataDecoder();

  static bool Link();

  virtual nsresult Init() override;
  virtual nsresult Input(MediaRawData* aSample) override = 0;
  virtual nsresult Flush() override;
  virtual nsresult Drain() override = 0;
  virtual nsresult Shutdown() override;

protected:
  AVFrame*        PrepareFrame();

  FlushableTaskQueue* mTaskQueue;
  AVCodecContext* mCodecContext;
  AVFrame*        mFrame;
  nsRefPtr<MediaByteBuffer> mExtraData;

private:
  static bool sFFmpegInitDone;
  static StaticMutex sMonitor;

  AVCodecID mCodecID;
};

} 

#endif 
