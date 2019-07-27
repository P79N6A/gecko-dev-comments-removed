




#ifndef GFX_ColorLayerComposite_H
#define GFX_ColorLayerComposite_H

#include "Layers.h"                     
#include "mozilla/Attributes.h"         
#include "mozilla/layers/LayerManagerComposite.h"  
#include "nsISupportsImpl.h"            

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
  
  virtual Layer* GetLayer() override { return this; }

  virtual void SetLayerManager(LayerManagerComposite* aManager) override
  {
    LayerComposite::SetLayerManager(aManager);
    mManager = aManager;
  }

  virtual void Destroy() override { mDestroyed = true; }

  virtual void RenderLayer(const gfx::IntRect& aClipRect) override;
  virtual void CleanupResources() override {};

  virtual void GenEffectChain(EffectChain& aEffect) override;

  CompositableHost* GetCompositableHost() override { return nullptr; }

  virtual LayerComposite* AsLayerComposite() override { return this; }

  virtual const char* Name() const override { return "ColorLayerComposite"; }
};

} 
} 
#endif 
