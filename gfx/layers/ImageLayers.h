




#ifndef GFX_IMAGELAYER_H
#define GFX_IMAGELAYER_H

#include "Layers.h"                     
#include "GraphicsFilter.h"             
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/LayersTypes.h"
#include "nsAutoPtr.h"                  
#include "nscore.h"                     

namespace mozilla {
namespace layers {

class ImageContainer;

namespace layerscope {
class LayersPacket;
} 




class ImageLayer : public Layer {
public:
  




  virtual void SetContainer(ImageContainer* aContainer);

  



  void SetFilter(GraphicsFilter aFilter)
  {
    if (mFilter != aFilter) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) Filter", this));
      mFilter = aFilter;
      Mutated();
    }
  }

  



  void SetScaleToSize(const gfx::IntSize &aSize, ScaleMode aMode)
  {
    if (mScaleToSize != aSize || mScaleMode != aMode) {
      mScaleToSize = aSize;
      mScaleMode = aMode;
      Mutated();
    }
  }


  ImageContainer* GetContainer() { return mContainer; }
  GraphicsFilter GetFilter() { return mFilter; }
  const gfx::IntSize& GetScaleToSize() { return mScaleToSize; }
  ScaleMode GetScaleMode() { return mScaleMode; }

  MOZ_LAYER_DECL_NAME("ImageLayer", TYPE_IMAGE)

  virtual void ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface) override;

  virtual const gfx::Matrix4x4& GetEffectiveTransformForBuffer() const override
  {
    return mEffectiveTransformForBuffer;
  }

  


  void SetDisallowBigImage(bool aDisallowBigImage)
  {
    if (mDisallowBigImage != aDisallowBigImage) {
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) DisallowBigImage", this));
      mDisallowBigImage = aDisallowBigImage;
      Mutated();
    }
  }

protected:
  ImageLayer(LayerManager* aManager, void* aImplData);
  ~ImageLayer();
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;
  virtual void DumpPacket(layerscope::LayersPacket* aPacket, const void* aParent) override;

  nsRefPtr<ImageContainer> mContainer;
  GraphicsFilter mFilter;
  gfx::IntSize mScaleToSize;
  ScaleMode mScaleMode;
  bool mDisallowBigImage;
  gfx::Matrix4x4 mEffectiveTransformForBuffer;
};

} 
} 

#endif 
