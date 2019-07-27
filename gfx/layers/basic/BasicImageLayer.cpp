




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
    ImageLayer(aLayerManager, static_cast<BasicImplData*>(this)),
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
  virtual void SetVisibleRegion(const nsIntRegion& aRegion) override
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ImageLayer::SetVisibleRegion(aRegion);
  }

  virtual void Paint(DrawTarget* aDT,
                     const gfx::Point& aDeviceOffset,
                     Layer* aMaskLayer) override;

  virtual already_AddRefed<SourceSurface> GetAsSourceSurface() override;

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

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

  AutoLockImage autoLock(mContainer);
  Image *image = autoLock.GetImage();
  if (!image) {
    mContainer->SetImageFactory(originalIF);
    return;
  }
  RefPtr<gfx::SourceSurface> surface = image->GetAsSourceSurface();
  if (!surface || !surface->IsValid()) {
    mContainer->SetImageFactory(originalIF);
    return;
  }

  gfx::IntSize size = mSize = surface->GetSize();
  FillRectWithMask(aDT, aDeviceOffset, Rect(0, 0, size.width, size.height), 
                   surface, ToFilter(mFilter),
                   DrawOptions(GetEffectiveOpacity(), GetEffectiveOperator(this)),
                   aMaskLayer);

  mContainer->SetImageFactory(originalIF);
}

already_AddRefed<SourceSurface>
BasicImageLayer::GetAsSourceSurface()
{
  if (!mContainer) {
    return nullptr;
  }

  AutoLockImage lockImage(mContainer);
  Image* image = lockImage.GetImage();
  if (!image) {
    return nullptr;
  }
  return image->GetAsSourceSurface();
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
