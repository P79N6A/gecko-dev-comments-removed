





#include "mozilla/TaskQueue.h"

#include <string.h>
#include <unistd.h>

#include "FFmpegLibs.h"
#include "FFmpegLog.h"
#include "FFmpegDataDecoder.h"
#include "prsystem.h"

namespace mozilla
{

bool FFmpegDataDecoder<LIBAV_VER>::sFFmpegInitDone = false;
StaticMutex FFmpegDataDecoder<LIBAV_VER>::sMonitor;

FFmpegDataDecoder<LIBAV_VER>::FFmpegDataDecoder(FlushableTaskQueue* aTaskQueue,
                                                AVCodecID aCodecID)
  : mTaskQueue(aTaskQueue)
  , mCodecContext(nullptr)
  , mFrame(NULL)
  , mExtraData(nullptr)
  , mCodecID(aCodecID)
{
  MOZ_COUNT_CTOR(FFmpegDataDecoder);
}

FFmpegDataDecoder<LIBAV_VER>::~FFmpegDataDecoder()
{
  MOZ_COUNT_DTOR(FFmpegDataDecoder);
}







static PixelFormat
ChoosePixelFormat(AVCodecContext* aCodecContext, const PixelFormat* aFormats)
{
  FFMPEG_LOG("Choosing FFmpeg pixel format for video decoding.");
  for (; *aFormats > -1; aFormats++) {
    if (*aFormats == PIX_FMT_YUV420P) {
      FFMPEG_LOG("Requesting pixel format YUV420P.");
      return PIX_FMT_YUV420P;
    }
  }

  NS_WARNING("FFmpeg does not share any supported pixel formats.");
  return PIX_FMT_NONE;
}

nsresult
FFmpegDataDecoder<LIBAV_VER>::Init()
{
  StaticMutexAutoLock mon(sMonitor);

  FFMPEG_LOG("Initialising FFmpeg decoder.");

  if (!sFFmpegInitDone) {
    av_register_all();
#ifdef DEBUG
    av_log_set_level(AV_LOG_DEBUG);
#endif
    sFFmpegInitDone = true;
  }

  AVCodec* codec = avcodec_find_decoder(mCodecID);
  if (!codec) {
    NS_WARNING("Couldn't find ffmpeg decoder");
    return NS_ERROR_FAILURE;
  }

  if (!(mCodecContext = avcodec_alloc_context3(codec))) {
    NS_WARNING("Couldn't init ffmpeg context");
    return NS_ERROR_FAILURE;
  }

  mCodecContext->opaque = this;

  
  mCodecContext->request_sample_fmt = AV_SAMPLE_FMT_FLT;

  
  mCodecContext->get_format = ChoosePixelFormat;

  mCodecContext->thread_count = PR_GetNumberOfProcessors();
  mCodecContext->thread_type = FF_THREAD_SLICE | FF_THREAD_FRAME;
  mCodecContext->thread_safe_callbacks = false;

  if (mExtraData) {
    mCodecContext->extradata_size = mExtraData->Length();
    
    
    mExtraData->AppendElements(FF_INPUT_BUFFER_PADDING_SIZE);
    mCodecContext->extradata = mExtraData->Elements();
  } else {
    mCodecContext->extradata_size = 0;
  }

  if (codec->capabilities & CODEC_CAP_DR1) {
    mCodecContext->flags |= CODEC_FLAG_EMU_EDGE;
  }

  if (avcodec_open2(mCodecContext, codec, nullptr) < 0) {
    NS_WARNING("Couldn't initialise ffmpeg decoder");
    return NS_ERROR_FAILURE;
  }

  if (mCodecContext->codec_type == AVMEDIA_TYPE_AUDIO &&
      mCodecContext->sample_fmt != AV_SAMPLE_FMT_FLT &&
      mCodecContext->sample_fmt != AV_SAMPLE_FMT_FLTP &&
      mCodecContext->sample_fmt != AV_SAMPLE_FMT_S16 &&
      mCodecContext->sample_fmt != AV_SAMPLE_FMT_S16P) {
    NS_WARNING("FFmpeg audio decoder outputs unsupported audio format.");
    return NS_ERROR_FAILURE;
  }

  FFMPEG_LOG("FFmpeg init successful.");
  return NS_OK;
}

nsresult
FFmpegDataDecoder<LIBAV_VER>::Flush()
{
  mTaskQueue->Flush();
  avcodec_flush_buffers(mCodecContext);
  return NS_OK;
}

nsresult
FFmpegDataDecoder<LIBAV_VER>::Shutdown()
{
  StaticMutexAutoLock mon(sMonitor);

  if (sFFmpegInitDone) {
    avcodec_close(mCodecContext);
    av_freep(&mCodecContext);
#if LIBAVCODEC_VERSION_MAJOR >= 55
    av_frame_free(&mFrame);
#elif LIBAVCODEC_VERSION_MAJOR == 54
    avcodec_free_frame(&mFrame);
#else
    delete mFrame;
    mFrame = nullptr;
#endif
  }
  return NS_OK;
}

AVFrame*
FFmpegDataDecoder<LIBAV_VER>::PrepareFrame()
{
#if LIBAVCODEC_VERSION_MAJOR >= 55
  if (mFrame) {
    av_frame_unref(mFrame);
  } else {
    mFrame = av_frame_alloc();
  }
#elif LIBAVCODEC_VERSION_MAJOR == 54
  if (mFrame) {
    avcodec_get_frame_defaults(mFrame);
  } else {
    mFrame = avcodec_alloc_frame();
  }
#else
  delete mFrame;
  mFrame = new AVFrame;
  avcodec_get_frame_defaults(mFrame);
#endif
  return mFrame;
}

} 
