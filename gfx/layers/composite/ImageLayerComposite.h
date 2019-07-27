




#ifndef GFX_ImageLayerComposite_H
#define GFX_ImageLayerComposite_H

#include "GLTextureImage.h"             
#include "ImageLayers.h"                
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/LayerManagerComposite.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "nsISupportsImpl.h"            
#include "nscore.h"                     

struct nsIntPoint;
struct nsIntRect;

namespace mozilla {
namespace layers {

class CompositableHost;
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
  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  virtual void Disconnect() MOZ_OVERRIDE;

  virtual bool SetCompositableHost(CompositableHost* aHost) MOZ_OVERRIDE;

  virtual Layer* GetLayer() MOZ_OVERRIDE;

  virtual void RenderLayer(const nsIntRect& aClipRect);

  virtual void ComputeEffectiveTransforms(const mozilla::gfx::Matrix4x4& aTransformToSurface) MOZ_OVERRIDE;

  virtual void CleanupResources() MOZ_OVERRIDE;

  CompositableHost* GetCompositableHost() MOZ_OVERRIDE;

  virtual void GenEffectChain(EffectChain& aEffect) MOZ_OVERRIDE;

  virtual LayerComposite* AsLayerComposite() MOZ_OVERRIDE { return this; }

  virtual const char* Name() const { return "ImageLayerComposite"; }

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
