




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
  
  virtual void Initialize(const Data& aData) override
  {
    NS_RUNTIMEABORT("Incompatibe surface type");
  }

  virtual LayerRenderState GetRenderState() override;

  virtual bool SetCompositableHost(CompositableHost* aHost) override;

  virtual void Disconnect() override
  {
    Destroy();
  }

  virtual void SetLayerManager(LayerManagerComposite* aManager) override;

  virtual Layer* GetLayer() override;
  virtual void RenderLayer(const nsIntRect& aClipRect) override;

  virtual void CleanupResources() override;

  virtual void GenEffectChain(EffectChain& aEffect) override;

  CompositableHost* GetCompositableHost() override;

  virtual LayerComposite* AsLayerComposite() override { return this; }

  void SetBounds(nsIntRect aBounds) { mBounds = aBounds; }

  virtual const char* Name() const override { return "CanvasLayerComposite"; }

protected:
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;

private:
  gfx::Filter GetEffectFilter();

private:
  RefPtr<CompositableHost> mImageHost;
};

} 
} 
#endif 
