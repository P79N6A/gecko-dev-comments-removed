




#include "BasicLayersImpl.h"            
#include "ImageContainer.h"             
#include "ImageLayers.h"                
#include "Layers.h"                     
#include "basic/BasicImplData.h"        
#include "basic/BasicLayers.h"          
#include "gfxASurface.h"                
#include "gfxContext.h"                 
#include "gfxPattern.h"                 
#include "gfxUtils.h"                   
#ifdef MOZ_X11
#include "gfxXlibSurface.h"             
#endif
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
  BasicImageLayer(BasicLayerManager* aLayerManager) :
    ImageLayer(aLayerManager,
               static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST())),
    mSize(-1, -1)
  {
    MOZ_COUNT_CTOR(BasicImageLayer);
  }
  virtual ~BasicImageLayer()
  {
    MOZ_COUNT_DTOR(BasicImageLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ImageLayer::SetVisibleRegion(aRegion);
  }

  virtual void Paint(DrawTarget* aDT, Layer* aMaskLayer) MOZ_OVERRIDE;

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
BasicImageLayer::Paint(DrawTarget* aDT, Layer* aMaskLayer)
{
  if (IsHidden() || !mContainer) {
    return;
  }

  mContainer->SetImageFactory(mManager->IsCompositingCheap() ? nullptr : BasicManager()->GetImageFactory());

  RefPtr<gfx::SourceSurface> surface;
  AutoLockImage autoLock(mContainer, &surface);
  Image *image = autoLock.GetImage();
  gfx::IntSize size = mSize = autoLock.GetSize();

  if (!surface || !surface->IsValid()) {
    return;
  }

  FillRectWithMask(aDT, Rect(0, 0, size.width, size.height), surface, ToFilter(mFilter),
                   DrawOptions(GetEffectiveOpacity(), GetEffectiveOperator(this)),
                   aMaskLayer);

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

  mContainer->SetImageFactory(mManager->IsCompositingCheap() ?
                              nullptr :
                              BasicManager()->GetImageFactory());
  IntSize size;
  Image* image = nullptr;
  RefPtr<SourceSurface> surf =
    mContainer->LockCurrentAsSourceSurface(&size, &image);

  if (!surf) {
    return;
  }

  if (aTarget) {
    
    
    SurfacePattern pat(surf, ExtendMode::CLAMP, Matrix(), ToFilter(mFilter));
    CompositionOp op = GetEffectiveOperator(this);
    DrawOptions opts(aOpacity, op);

    aTarget->MaskSurface(pat, aMaskSurface, Point(0, 0), opts);

    GetContainer()->NotifyPaintedImage(image);
  }

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
