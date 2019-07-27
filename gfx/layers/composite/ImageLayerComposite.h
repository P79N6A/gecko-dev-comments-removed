




#ifndef GFX_ImageLayerComposite_H
#define GFX_ImageLayerComposite_H

#include "GLTextureImage.h"             
#include "ImageLayers.h"                
#include "mozilla/Attributes.h"         
#include "mozilla/gfx/Rect.h"
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/LayerManagerComposite.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "nsISupportsImpl.h"            
#include "nscore.h"                     
#include "CompositableHost.h"           

namespace mozilla {
namespace layers {

class ImageHost;
class Layer;

class ImageLayerComposite : public ImageLayer,
                            public LayerComposite
{
  typedef gl::TextureImage TextureImage;

public:
  explicit ImageLayerComposite(LayerManagerComposite* aManager);

protected:
  virtual ~ImageLayerComposite();

public:
  virtual LayerRenderState GetRenderState() override;

  virtual void Disconnect() override;

  virtual bool SetCompositableHost(CompositableHost* aHost) override;

  virtual Layer* GetLayer() override;

  virtual void SetLayerManager(LayerManagerComposite* aManager) override
  {
    LayerComposite::SetLayerManager(aManager);
    mManager = aManager;
    if (mImageHost) {
      mImageHost->SetCompositor(mCompositor);
    }
  }

  virtual void RenderLayer(const gfx::IntRect& aClipRect) override;

  virtual void ComputeEffectiveTransforms(const mozilla::gfx::Matrix4x4& aTransformToSurface) override;

  virtual void CleanupResources() override;

  CompositableHost* GetCompositableHost() override;

  virtual void GenEffectChain(EffectChain& aEffect) override;

  virtual LayerComposite* AsLayerComposite() override { return this; }

  virtual const char* Name() const override { return "ImageLayerComposite"; }

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
