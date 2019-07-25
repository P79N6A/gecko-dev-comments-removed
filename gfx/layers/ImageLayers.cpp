




#include "ImageLayers.h"
#include "ImageContainer.h"

namespace mozilla {
namespace layers {

ImageLayer::ImageLayer(LayerManager* aManager, void* aImplData)
: Layer(aManager, aImplData), mFilter(gfxPattern::FILTER_GOOD)
, mScaleMode(SCALE_NONE), mForceSingleTile(false) 
{}

ImageLayer::~ImageLayer()
{}

void ImageLayer::SetContainer(ImageContainer* aContainer) 
{
  mContainer = aContainer;
}

void ImageLayer::ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
{
  
  gfxRect snap(0, 0, 0, 0);
  if (mContainer) {
    gfxIntSize size = mContainer->GetCurrentSize();
    snap.SizeTo(gfxSize(size.width, size.height));
  }
  
  
  
  
  mEffectiveTransform =
      SnapTransform(GetLocalTransform(), snap, nullptr)*
      SnapTransform(aTransformToSurface, gfxRect(0, 0, 0, 0), nullptr);
  ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
}

}
}
