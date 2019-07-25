







































#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h







#define COMPOSITOR_PERFORMANCE_WARNING

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
  CompositorParent(nsIWidget* aWidget, MessageLoop* aMsgLoop, PlatformThreadId aThreadID);

  virtual ~CompositorParent();

  virtual bool RecvWillStop() MOZ_OVERRIDE;
  virtual bool RecvStop() MOZ_OVERRIDE;
  virtual bool RecvPause() MOZ_OVERRIDE;
  virtual bool RecvResume() MOZ_OVERRIDE;

  virtual void ShadowLayersUpdated(bool isFirstPaint) MOZ_OVERRIDE;
  void Destroy();

  LayerManager* GetLayerManager() { return mLayerManager; }

  void SetTransformation(float aScale, nsIntPoint aScrollOffset);
  void AsyncRender();

  
  void ScheduleRenderOnCompositorThread();
  void SchedulePauseOnCompositorThread();
  void ScheduleResumeOnCompositorThread(int width, int height);

protected:
  virtual PLayersParent* AllocPLayers(const LayersBackend &backendType);
  virtual bool DeallocPLayers(PLayersParent* aLayers);
  virtual void ScheduleTask(CancelableTask*, int);
  virtual void Composite();
  virtual void ScheduleComposition();
  virtual void SetFirstPaintViewport(float aOffsetX, float aOffsetY, float aZoom, float aPageWidth, float aPageHeight,
                                     float aCssPageWidth, float aCssPageHeight);
  virtual void SetPageSize(float aZoom, float aPageWidth, float aPageHeight, float aCssPageWidth, float aCssPageHeight);
  virtual void SyncViewportInfo(const nsIntRect& aDisplayPort, float aDisplayResolution, bool aLayersUpdated,
                                nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY);

private:
  void PauseComposition();
  void ResumeComposition();
  void ResumeCompositionAndResize(int width, int height);

  void TransformShadowTree();

  inline MessageLoop* CompositorLoop();
  inline PlatformThreadId CompositorThreadID();

  
  



  Layer* GetPrimaryScrollableLayer();

  nsRefPtr<LayerManager> mLayerManager;
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

  
  
  bool mLayersUpdated;

  MessageLoop* mCompositorLoop;
  PlatformThreadId mThreadID;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 
