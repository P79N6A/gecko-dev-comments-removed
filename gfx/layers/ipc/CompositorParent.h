





#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h









#include <stdint.h>                     
#include "Layers.h"                     
#include "ShadowLayersManager.h"        
#include "base/basictypes.h"            
#include "base/platform_thread.h"       
#include "base/thread.h"                
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/Monitor.h"            
#include "mozilla/RefPtr.h"             
#include "mozilla/TimeStamp.h"          
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/LayersMessages.h"  
#include "mozilla/layers/PCompositorParent.h"
#include "mozilla/layers/APZTestData.h"
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"
#include "nsSize.h"                     
#include "ThreadSafeRefcountingWithMainThreadDestruction.h"

class CancelableTask;
class MessageLoop;
class gfxContext;
class nsIWidget;

namespace mozilla {
namespace gfx {
class DrawTarget;
}

namespace layers {

class APZCTreeManager;
class AsyncCompositionManager;
class Compositor;
class LayerManagerComposite;
class LayerTransactionParent;

struct ScopedLayerTreeRegistration
{
  ScopedLayerTreeRegistration(uint64_t aLayersId,
                              Layer* aRoot,
                              GeckoContentController* aController);
  ~ScopedLayerTreeRegistration();

private:
  uint64_t mLayersId;
};

class CompositorThreadHolder MOZ_FINAL
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(CompositorThreadHolder)

public:
  CompositorThreadHolder();

  base::Thread* GetCompositorThread() const {
    return mCompositorThread;
  }

private:
  ~CompositorThreadHolder();

  base::Thread* const mCompositorThread;

  static base::Thread* CreateCompositorThread();
  static void DestroyCompositorThread(base::Thread* aCompositorThread);

  friend class CompositorParent;
};

class CompositorParent : public PCompositorParent,
                         public ShadowLayersManager
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(CompositorParent)

public:
  CompositorParent(nsIWidget* aWidget,
                   bool aUseExternalSurfaceSize = false,
                   int aSurfaceWidth = -1, int aSurfaceHeight = -1);

  
  virtual IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<mozilla::ipc::ProtocolFdMapping>& aFds,
                base::ProcessHandle aPeerProcess,
                mozilla::ipc::ProtocolCloneContext* aCtx) MOZ_OVERRIDE;

  virtual bool RecvRequestOverfill() MOZ_OVERRIDE;
  virtual bool RecvWillStop() MOZ_OVERRIDE;
  virtual bool RecvStop() MOZ_OVERRIDE;
  virtual bool RecvPause() MOZ_OVERRIDE;
  virtual bool RecvResume() MOZ_OVERRIDE;
  virtual bool RecvNotifyChildCreated(const uint64_t& child) MOZ_OVERRIDE;
  virtual bool RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                const nsIntRect& aRect) MOZ_OVERRIDE;
  virtual bool RecvFlushRendering() MOZ_OVERRIDE;

  virtual bool RecvNotifyRegionInvalidated(const nsIntRegion& aRegion) MOZ_OVERRIDE;
  virtual bool RecvStartFrameTimeRecording(const int32_t& aBufferSize, uint32_t* aOutStartIndex) MOZ_OVERRIDE;
  virtual bool RecvStopFrameTimeRecording(const uint32_t& aStartIndex, InfallibleTArray<float>* intervals) MOZ_OVERRIDE;

  
  
  virtual bool RecvRequestNotifyAfterRemotePaint() MOZ_OVERRIDE { return true; };

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  virtual void ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                   const uint64_t& aTransactionId,
                                   const TargetConfig& aTargetConfig,
                                   bool aIsFirstPaint,
                                   bool aScheduleComposite,
                                   uint32_t aPaintSequenceNumber,
                                   bool aIsRepeatTransaction) MOZ_OVERRIDE;
  virtual void ForceComposite(LayerTransactionParent* aLayerTree) MOZ_OVERRIDE;
  virtual bool SetTestSampleTime(LayerTransactionParent* aLayerTree,
                                 const TimeStamp& aTime) MOZ_OVERRIDE;
  virtual void LeaveTestMode(LayerTransactionParent* aLayerTree) MOZ_OVERRIDE;
  virtual void GetAPZTestData(const LayerTransactionParent* aLayerTree,
                              APZTestData* aOutData) MOZ_OVERRIDE;
  virtual AsyncCompositionManager* GetCompositionManager(LayerTransactionParent* aLayerTree) MOZ_OVERRIDE { return mCompositionManager; }

  






  void ForceIsFirstPaint();
  void Destroy();

  void NotifyChildCreated(const uint64_t& aChild);

  void AsyncRender();

  
  void ScheduleRenderOnCompositorThread();
  void SchedulePauseOnCompositorThread();
  



  bool ScheduleResumeOnCompositorThread(int width, int height);

  virtual void ScheduleComposition();
  void NotifyShadowTreeTransaction(uint64_t aId, bool aIsFirstPaint,
      bool aScheduleComposite, uint32_t aPaintSequenceNumber,
      bool aIsRepeatTransaction);

  



  void ScheduleRotationOnCompositorThread(const TargetConfig& aTargetConfig, bool aIsFirstPaint);

  



  uint64_t RootLayerTreeId();

  


  static CompositorParent* GetCompositor(uint64_t id);

  




  static MessageLoop* CompositorLoop();

  


  static void StartUp();

  





  static void ShutDown();

  





  static uint64_t AllocateLayerTreeId();
  




  static void DeallocateLayerTreeId(uint64_t aId);

  





  static void SetControllerForLayerTree(uint64_t aLayersId,
                                        GeckoContentController* aController);

  



  static APZCTreeManager* GetAPZCTreeManager(uint64_t aLayersId);

  



  static PCompositorParent*
  Create(Transport* aTransport, ProcessId aOtherProcess);

  struct LayerTreeState {
    LayerTreeState();
    nsRefPtr<Layer> mRoot;
    nsRefPtr<GeckoContentController> mController;
    CompositorParent* mParent;
    LayerManagerComposite* mLayerManager;
    
    
    
    PCompositorParent* mCrossProcessParent;
    TargetConfig mTargetConfig;
    APZTestData mApzTestData;
    LayerTransactionParent* mLayerTree;
  };

  




  static LayerTreeState* GetIndirectShadowTree(uint64_t aId);

  float ComputeRenderIntegrity();

  


  static bool IsInCompositorThread();

protected:
  
  virtual ~CompositorParent();

  void DeferredDestroy();

  virtual PLayerTransactionParent*
    AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                                 const uint64_t& aId,
                                 TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                 bool* aSuccess) MOZ_OVERRIDE;
  virtual bool DeallocPLayerTransactionParent(PLayerTransactionParent* aLayers) MOZ_OVERRIDE;
  virtual void ScheduleTask(CancelableTask*, int);
  void CompositeCallback();
  void CompositeToTarget(gfx::DrawTarget* aTarget, const nsIntRect* aRect = nullptr);
  void ForceComposeToTarget(gfx::DrawTarget* aTarget, const nsIntRect* aRect = nullptr);

  void SetEGLSurfaceSize(int width, int height);

  void InitializeLayerManager(const nsTArray<LayersBackend>& aBackendHints);
  void PauseComposition();
  void ResumeComposition();
  void ResumeCompositionAndResize(int width, int height);
  void ForceComposition();
  void CancelCurrentCompositeTask();

  


  static void AddCompositor(CompositorParent* compositor, uint64_t* id);
  


  static CompositorParent* RemoveCompositor(uint64_t id);

   



  bool CanComposite();

  void DidComposite();

  nsRefPtr<LayerManagerComposite> mLayerManager;
  nsRefPtr<Compositor> mCompositor;
  RefPtr<AsyncCompositionManager> mCompositionManager;
  nsIWidget* mWidget;
  CancelableTask *mCurrentCompositeTask;
  TimeStamp mLastCompose;
  TimeStamp mTestTime;
  bool mIsTesting;
#ifdef COMPOSITOR_PERFORMANCE_WARNING
  TimeStamp mExpectedComposeStartTime;
#endif

  uint64_t mPendingTransaction;

  bool mPaused;

  bool mUseExternalSurfaceSize;
  nsIntSize mEGLSurfaceSize;

  mozilla::Monitor mPauseCompositionMonitor;
  mozilla::Monitor mResumeCompositionMonitor;

  uint64_t mCompositorID;
  uint64_t mRootLayerTreeID;

  bool mOverrideComposeReadiness;
  CancelableTask* mForceCompositionTask;

  nsRefPtr<APZCTreeManager> mApzcTreeManager;

  nsRefPtr<CompositorThreadHolder> mCompositorThreadHolder;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 
