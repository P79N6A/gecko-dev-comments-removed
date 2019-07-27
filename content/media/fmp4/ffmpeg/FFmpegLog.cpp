





#include "prlog.h"

PRLogModuleInfo* GetFFmpegDecoderLog()
{
  static PRLogModuleInfo* sFFmpegDecoderLog = nullptr;
  if (!sFFmpegDecoderLog) {
    sFFmpegDecoderLog = PR_NewLogModule("FFmpegDecoderModule");
  }
  return sFFmpegDecoderLog;
}

