





#ifndef __FFmpegDataDecoder_h__
#define __FFmpegDataDecoder_h__

#include "PlatformDecoderModule.h"
#include "FFmpegLibs.h"
#include "mozilla/Vector.h"
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
  FFmpegDataDecoder(MediaTaskQueue* aTaskQueue, AVCodecID aCodecID);
  virtual ~FFmpegDataDecoder();

  static bool Link();

  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) = 0;
  virtual nsresult Flush() MOZ_OVERRIDE;
  virtual nsresult Drain() = 0;
  virtual nsresult Shutdown() MOZ_OVERRIDE;

protected:
  AVFrame*        PrepareFrame();

  MediaTaskQueue* mTaskQueue;
  AVCodecContext* mCodecContext;
  AVFrame*        mFrame;
  Vector<uint8_t> mExtraData;

private:
  static bool sFFmpegInitDone;
  static StaticMutex sMonitor;

  AVCodecID mCodecID;
};

} 

#endif 
