




#ifndef GFX_BASICLAYERSIMPL_H
#define GFX_BASICLAYERSIMPL_H

#include "BasicImplData.h"              
#include "BasicLayers.h"                
#include "ReadbackLayer.h"              
#include "gfxContext.h"                 
#include "mozilla/Attributes.h"         
#include "mozilla/Maybe.h"              
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   

namespace mozilla {
namespace gfx {
class DrawTarget;
}

namespace layers {

class AutoMoz2DMaskData;
class Layer;

class AutoSetOperator {
public:
  AutoSetOperator(gfxContext* aContext, gfxContext::GraphicsOperator aOperator) {
    if (aOperator != gfxContext::OPERATOR_OVER) {
      aContext->SetOperator(aOperator);
      mContext = aContext;
    }
  }
  ~AutoSetOperator() {
    if (mContext) {
      mContext->SetOperator(gfxContext::OPERATOR_OVER);
    }
  }
private:
  nsRefPtr<gfxContext> mContext;
};

class BasicReadbackLayer : public ReadbackLayer,
                           public BasicImplData
{
public:
  explicit BasicReadbackLayer(BasicLayerManager* aLayerManager) :
    ReadbackLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicReadbackLayer);
  }

protected:
  virtual ~BasicReadbackLayer()
  {
    MOZ_COUNT_DTOR(BasicReadbackLayer);
  }

public:
  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ReadbackLayer::SetVisibleRegion(aRegion);
  }

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};








bool
GetMaskData(Layer* aMaskLayer,
            const gfx::Point& aDeviceOffset,
            AutoMoz2DMaskData* aMaskData);


void
PaintWithMask(gfxContext* aContext, float aOpacity, Layer* aMaskLayer);


void
FillRectWithMask(gfx::DrawTarget* aDT,
                 const gfx::Rect& aRect,
                 const gfx::Color& aColor,
                 const gfx::DrawOptions& aOptions,
                 gfx::SourceSurface* aMaskSource = nullptr,
                 const gfx::Matrix* aMaskTransform = nullptr);
void
FillRectWithMask(gfx::DrawTarget* aDT,
                 const gfx::Rect& aRect,
                 gfx::SourceSurface* aSurface,
                 gfx::Filter aFilter,
                 const gfx::DrawOptions& aOptions,
                 gfx::ExtendMode aExtendMode,
                 gfx::SourceSurface* aMaskSource = nullptr,
                 const gfx::Matrix* aMaskTransform = nullptr,
                 const gfx::Matrix* aSurfaceTransform = nullptr);
void
FillRectWithMask(gfx::DrawTarget* aDT,
                 const gfx::Point& aDeviceOffset,
                 const gfx::Rect& aRect,
                 gfx::SourceSurface* aSurface,
                 gfx::Filter aFilter,
                 const gfx::DrawOptions& aOptions,
                 Layer* aMaskLayer);
void
FillRectWithMask(gfx::DrawTarget* aDT,
                 const gfx::Point& aDeviceOffset,
                 const gfx::Rect& aRect,
                 const gfx::Color& aColor,
                 const gfx::DrawOptions& aOptions,
                 Layer* aMaskLayer);

BasicImplData*
ToData(Layer* aLayer);













gfx::CompositionOp
GetEffectiveOperator(Layer* aLayer);

}
}

#endif
