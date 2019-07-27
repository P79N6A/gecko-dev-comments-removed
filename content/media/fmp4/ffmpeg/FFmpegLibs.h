





#ifndef __FFmpegLibs_h__
#define __FFmpegLibs_h__

extern "C" {
#pragma GCC visibility push(default)
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#pragma GCC visibility pop
}

#if LIBAVCODEC_VERSION_MAJOR < 55
#define AV_CODEC_ID_H264 CODEC_ID_H264
#define AV_CODEC_ID_AAC CODEC_ID_AAC
#define AV_CODEC_ID_MP3 CODEC_ID_MP3
#define AV_CODEC_ID_NONE CODEC_ID_NONE
typedef CodecID AVCodecID;
#endif

enum { LIBAV_VER = LIBAVFORMAT_VERSION_MAJOR };

namespace mozilla {

#define AV_FUNC(func, ver) extern typeof(func)* func;
#include "FFmpegFunctionList.h"
#undef AV_FUNC

}

#endif 
