




#ifndef GFX_ThebesLayerComposite_H
#define GFX_ThebesLayerComposite_H

#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"

#include "Layers.h"
#include "LayerManagerComposite.h"
#include "base/task.h"


namespace mozilla {
namespace layers {







class ContentHost;

class ThebesLayerComposite : public ShadowThebesLayer,
                             public LayerComposite
{
public:
  ThebesLayerComposite(LayerManagerComposite *aManager);
  virtual ~ThebesLayerComposite();

  virtual void Disconnect() MOZ_OVERRIDE;

  virtual void SetValidRegion(const nsIntRegion& aRegion) MOZ_OVERRIDE
  {
    ShadowThebesLayer::SetValidRegion(aRegion);
  }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  CompositableHost* GetCompositableHost() MOZ_OVERRIDE;

  virtual void Destroy() MOZ_OVERRIDE;

  virtual Layer* GetLayer() MOZ_OVERRIDE;

  virtual TiledLayerComposer* AsTiledLayerComposer() MOZ_OVERRIDE;

  virtual void RenderLayer(const nsIntPoint& aOffset,
                           const nsIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void CleanupResources() MOZ_OVERRIDE;

  virtual void SetCompositableHost(CompositableHost* aHost) MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  virtual void EnsureBuffer(CompositableType aType) MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() const MOZ_OVERRIDE { return "ThebesLayerComposite"; }

protected:
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix) MOZ_OVERRIDE;
#endif

private:
  gfxRect GetDisplayPort();
  gfxSize GetEffectiveResolution();
  gfxRect GetCompositionBounds();

  RefPtr<ContentHost> mBuffer;
  bool mRequiresTiledProperties;
};

} 
} 
#endif 
