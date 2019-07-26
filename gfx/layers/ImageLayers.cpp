




#include "ImageLayers.h"
#include "ImageContainer.h"             
#include "gfx3DMatrix.h"                
#include "gfxRect.h"                    
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "gfx2DGlue.h"

namespace mozilla {
namespace layers {

ImageLayer::ImageLayer(LayerManager* aManager, void* aImplData)
: Layer(aManager, aImplData), mFilter(GraphicsFilter::FILTER_GOOD)
, mScaleMode(ScaleMode::SCALE_NONE), mDisallowBigImage(false)
{}

ImageLayer::~ImageLayer()
{}

void ImageLayer::SetContainer(ImageContainer* aContainer) 
{
  mContainer = aContainer;
}

void ImageLayer::ComputeEffectiveTransforms(const gfx::Matrix4x4& aTransformToSurface)
{
  gfx::Matrix4x4 local;
  gfx::ToMatrix4x4(GetLocalTransform(), local);

  
  gfxRect sourceRect(0, 0, 0, 0);
  if (mContainer) {
    sourceRect.SizeTo(gfx::ThebesIntSize(mContainer->GetCurrentSize()));
    if (mScaleMode != ScaleMode::SCALE_NONE &&
        sourceRect.width != 0.0 && sourceRect.height != 0.0) {
      NS_ASSERTION(mScaleMode == ScaleMode::STRETCH,
                   "No other scalemodes than stretch and none supported yet.");
      local.Scale(mScaleToSize.width / sourceRect.width,
                  mScaleToSize.height / sourceRect.height, 1.0);
    }
  }
  
  
  
  
  mEffectiveTransform =
      SnapTransform(local, sourceRect, nullptr) *
      SnapTransformTranslation(aTransformToSurface, nullptr);
  ComputeEffectiveTransformForMaskLayer(aTransformToSurface);
}

}
}
