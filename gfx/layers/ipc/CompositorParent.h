







































#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h

#include "mozilla/layers/PCompositorParent.h"
#include "mozilla/layers/PLayersParent.h"
#include "base/thread.h"
#include "ShadowLayersManager.h"

class nsIWidget;

namespace mozilla {
namespace layers {

class LayerManager;


struct ViewTransform {
  ViewTransform(nsIntPoint aTranslation = nsIntPoint(0, 0), float aXScale = 1, float aYScale = 1)
    : mTranslation(aTranslation)
    , mXScale(aXScale)
    , mYScale(aYScale)
  {}

  operator gfx3DMatrix() const
  {
    return
      gfx3DMatrix::ScalingMatrix(mXScale, mYScale, 1) *
      gfx3DMatrix::Translation(mTranslation.x, mTranslation.y, 0);
  }

  nsIntPoint mTranslation;
  float mXScale;
  float mYScale;
};

class CompositorParent : public PCompositorParent,
                         public ShadowLayersManager
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositorParent)
public:
  CompositorParent(nsIWidget* aWidget);
  virtual ~CompositorParent();

  virtual bool RecvStop() MOZ_OVERRIDE;

  virtual void ShadowLayersUpdated() MOZ_OVERRIDE;
  void Destroy();

  LayerManager* GetLayerManager() { return mLayerManager; }

  void SetTransformation(float aScale, nsIntPoint aScrollOffset);
  void AsyncRender();

  
  void ScheduleRenderOnCompositorThread(::base::Thread &aCompositorThread);
  void SchedulePauseOnCompositorThread(::base::Thread &aCompositorThread);
  void ScheduleResumeOnCompositorThread(::base::Thread &aCompositorThread);

  void PauseComposition();
  void ResumeComposition();

protected:
  virtual PLayersParent* AllocPLayers(const LayersBackend &backendType);
  virtual bool DeallocPLayers(PLayersParent* aLayers);

private:
  void ScheduleComposition();
  void Composite();
  void TransformShadowTree(Layer* aLayer, const ViewTransform& aTransform,
                           float aTempScaleDiffX = 1.0,
                           float aTempScaleDiffY = 1.0);

  
#ifdef MOZ_WIDGET_ANDROID
  



  void RequestViewTransform();

  



  Layer* GetPrimaryScrollableLayer();
#endif

  nsRefPtr<LayerManager> mLayerManager;
  nsIWidget* mWidget;
  CancelableTask *mCurrentCompositeTask;
  TimeStamp mLastCompose;

  bool mPaused;
  float mXScale;
  float mYScale;
  nsIntPoint mScrollOffset;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 
