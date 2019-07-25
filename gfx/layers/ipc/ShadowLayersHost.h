






































#ifndef mozilla_layers_ShadowLayersHost_h
#define mozilla_layers_ShadowLayersHost_h

namespace mozilla {

namespace layout {
class RenderFrameParent;
}

namespace layers {

class CompositorParent;

class ShadowLayersHost
{
NS_INLINE_DECL_REFCOUNTING(ShadowLayersHost)

public:
  virtual mozilla::layout::RenderFrameParent* GetRenderFrameParent() = 0;
  virtual CompositorParent* GetCompositorParent() = 0;
};

} 
} 

#endif 
