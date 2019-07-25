







































#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h







#define COMPOSITOR_PERFORMANCE_WARNING

#include "mozilla/layers/PCompositorParent.h"
#include "mozilla/layers/PLayersParent.h"
#include "base/thread.h"
#include "ShadowLayersManager.h"

class nsIWidget;

namespace base {
class Thread;
}

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
  CompositorParent(nsIWidget* aWidget, base::Thread* aCompositorThread);
  virtual ~CompositorParent();

  virtual bool RecvStop() MOZ_OVERRIDE;

  virtual void ShadowLayersUpdated(bool isFirstPaint) MOZ_OVERRIDE;
  void Destroy();

  LayerManager* GetLayerManager() { return mLayerManager; }

  void SetTransformation(float aScale, nsIntPoint aScrollOffset);
  void AsyncRender();

  
  void ScheduleRenderOnCompositorThread();
  void SchedulePauseOnCompositorThread();
  void ScheduleResumeOnCompositorThread();

protected:
  virtual PLayersParent* AllocPLayers(const LayersBackend &backendType);
  virtual bool DeallocPLayers(PLayersParent* aLayers);

private:
  void PauseComposition();
  void ResumeComposition();

  void Composite();
  void ScheduleComposition();
  void TransformShadowTree();

  
#ifdef MOZ_WIDGET_ANDROID
  




  void SyncViewportInfo();

  



  Layer* GetPrimaryScrollableLayer();
#endif

  nsRefPtr<LayerManager> mLayerManager;
  base::Thread* mCompositorThread;
  nsIWidget* mWidget;
  CancelableTask *mCurrentCompositeTask;
  TimeStamp mLastCompose;
#ifdef COMPOSITOR_PERFORMANCE_WARNING
  TimeStamp mExpectedComposeTime;
#endif

  bool mPaused;
  float mXScale;
  float mYScale;
  nsIntPoint mScrollOffset;
  nsIntSize mContentSize;

  
  
  
  
  
  bool mIsFirstPaint;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 
