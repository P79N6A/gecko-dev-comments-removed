






#include "Hal.h"

namespace mozilla {
namespace hal_impl {

bool
SetLight(hal::LightType light, const hal::LightConfiguration& aConfig)
{
  return true;
}

bool
GetLight(hal::LightType light, hal::LightConfiguration* aConfig)
{
  aConfig->light() = light;
  
  return true;
}

} 
} 
