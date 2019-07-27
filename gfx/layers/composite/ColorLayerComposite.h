




#ifndef GFX_ColorLayerComposite_H
#define GFX_ColorLayerComposite_H

#include "Layers.h"                     
#include "mozilla/Attributes.h"         
#include "mozilla/layers/LayerManagerComposite.h"  
#include "nsISupportsImpl.h"            

struct nsIntPoint;
struct nsIntRect;

namespace mozilla {
namespace layers {

class CompositableHost;

class ColorLayerComposite : public ColorLayer,
                            public LayerComposite
{
public:
  explicit ColorLayerComposite(LayerManagerComposite *aManager)
    : ColorLayer(aManager, nullptr)
    , LayerComposite(aManager)
  {
    MOZ_COUNT_CTOR(ColorLayerComposite);
    mImplData = static_cast<LayerComposite*>(this);
  }

protected:
  ~ColorLayerComposite()
  {
    MOZ_COUNT_DTOR(ColorLayerComposite);
    Destroy();
  }

public:
  
  virtual Layer* GetLayer() MOZ_OVERRIDE { return this; }

  virtual void Destroy() MOZ_OVERRIDE { mDestroyed = true; }

  virtual void RenderLayer(const nsIntRect& aClipRect) MOZ_OVERRIDE;
  virtual void CleanupResources() MOZ_OVERRIDE {};

  virtual void GenEffectChain(EffectChain& aEffect) MOZ_OVERRIDE;

  CompositableHost* GetCompositableHost() MOZ_OVERRIDE { return nullptr; }

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  virtual const char* Name() const MOZ_OVERRIDE { return "ColorLayerComposite"; }
};

} 
} 
#endif 
