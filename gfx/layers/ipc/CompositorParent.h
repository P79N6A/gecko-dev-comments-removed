





#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h









#include "mozilla/layers/PCompositorParent.h"
#include "mozilla/layers/PLayerTransactionParent.h"
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
class LayerManagerComposite;
class AsyncCompositionManager;
struct TextureFactoryIdentifier;

class CompositorParent : public PCompositorParent,
                         public ShadowLayersManager
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositorParent)

public:
  CompositorParent(nsIWidget* aWidget,
                   bool aUseExternalSurfaceSize = false,
                   int aSurfaceWidth = -1, int aSurfaceHeight = -1);

  virtual ~CompositorParent();

  virtual bool RecvWillStop() MOZ_OVERRIDE;
  virtual bool RecvStop() MOZ_OVERRIDE;
  virtual bool RecvPause() MOZ_OVERRIDE;
  virtual bool RecvResume() MOZ_OVERRIDE;
  virtual bool RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                SurfaceDescriptor* aOutSnapshot);

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  virtual void ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                   const TargetConfig& aTargetConfig,
                                   bool isFirstPaint) MOZ_OVERRIDE;
  






  void ForceIsFirstPaint();
  void Destroy();

  LayerManagerComposite* GetLayerManager() { return mLayerManager; }

  void SetTransformation(float aScale, nsIntPoint aScrollOffset);

  void AsyncRender();

  
  void ScheduleRenderOnCompositorThread();
  void SchedulePauseOnCompositorThread();
  



  bool ScheduleResumeOnCompositorThread(int width, int height);

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

  struct LayerTreeState {
    nsRefPtr<Layer> mRoot;
    nsRefPtr<AsyncPanZoomController> mController;
    TargetConfig mTargetConfig;
  };

  




  static const LayerTreeState* GetIndirectShadowTree(uint64_t aId);

protected:
  virtual PLayerTransactionParent*
    AllocPLayerTransaction(const LayersBackend& aBackendHint,
                           const uint64_t& aId,
                           TextureFactoryIdentifier* aTextureFactoryIdentifier);
  virtual bool DeallocPLayerTransaction(PLayerTransactionParent* aLayers);
  virtual void ScheduleTask(CancelableTask*, int);
  virtual void Composite();
  virtual void ComposeToTarget(gfxContext* aTarget);

  void SetEGLSurfaceSize(int width, int height);

private:
  void PauseComposition();
  void ResumeComposition();
  void ResumeCompositionAndResize(int width, int height);
  void ForceComposition();

  inline PlatformThreadId CompositorThreadID();

  






  static void CreateCompositorMap();
  static void DestroyCompositorMap();

  








  static bool CreateThread();

  






  static void DestroyThread();

  


  static void AddCompositor(CompositorParent* compositor, uint64_t* id);
  


  static CompositorParent* RemoveCompositor(uint64_t id);

   



  bool CanComposite();

  nsRefPtr<LayerManagerComposite> mLayerManager;
  RefPtr<AsyncCompositionManager> mCompositionManager;
  nsIWidget* mWidget;
  CancelableTask *mCurrentCompositeTask;
  TimeStamp mLastCompose;
#ifdef COMPOSITOR_PERFORMANCE_WARNING
  TimeStamp mExpectedComposeTime;
#endif

  bool mPaused;

  bool mUseExternalSurfaceSize;
  nsIntSize mEGLSurfaceSize;

  mozilla::Monitor mPauseCompositionMonitor;
  mozilla::Monitor mResumeCompositionMonitor;

  uint64_t mCompositorID;

  bool mOverrideComposeReadiness;
  CancelableTask* mForceCompositionTask;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 
