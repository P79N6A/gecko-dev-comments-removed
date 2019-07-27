





#ifndef __FFmpegRuntimeLinker_h__
#define __FFmpegRuntimeLinker_h__

#include <stdint.h>

namespace mozilla
{

class PlatformDecoderModule;
struct AvFormatLib;

class FFmpegRuntimeLinker
{
public:
  static bool Link();
  static void Unlink();
  static PlatformDecoderModule* CreateDecoderModule();

private:
  static void* sLinkedLib;
  static const AvFormatLib* sLib;

  static bool Bind(const char* aLibName, uint32_t Version);

  static enum LinkStatus {
    LinkStatus_INIT = 0,
    LinkStatus_FAILED,
    LinkStatus_SUCCEEDED
  } sLinkStatus;
};

}

#endif 
