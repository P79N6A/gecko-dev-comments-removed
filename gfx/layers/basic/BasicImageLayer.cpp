




#include "BasicLayersImpl.h"            
#include "ImageContainer.h"             
#include "ImageLayers.h"                
#include "Layers.h"                     
#include "basic/BasicImplData.h"        
#include "basic/BasicLayers.h"          
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "mozilla/gfx/Point.h"          

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class BasicImageLayer : public ImageLayer, public BasicImplData {
public:
  explicit BasicImageLayer(BasicLayerManager* aLayerManager) :
    ImageLayer(aLayerManager,
               static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST())),
    mSize(-1, -1)
  {
    MOZ_COUNT_CTOR(BasicImageLayer);
  }
protected:
  virtual ~BasicImageLayer()
  {
    MOZ_COUNT_DTOR(BasicImageLayer);
  }

public:
  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ImageLayer::SetVisibleRegion(aRegion);
  }

  virtual void Paint(DrawTarget* aDT,
                     const gfx::Point& aDeviceOffset,
                     Layer* aMaskLayer) MOZ_OVERRIDE;

  virtual TemporaryRef<SourceSurface> GetAsSourceSurface() MOZ_OVERRIDE;

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

  
  void
  GetAndPaintCurrentImage(DrawTarget* aTarget,
                          float aOpacity,
                          SourceSurface* aMaskSurface);

  gfx::IntSize mSize;
};

void
BasicImageLayer::Paint(DrawTarget* aDT,
                       const gfx::Point& aDeviceOffset,
                       Layer* aMaskLayer)
{
  if (IsHidden() || !mContainer) {
    return;
  }

  nsRefPtr<ImageFactory> originalIF = mContainer->GetImageFactory();
  mContainer->SetImageFactory(mManager->IsCompositingCheap() ? nullptr : BasicManager()->GetImageFactory());

  RefPtr<gfx::SourceSurface> surface;
  AutoLockImage autoLock(mContainer, &surface);
  Image *image = autoLock.GetImage();
  gfx::IntSize size = mSize = autoLock.GetSize();

  if (!surface || !surface->IsValid()) {
    mContainer->SetImageFactory(originalIF);
    return;
  }

  FillRectWithMask(aDT, aDeviceOffset, Rect(0, 0, size.width, size.height), 
                   surface, ToFilter(mFilter),
                   DrawOptions(GetEffectiveOpacity(), GetEffectiveOperator(this)),
                   aMaskLayer);

  mContainer->SetImageFactory(originalIF);
  GetContainer()->NotifyPaintedImage(image);
}

void
BasicImageLayer::GetAndPaintCurrentImage(DrawTarget* aTarget,
                                         float aOpacity,
                                         SourceSurface* aMaskSurface)
{
  if (!mContainer) {
    return;
  }

  nsRefPtr<ImageFactory> originalIF = mContainer->GetImageFactory();
  mContainer->SetImageFactory(mManager->IsCompositingCheap() ?
                              nullptr :
                              BasicManager()->GetImageFactory());
  IntSize size;
  Image* image = nullptr;
  RefPtr<SourceSurface> surf =
    mContainer->LockCurrentAsSourceSurface(&size, &image);

  if (!surf) {
    mContainer->SetImageFactory(originalIF);
    return;
  }

  if (aTarget) {
    
    
    SurfacePattern pat(surf, ExtendMode::CLAMP, Matrix(), ToFilter(mFilter));
    CompositionOp op = GetEffectiveOperator(this);
    DrawOptions opts(aOpacity, op);

    aTarget->MaskSurface(pat, aMaskSurface, Point(0, 0), opts);

    GetContainer()->NotifyPaintedImage(image);
  }

  mContainer->SetImageFactory(originalIF);

  mContainer->UnlockCurrentImage();
}

TemporaryRef<SourceSurface>
BasicImageLayer::GetAsSourceSurface()
{
  if (!mContainer) {
    return nullptr;
  }

  gfx::IntSize dontCare;
  return mContainer->GetCurrentAsSourceSurface(&dontCare);
}

already_AddRefed<ImageLayer>
BasicLayerManager::CreateImageLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ImageLayer> layer = new BasicImageLayer(this);
  return layer.forget();
}

}
}
