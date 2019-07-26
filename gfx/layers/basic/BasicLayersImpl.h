




#ifndef GFX_BASICLAYERSIMPL_H
#define GFX_BASICLAYERSIMPL_H

#include "BasicImplData.h"              
#include "BasicLayers.h"                
#include "ReadbackLayer.h"              
#include "gfxASurface.h"                
#include "gfxContext.h"                 
#include "gfxMatrix.h"                  
#include "ipc/AutoOpenSurface.h"        
#include "mozilla/Attributes.h"         
#include "mozilla/Util.h"               
#include "mozilla/layers/LayersSurfaces.h"  
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRegion.h"                   
#include "nsTraceRefcnt.h"              

namespace mozilla {
namespace layers {

class BasicContainerLayer;
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
  BasicReadbackLayer(BasicLayerManager* aLayerManager) :
    ReadbackLayer(aLayerManager,
                  static_cast<BasicImplData*>(MOZ_THIS_IN_INITIALIZER_LIST()))
  {
    MOZ_COUNT_CTOR(BasicReadbackLayer);
  }
  virtual ~BasicReadbackLayer()
  {
    MOZ_COUNT_DTOR(BasicReadbackLayer);
  }

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











class MOZ_STACK_CLASS AutoMaskData {
public:
  AutoMaskData() { }
  ~AutoMaskData() { }

  






  void Construct(const gfxMatrix& aTransform,
                 gfxASurface* aSurface);

  void Construct(const gfxMatrix& aTransform,
                 const SurfaceDescriptor& aSurface);

  
  gfxASurface* GetSurface();
  const gfxMatrix& GetTransform();

private:
  bool IsConstructed();

  gfxMatrix mTransform;
  nsRefPtr<gfxASurface> mSurface;
  Maybe<AutoOpenSurface> mSurfaceOpener;

  AutoMaskData(const AutoMaskData&) MOZ_DELETE;
  AutoMaskData& operator=(const AutoMaskData&) MOZ_DELETE;
};








bool
GetMaskData(Layer* aMaskLayer, AutoMaskData* aMaskData);


void
PaintWithMask(gfxContext* aContext, float aOpacity, Layer* aMaskLayer);



void
FillWithMask(gfxContext* aContext, float aOpacity, Layer* aMaskLayer);

BasicImplData*
ToData(Layer* aLayer);

}
}

#endif
