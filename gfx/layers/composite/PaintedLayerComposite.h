




#ifndef GFX_PaintedLayerComposite_H
#define GFX_PaintedLayerComposite_H

#include "Layers.h"                     
#include "mozilla/gfx/Rect.h"
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/LayerManagerComposite.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "nsDebug.h"                    
#include "nsRegion.h"                   
#include "nscore.h"                     


namespace mozilla {
namespace layers {







class CompositableHost;
class ContentHost;
class TiledLayerComposer;

class PaintedLayerComposite : public PaintedLayer,
                              public LayerComposite
{
public:
  explicit PaintedLayerComposite(LayerManagerComposite *aManager);

protected:
  virtual ~PaintedLayerComposite();

public:
  virtual void Disconnect() override;

  virtual LayerRenderState GetRenderState() override;

  CompositableHost* GetCompositableHost() override;

  virtual void Destroy() override;

  virtual Layer* GetLayer() override;

  virtual void SetLayerManager(LayerManagerComposite* aManager) override;

  virtual TiledLayerComposer* GetTiledLayerComposer() override;

  virtual void RenderLayer(const gfx::IntRect& aClipRect) override;

  virtual void CleanupResources() override;

  virtual void GenEffectChain(EffectChain& aEffect) override;

  virtual bool SetCompositableHost(CompositableHost* aHost) override;

  virtual LayerComposite* AsLayerComposite() override { return this; }

  virtual void InvalidateRegion(const nsIntRegion& aRegion) override
  {
    NS_RUNTIMEABORT("PaintedLayerComposites can't fill invalidated regions");
  }

  void SetValidRegion(const nsIntRegion& aRegion)
  {
    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ValidRegion", this));
    mValidRegion = aRegion;
    Mutated();
  }

  MOZ_LAYER_DECL_NAME("PaintedLayerComposite", TYPE_PAINTED)

protected:

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;

private:
  gfx::Filter GetEffectFilter() { return gfx::Filter::LINEAR; }

private:
  RefPtr<ContentHost> mBuffer;
};

} 
} 
#endif 
