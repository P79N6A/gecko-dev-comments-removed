




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

  virtual void Paint(DrawTarget* aTarget, SourceSurface* aMaskSurface);
  virtual void DeprecatedPaint(gfxContext* aContext, Layer* aMaskLayer);

  virtual bool GetAsSurface(gfxASurface** aSurface,
                            SurfaceDescriptor* aDescriptor);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

  
  void
  GetAndPaintCurrentImage(DrawTarget* aTarget,
                          float aOpacity,
                          SourceSurface* aMaskSurface);
  already_AddRefed<gfxPattern>
  DeprecatedGetAndPaintCurrentImage(gfxContext* aContext,
                                    float aOpacity,
                                    Layer* aMaskLayer);

  gfx::IntSize mSize;
};

static void
DeprecatedPaintContext(gfxPattern* aPattern,
                       const nsIntRegion& aVisible,
                       float aOpacity,
                       gfxContext* aContext,
                       Layer* aMaskLayer);

void
BasicImageLayer::Paint(DrawTarget* aTarget, SourceSurface* aMaskSurface)
{
  if (IsHidden()) {
    return;
  }
  GetAndPaintCurrentImage(aTarget, GetEffectiveOpacity(), aMaskSurface);
}

void
BasicImageLayer::DeprecatedPaint(gfxContext* aContext, Layer* aMaskLayer)
{
  if (IsHidden()) {
    return;
  }
  nsRefPtr<gfxPattern> dontcare =
    DeprecatedGetAndPaintCurrentImage(aContext,
                                      GetEffectiveOpacity(),
                                      aMaskLayer);
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

already_AddRefed<gfxPattern>
BasicImageLayer::DeprecatedGetAndPaintCurrentImage(gfxContext* aContext,
                                                   float aOpacity,
                                                   Layer* aMaskLayer)
{
  if (!mContainer)
    return nullptr;

  mContainer->SetImageFactory(mManager->IsCompositingCheap() ? nullptr : BasicManager()->GetImageFactory());

  RefPtr<gfx::SourceSurface> surface;
  AutoLockImage autoLock(mContainer, &surface);
  Image *image = autoLock.GetImage();
  gfx::IntSize size = mSize = autoLock.GetSize();

  if (!surface || !surface->IsValid()) {
    return nullptr;
  }

  nsRefPtr<gfxPattern> pat = new gfxPattern(surface, gfx::Matrix());
  if (!pat) {
    return nullptr;
  }

  pat->SetFilter(mFilter);

  
  
  if (aContext) {
    CompositionOp op = GetEffectiveOperator(this);
    AutoSetOperator setOptimizedOperator(aContext, ThebesOp(op));

    DeprecatedPaintContext(pat,
                 nsIntRegion(nsIntRect(0, 0, size.width, size.height)),
                 aOpacity, aContext, aMaskLayer);

    GetContainer()->NotifyPaintedImage(image);
  }

  return pat.forget();
}

static void
DeprecatedPaintContext(gfxPattern* aPattern,
                       const nsIntRegion& aVisible,
                       float aOpacity,
                       gfxContext* aContext,
                       Layer* aMaskLayer)
{
  
  
  gfxPattern::GraphicsExtend extend = gfxPattern::EXTEND_PAD;

#ifdef MOZ_X11
  
  
  if (aContext->IsCairo()) {
    nsRefPtr<gfxASurface> target = aContext->CurrentSurface();
    if (target->GetType() == gfxSurfaceType::Xlib &&
        static_cast<gfxXlibSurface*>(target.get())->IsPadSlow()) {
      extend = gfxPattern::EXTEND_NONE;
    }
  }
#endif

  aContext->NewPath();
  
  
  gfxUtils::PathFromRegion(aContext, aVisible);
  aPattern->SetExtend(extend);
  aContext->SetPattern(aPattern);
  FillWithMask(aContext, aOpacity, aMaskLayer);

  
  aPattern->SetExtend(extend);
}

bool
BasicImageLayer::GetAsSurface(gfxASurface** aSurface,
                              SurfaceDescriptor* aDescriptor)
{
  if (!mContainer) {
    return false;
  }

  gfx::IntSize dontCare;
  nsRefPtr<gfxASurface> surface = mContainer->DeprecatedGetCurrentAsSurface(&dontCare);
  surface.forget(aSurface);
  return true;
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
