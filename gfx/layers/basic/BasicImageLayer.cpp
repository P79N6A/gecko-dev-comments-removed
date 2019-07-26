




#include "BasicLayersImpl.h"            
#include "ImageContainer.h"             
#include "ImageLayers.h"                
#include "Layers.h"                     
#include "basic/BasicImplData.h"        
#include "basic/BasicLayers.h"          
#include "gfxASurface.h"                
#include "gfxContext.h"                 
#include "gfxPattern.h"                 
#include "gfxPoint.h"                   
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
#include "nsTraceRefcnt.h"              

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

  virtual void Paint(gfxContext* aContext, Layer* aMaskLayer);

  virtual bool GetAsSurface(gfxASurface** aSurface,
                            SurfaceDescriptor* aDescriptor);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

  
  already_AddRefed<gfxPattern>
  GetAndPaintCurrentImage(gfxContext* aContext,
                          float aOpacity,
                          Layer* aMaskLayer);

  gfxIntSize mSize;
};

void
BasicImageLayer::Paint(gfxContext* aContext, Layer* aMaskLayer)
{
  if (IsHidden())
    return;
  nsRefPtr<gfxPattern> dontcare =
    GetAndPaintCurrentImage(aContext, GetEffectiveOpacity(), aMaskLayer);
}

already_AddRefed<gfxPattern>
BasicImageLayer::GetAndPaintCurrentImage(gfxContext* aContext,
                                         float aOpacity,
                                         Layer* aMaskLayer)
{
  if (!mContainer)
    return nullptr;

  mContainer->SetImageFactory(mManager->IsCompositingCheap() ? nullptr : BasicManager()->GetImageFactory());

  nsRefPtr<gfxASurface> surface;
  AutoLockImage autoLock(mContainer, getter_AddRefs(surface));
  Image *image = autoLock.GetImage();
  gfxIntSize size = mSize = autoLock.GetSize();

  if (!surface || surface->CairoStatus()) {
    return nullptr;
  }

  nsRefPtr<gfxPattern> pat = new gfxPattern(surface);
  if (!pat) {
    return nullptr;
  }

  pat->SetFilter(mFilter);

  
  
  if (aContext) {
    AutoSetOperator setOperator(aContext, GetOperator());
    PaintContext(pat,
                 nsIntRegion(nsIntRect(0, 0, size.width, size.height)),
                 aOpacity, aContext, aMaskLayer);

    GetContainer()->NotifyPaintedImage(image);
  }

  return pat.forget();
}

void
PaintContext(gfxPattern* aPattern,
             const nsIntRegion& aVisible,
             float aOpacity,
             gfxContext* aContext,
             Layer* aMaskLayer)
{
  
  
  gfxPattern::GraphicsExtend extend = gfxPattern::EXTEND_PAD;

#ifdef MOZ_X11
  
  
  if (aContext->IsCairo()) {
    nsRefPtr<gfxASurface> target = aContext->CurrentSurface();
    if (target->GetType() == gfxASurface::SurfaceTypeXlib &&
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

  gfxIntSize dontCare;
  nsRefPtr<gfxASurface> surface = mContainer->GetCurrentAsSurface(&dontCare);
  *aSurface = surface.forget().get();
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
