






































#ifndef mozilla_layers_ShadowLayersManager_h
#define mozilla_layers_ShadowLayersManager_h

namespace mozilla {

namespace layout {
class RenderFrameParent;
}

namespace layers {

class CompositorParent;

class ShadowLayersManager
{

public:
  virtual void ShadowLayersUpdated() = 0;
};

} 
} 

#endif 
