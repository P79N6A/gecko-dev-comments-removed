



#ifndef GFX_BASICIMPLDATA_H
#define GFX_BASICIMPLDATA_H

namespace mozilla {
namespace layers {























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

  





  virtual bool MustRetainContent() { return false; }

  




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

  





  virtual already_AddRefed<gfxASurface> GetAsSurface() { return nsnull; }

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
