




#ifndef GFX_CanvasLayerComposite_H
#define GFX_CanvasLayerComposite_H

#include "Layers.h"                     
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/LayerManagerComposite.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "nsDebug.h"                    
#include "nsRect.h"                     
#include "nscore.h"                     
struct nsIntPoint;

namespace mozilla {
namespace layers {

class CompositableHost;


class ImageHost;

class CanvasLayerComposite : public CanvasLayer,
                             public LayerComposite
{
public:
  explicit CanvasLayerComposite(LayerManagerComposite* aManager);

protected:
  virtual ~CanvasLayerComposite();

public:
  
  virtual void Initialize(const Data& aData) MOZ_OVERRIDE
  {
    NS_RUNTIMEABORT("Incompatibe surface type");
  }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  virtual bool SetCompositableHost(CompositableHost* aHost) MOZ_OVERRIDE;

  virtual void Disconnect() MOZ_OVERRIDE
  {
    Destroy();
  }

  virtual Layer* GetLayer() MOZ_OVERRIDE;
  virtual void RenderLayer(const nsIntRect& aClipRect) MOZ_OVERRIDE;

  virtual void CleanupResources() MOZ_OVERRIDE;

  virtual void GenEffectChain(EffectChain& aEffect) MOZ_OVERRIDE;

  CompositableHost* GetCompositableHost() MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  void SetBounds(nsIntRect aBounds) { mBounds = aBounds; }

  virtual const char* Name() const MOZ_OVERRIDE { return "CanvasLayerComposite"; }

protected:
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) MOZ_OVERRIDE;

private:
  gfx::Filter GetEffectFilter();

private:
  RefPtr<CompositableHost> mImageHost;
};

} 
} 
#endif 
