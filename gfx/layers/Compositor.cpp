




#include "mozilla/layers/Compositor.h"

namespace mozilla {
namespace layers {

 LayersBackend Compositor::sBackend = LAYERS_NONE;
 LayersBackend
Compositor::GetBackend()
{
  return sBackend;
}

} 
} 
