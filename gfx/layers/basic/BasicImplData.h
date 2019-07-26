



#ifndef GFX_BASICIMPLDATA_H
#define GFX_BASICIMPLDATA_H

#include "Layers.h"                     
#include "gfxContext.h"                 
#include "nsDebug.h"                    
#include "nsTraceRefcnt.h"              
class gfxASurface;

namespace mozilla {
namespace layers {

class ReadbackProcessor;
class SurfaceDescriptor;























class BasicImplData {
public:
  BasicImplData() : mHidden(false),
    mClipToVisibleRegion(false),
    mDrawAtomically(false),
    mOperator(gfxContext::OPERATOR_OVER)
  {
    MOZ_COUNT_CTOR(BasicImplData);
  }
  virtual ~BasicImplData()
  {
    MOZ_COUNT_DTOR(BasicImplData);
  }

  





  virtual void Paint(gfxContext* aContext, Layer* aMaskLayer) {}

  





  virtual void PaintThebes(gfxContext* aContext,
                           Layer* aMasklayer,
                           LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData,
                           ReadbackProcessor* aReadback) {}

  virtual void Validate(LayerManager::DrawThebesLayerCallback aCallback,
                        void* aCallbackData) {}

  




  virtual void ClearCachedResources() {}

  



  void SetHidden(bool aCovered) { mHidden = aCovered; }
  bool IsHidden() const { return false; }
  




  void SetOperator(gfxContext::GraphicsOperator aOperator)
  {
    NS_ASSERTION(aOperator == gfxContext::OPERATOR_OVER ||
                 aOperator == gfxContext::OPERATOR_SOURCE,
                 "Bad composition operator");
    mOperator = aOperator;
  }
  gfxContext::GraphicsOperator GetOperator() const { return mOperator; }

  






  virtual bool GetAsSurface(gfxASurface** aSurface,
                            SurfaceDescriptor* aDescriptor)
  { return false; }

  bool GetClipToVisibleRegion() { return mClipToVisibleRegion; }
  void SetClipToVisibleRegion(bool aClip) { mClipToVisibleRegion = aClip; }

  void SetDrawAtomically(bool aDrawAtomically) { mDrawAtomically = aDrawAtomically; }

protected:
  bool mHidden;
  bool mClipToVisibleRegion;
  bool mDrawAtomically;
  gfxContext::GraphicsOperator mOperator;
};

} 
} 

#endif
