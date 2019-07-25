






































#ifndef mozilla_layers_Compositor_h
#define mozilla_layers_Compositor_h



#include "Layers.h"
#include "nsDebug.h"

namespace mozilla {
namespace layers {
namespace compositor {




extern int sShadowNativeContext;

class ShadowNativeContextUserData : public LayerUserData
{
public:
  ShadowNativeContextUserData(void *aNativeContext)
    : mNativeContext(aNativeContext)
  {
    MOZ_COUNT_CTOR(ShadowNativeContextUserData);
  }
  ~ShadowNativeContextUserData()
  {
    MOZ_COUNT_DTOR(ShadowNativeContextUserData);
  }

  
  
  
  
  void* GetNativeContext() {
    return mNativeContext;
  }
private:
  void *mNativeContext;
};


}
}
}
#endif
