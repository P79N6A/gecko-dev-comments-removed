




#ifndef GFX_PaintedLayerComposite_H
#define GFX_PaintedLayerComposite_H

#include "Layers.h"                     
#include "gfxRect.h"                    
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/LayerManagerComposite.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "nsDebug.h"                    
#include "nsRegion.h"                   
#include "nscore.h"                     

struct nsIntPoint;
struct nsIntRect;


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
  virtual void Disconnect() MOZ_OVERRIDE;

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  CompositableHost* GetCompositableHost() MOZ_OVERRIDE;

  virtual void Destroy() MOZ_OVERRIDE;

  virtual Layer* GetLayer() MOZ_OVERRIDE;

  virtual void SetLayerManager(LayerManagerComposite* aManager) MOZ_OVERRIDE;

  virtual TiledLayerComposer* GetTiledLayerComposer() MOZ_OVERRIDE;

  virtual void RenderLayer(const nsIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void CleanupResources() MOZ_OVERRIDE;

  virtual void GenEffectChain(EffectChain& aEffect) MOZ_OVERRIDE;

  virtual bool SetCompositableHost(CompositableHost* aHost) MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  virtual void InvalidateRegion(const nsIntRegion& aRegion) MOZ_OVERRIDE
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

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) MOZ_OVERRIDE;

private:
  gfx::Filter GetEffectFilter() { return gfx::Filter::LINEAR; }

private:
  RefPtr<ContentHost> mBuffer;
};

} 
} 
#endif 
