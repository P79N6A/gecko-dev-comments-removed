





#include "PlatformDecoderModule.h"

#ifdef XP_WIN
#include "WMFDecoderModule.h"
#endif

namespace mozilla {


PlatformDecoderModule*
PlatformDecoderModule::Create()
{
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
