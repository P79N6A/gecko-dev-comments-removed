





#ifndef __FFmpegCompat_h__
#define __FFmpegCompat_h__

#include <libavcodec/version.h>

#if LIBAVCODEC_VERSION_MAJOR < 55
#define AV_CODEC_ID_H264 CODEC_ID_H264
#define AV_CODEC_ID_AAC CODEC_ID_AAC
typedef CodecID AVCodecID;
#endif

#endif 
