





#include "PlatformDecoderModule.h"
#ifdef XP_WIN
#include "WMFDecoderModule.h"
#endif
#include "mozilla/Preferences.h"

namespace mozilla {

extern PlatformDecoderModule* CreateBlankDecoderModule();


PlatformDecoderModule*
PlatformDecoderModule::Create()
{
  if (Preferences::GetBool("media.fragmented-mp4.use-blank-decoder")) {
    return CreateBlankDecoderModule();
  }
#ifdef XP_WIN
  nsAutoPtr<WMFDecoderModule> m(new WMFDecoderModule());
  if (NS_SUCCEEDED(m->Init())) {
    return m.forget();
  }
#endif
  return nullptr;
}

void
PlatformDecoderModule::OnDecodeThreadStart()
{
}

void
PlatformDecoderModule::OnDecodeThreadFinish()
{
}

} 
