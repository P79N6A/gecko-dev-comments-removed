




#ifndef GFX_IMAGELAYER_H
#define GFX_IMAGELAYER_H

#include "Layers.h"

#include "ImageTypes.h"
#include "nsISupportsImpl.h"
#include "gfxPattern.h"

namespace mozilla {
namespace layers {

class ImageContainer;




class THEBES_API ImageLayer : public Layer {
public:
  enum ScaleMode {
    SCALE_NONE,
    SCALE_STRETCH
  
  };

  




  void SetContainer(ImageContainer* aContainer);

  



  void SetFilter(gfxPattern::GraphicsFilter aFilter) { mFilter = aFilter; }

  



  void SetScaleToSize(const gfxIntSize &aSize, ScaleMode aMode)
  {
    mScaleToSize = aSize;
    mScaleMode = aMode;
  }


  ImageContainer* GetContainer() { return mContainer; }
  gfxPattern::GraphicsFilter GetFilter() { return mFilter; }
  const gfxIntSize& GetScaleToSize() { return mScaleToSize; }
  ScaleMode GetScaleMode() { return mScaleMode; }

  MOZ_LAYER_DECL_NAME("ImageLayer", TYPE_IMAGE)

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface);

  


  void SetForceSingleTile(bool aForceSingleTile)
  {
    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ForceSingleTile", this));
    mForceSingleTile = aForceSingleTile;
    Mutated();
  }

protected:
  ImageLayer(LayerManager* aManager, void* aImplData);
  ~ImageLayer();
  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);


  nsRefPtr<ImageContainer> mContainer;
  gfxPattern::GraphicsFilter mFilter;
  gfxIntSize mScaleToSize;
  ScaleMode mScaleMode;
  bool mForceSingleTile;
};

}
}

#endif
