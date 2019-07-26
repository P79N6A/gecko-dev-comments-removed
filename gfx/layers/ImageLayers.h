




#ifndef GFX_IMAGELAYER_H
#define GFX_IMAGELAYER_H

#include "Layers.h"                     
#include "GraphicsFilter.h"             
#include "gfxPoint.h"                   
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/layers/LayersTypes.h"
#include "nsAutoPtr.h"                  
#include "nscore.h"                     

class gfx3DMatrix;

namespace mozilla {
namespace layers {

class ImageContainer;




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

  



  void SetScaleToSize(const gfxIntSize &aSize, ScaleMode aMode)
  {
    if (mScaleToSize != aSize || mScaleMode != aMode) {
      mScaleToSize = aSize;
      mScaleMode = aMode;
      Mutated();
    }
  }


  ImageContainer* GetContainer() { return mContainer; }
  GraphicsFilter GetFilter() { return mFilter; }
  const gfxIntSize& GetScaleToSize() { return mScaleToSize; }
  ScaleMode GetScaleMode() { return mScaleMode; }

  MOZ_LAYER_DECL_NAME("ImageLayer", TYPE_IMAGE)

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface);

  


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
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);


  nsRefPtr<ImageContainer> mContainer;
  GraphicsFilter mFilter;
  gfxIntSize mScaleToSize;
  ScaleMode mScaleMode;
  bool mDisallowBigImage;
};

}
}

#endif 
