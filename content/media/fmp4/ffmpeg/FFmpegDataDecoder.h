





#ifndef __FFmpegDataDecoder_h__
#define __FFmpegDataDecoder_h__

#include "FFmpegDecoderModule.h"
#include "FFmpegRuntimeLinker.h"
#include "FFmpegCompat.h"

namespace mozilla
{

class FFmpegDataDecoder : public MediaDataDecoder
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
  nsAutoArrayPtr<uint8_t> mExtraData;
  size_t mExtraDataSize;
  MediaTaskQueue* mTaskQueue;
  AVCodecContext mCodecContext;

private:
  static bool sFFmpegInitDone;

  AVCodecID mCodecID;
};

} 

#endif 
