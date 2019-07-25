





#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h









#include "mozilla/layers/PCompositorParent.h"
#include "mozilla/layers/PLayersParent.h"
#include "base/thread.h"
#include "mozilla/Monitor.h"
#include "mozilla/TimeStamp.h"
#include "ShadowLayersManager.h"
class nsIWidget;

namespace base {
class Thread;
}

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
class Layer;
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
  CompositorParent(nsIWidget* aWidget,
                   bool aRenderToEGLSurface = false,
                   int aSurfaceWidth = -1, int aSurfaceHeight = -1);

  virtual ~CompositorParent();

  virtual bool RecvWillStop() MOZ_OVERRIDE;
  virtual bool RecvStop() MOZ_OVERRIDE;
  virtual bool RecvPause() MOZ_OVERRIDE;
  virtual bool RecvResume() MOZ_OVERRIDE;

  virtual void ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                   const TargetConfig& aTargetConfig,
                                   bool isFirstPaint) MOZ_OVERRIDE;
  void Destroy();

  LayerManager* GetLayerManager() { return mLayerManager; }

  void SetTransformation(float aScale, nsIntPoint aScrollOffset);
  void AsyncRender();

  
  void ScheduleRenderOnCompositorThread();
  void SchedulePauseOnCompositorThread();
  void ScheduleResumeOnCompositorThread(int width, int height);

  virtual void ScheduleComposition();
  void NotifyShadowTreeTransaction();

  


  static CompositorParent* GetCompositor(uint64_t id);

  




  static MessageLoop* CompositorLoop();

  


  static void StartUp();

  


  static void ShutDown();

  





  static uint64_t AllocateLayerTreeId();
  




  static void DeallocateLayerTreeId(uint64_t aId);

  





  static void SetPanZoomControllerForLayerTree(uint64_t aLayersId,
                                               AsyncPanZoomController* aController);

  



  static PCompositorParent*
  Create(Transport* aTransport, ProcessId aOtherProcess);

  





  static void StartUpWithExistingThread(MessageLoop* aMsgLoop,
                                        PlatformThreadId aThreadID);

protected:
  virtual PLayersParent* AllocPLayers(const LayersBackend& aBackendHint,
                                      const uint64_t& aId,
                                      LayersBackend* aBackend,
                                      int32_t* aMaxTextureSize);
  virtual bool DeallocPLayers(PLayersParent* aLayers);
  virtual void ScheduleTask(CancelableTask*, int);
  virtual void Composite();
  virtual void SetFirstPaintViewport(const nsIntPoint& aOffset, float aZoom, const nsIntRect& aPageRect, const gfx::Rect& aCssPageRect);
  virtual void SetPageRect(const gfx::Rect& aCssPageRect);
  virtual void SyncViewportInfo(const nsIntRect& aDisplayPort, float aDisplayResolution, bool aLayersUpdated,
                                nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY);
  void SetEGLSurfaceSize(int width, int height);

private:
  void PauseComposition();
  void ResumeComposition();
  void ResumeCompositionAndResize(int width, int height);

  
  
  bool TransformShadowTree(TimeStamp aCurrentFrame);
  
  
  
  bool ApplyAsyncContentTransformToTree(TimeStamp aCurrentFrame, Layer* aLayer,
                                        bool* aWantNextFrame);

  inline PlatformThreadId CompositorThreadID();

  






  static void CreateCompositorMap();
  static void DestroyCompositorMap();

  








  static bool CreateThread();

  






  static void DestroyThread();

  


  static void AddCompositor(CompositorParent* compositor, uint64_t* id);
  


  static CompositorParent* RemoveCompositor(uint64_t id);


  
  



  Layer* GetPrimaryScrollableLayer();

  






  void TransformFixedLayers(Layer* aLayer,
                            const gfxPoint& aTranslation,
                            const gfxPoint& aScaleDiff);

  nsRefPtr<LayerManager> mLayerManager;
  nsIWidget* mWidget;
  TargetConfig mTargetConfig;
  CancelableTask *mCurrentCompositeTask;
  TimeStamp mLastCompose;
#ifdef COMPOSITOR_PERFORMANCE_WARNING
  TimeStamp mExpectedComposeTime;
#endif

  bool mPaused;
  float mXScale;
  float mYScale;
  nsIntPoint mScrollOffset;
  nsIntRect mContentRect;
  nsIntSize mWidgetSize;

  
  
  
  
  
  bool mIsFirstPaint;

  
  
  bool mLayersUpdated;

  bool mRenderToEGLSurface;
  nsIntSize mEGLSurfaceSize;

  mozilla::Monitor mPauseCompositionMonitor;
  mozilla::Monitor mResumeCompositionMonitor;

  uint64_t mCompositorID;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 
