





#ifndef __FFmpegLog_h__
#define __FFmpegLog_h__

extern PRLogModuleInfo* GetFFmpegDecoderLog();
#define FFMPEG_LOG(...) PR_LOG(GetFFmpegDecoderLog(), PR_LOG_DEBUG, (__VA_ARGS__))

#endif 
