



#ifndef GFX_BASICIMPLDATA_H
#define GFX_BASICIMPLDATA_H

#include "Layers.h"                     
#include "gfxContext.h"                 
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "mozilla/gfx/Types.h"

namespace mozilla {
namespace layers {

class ReadbackProcessor;























class BasicImplData {
public:
  BasicImplData() : mHidden(false),
    mClipToVisibleRegion(false),
    mDrawAtomically(false),
    mOperator(gfx::CompositionOp::OP_OVER)
  {
    MOZ_COUNT_CTOR(BasicImplData);
  }
  virtual ~BasicImplData()
  {
    MOZ_COUNT_DTOR(BasicImplData);
  }

  





  virtual void Paint(gfx::DrawTarget* aDT,
                     const gfx::Point& aDeviceOffset,
                     Layer* aMaskLayer) {}

  





  virtual void PaintThebes(gfxContext* aContext,
                           Layer* aMasklayer,
                           LayerManager::DrawPaintedLayerCallback aCallback,
                           void* aCallbackData) {}

  virtual void Validate(LayerManager::DrawPaintedLayerCallback aCallback,
                        void* aCallbackData,
                        ReadbackProcessor* aReadback) {}

  




  virtual void ClearCachedResources() {}

  



  void SetHidden(bool aCovered) { mHidden = aCovered; }
  bool IsHidden() const { return false; }
  




  void SetOperator(gfx::CompositionOp aOperator)
  {
    NS_ASSERTION(aOperator == gfx::CompositionOp::OP_OVER ||
                 aOperator == gfx::CompositionOp::OP_SOURCE,
                 "Bad composition operator");
    mOperator = aOperator;
  }

  gfx::CompositionOp GetOperator() const { return mOperator; }
  gfxContext::GraphicsOperator DeprecatedGetOperator() const
  {
    return gfx::ThebesOp(mOperator);
  }

  






  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() { return nullptr; }

  bool GetClipToVisibleRegion() { return mClipToVisibleRegion; }
  void SetClipToVisibleRegion(bool aClip) { mClipToVisibleRegion = aClip; }

  void SetDrawAtomically(bool aDrawAtomically) { mDrawAtomically = aDrawAtomically; }

protected:
  bool mHidden;
  bool mClipToVisibleRegion;
  bool mDrawAtomically;
  gfx::CompositionOp mOperator;
};

} 
} 

#endif
