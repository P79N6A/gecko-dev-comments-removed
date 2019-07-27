





#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h









#include <stdint.h>                     
#include "Layers.h"                     
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
#include "mozilla/layers/ShadowLayersManager.h" 
#include "mozilla/layers/APZTestData.h"
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"
#include "nsSize.h"                     
#include "ThreadSafeRefcountingWithMainThreadDestruction.h"
#include "mozilla/VsyncDispatcher.h"

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
class CompositorParent;
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

class CompositorThreadHolder final
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







class CompositorVsyncObserver final : public VsyncObserver
{
  friend class CompositorParent;

public:
  explicit CompositorVsyncObserver(CompositorParent* aCompositorParent, nsIWidget* aWidget);
  virtual bool NotifyVsync(TimeStamp aVsyncTimestamp) override;
  void SetNeedsComposite(bool aSchedule);
  bool NeedsComposite();
  void CancelCurrentCompositeTask();
  void Destroy();
  void OnForceComposeToTarget();

private:
  virtual ~CompositorVsyncObserver();

  void Composite(TimeStamp aVsyncTimestamp);
  void NotifyCompositeTaskExecuted();
  void ObserveVsync();
  void UnobserveVsync();
  void DispatchTouchEvents(TimeStamp aVsyncTimestamp);
  void CancelCurrentSetNeedsCompositeTask();

  bool mNeedsComposite;
  bool mIsObservingVsync;
  int32_t mVsyncNotificationsSkipped;
  nsRefPtr<CompositorParent> mCompositorParent;
  nsRefPtr<CompositorVsyncDispatcher> mCompositorVsyncDispatcher;

  mozilla::Monitor mCurrentCompositeTaskMonitor;
  CancelableTask* mCurrentCompositeTask;

  mozilla::Monitor mSetNeedsCompositeMonitor;
  CancelableTask* mSetNeedsCompositeTask;
};

class CompositorUpdateObserver
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositorUpdateObserver);

  virtual void ObserveUpdate(uint64_t aLayersId, bool aActive) = 0;

protected:
  virtual ~CompositorUpdateObserver() {}
};

class CompositorParent final : public PCompositorParent,
                               public ShadowLayersManager
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(CompositorParent)
  friend class CompositorVsyncObserver;

public:
  explicit CompositorParent(nsIWidget* aWidget,
                            bool aUseExternalSurfaceSize = false,
                            int aSurfaceWidth = -1, int aSurfaceHeight = -1);

  
  virtual IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<mozilla::ipc::ProtocolFdMapping>& aFds,
                base::ProcessHandle aPeerProcess,
                mozilla::ipc::ProtocolCloneContext* aCtx) override;

  virtual bool RecvRequestOverfill() override;
  virtual bool RecvWillStop() override;
  virtual bool RecvStop() override;
  virtual bool RecvPause() override;
  virtual bool RecvResume() override;
  virtual bool RecvNotifyChildCreated(const uint64_t& child) override;
  virtual bool RecvAdoptChild(const uint64_t& child) override;
  virtual bool RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                const nsIntRect& aRect) override;
  virtual bool RecvFlushRendering() override;

  virtual bool RecvGetTileSize(int32_t* aWidth, int32_t* aHeight) override;

  virtual bool RecvNotifyRegionInvalidated(const nsIntRegion& aRegion) override;
  virtual bool RecvStartFrameTimeRecording(const int32_t& aBufferSize, uint32_t* aOutStartIndex) override;
  virtual bool RecvStopFrameTimeRecording(const uint32_t& aStartIndex, InfallibleTArray<float>* intervals) override;

  
  
  virtual bool RecvRequestNotifyAfterRemotePaint() override { return true; };

  virtual void ActorDestroy(ActorDestroyReason why) override;

  virtual void ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                   const uint64_t& aTransactionId,
                                   const TargetConfig& aTargetConfig,
                                   const InfallibleTArray<PluginWindowData>& aPlugins,
                                   bool aIsFirstPaint,
                                   bool aScheduleComposite,
                                   uint32_t aPaintSequenceNumber,
                                   bool aIsRepeatTransaction) override;
  virtual void ForceComposite(LayerTransactionParent* aLayerTree) override;
  virtual bool SetTestSampleTime(LayerTransactionParent* aLayerTree,
                                 const TimeStamp& aTime) override;
  virtual void LeaveTestMode(LayerTransactionParent* aLayerTree) override;
  virtual void ApplyAsyncProperties(LayerTransactionParent* aLayerTree)
               override;
  virtual void GetAPZTestData(const LayerTransactionParent* aLayerTree,
                              APZTestData* aOutData) override;
  virtual AsyncCompositionManager* GetCompositionManager(LayerTransactionParent* aLayerTree) override { return mCompositionManager; }

  






  void ForceIsFirstPaint();
  void Destroy();

  static void SetShadowProperties(Layer* aLayer);

  void NotifyChildCreated(const uint64_t& aChild);

  void AsyncRender();

  
  void ScheduleRenderOnCompositorThread();
  void SchedulePauseOnCompositorThread();
  



  bool ScheduleResumeOnCompositorThread();
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
    ~LayerTreeState();
    nsRefPtr<Layer> mRoot;
    nsRefPtr<GeckoContentController> mController;
    CompositorParent* mParent;
    LayerManagerComposite* mLayerManager;
    
    
    
    PCompositorParent* mCrossProcessParent;
    TargetConfig mTargetConfig;
    APZTestData mApzTestData;
    LayerTransactionParent* mLayerTree;
    nsTArray<PluginWindowData> mPluginData;
    bool mUpdatedPluginDataAvailable;
    nsRefPtr<CompositorUpdateObserver> mLayerTreeReadyObserver;
    nsRefPtr<CompositorUpdateObserver> mLayerTreeClearedObserver;
  };

  




  static LayerTreeState* GetIndirectShadowTree(uint64_t aId);

  


  static void PostInsertVsyncProfilerMarker(mozilla::TimeStamp aVsyncTimestamp);

  static void RequestNotifyLayerTreeReady(uint64_t aLayersId, CompositorUpdateObserver* aObserver);
  static void RequestNotifyLayerTreeCleared(uint64_t aLayersId, CompositorUpdateObserver* aObserver);

  float ComputeRenderIntegrity();

  


  static bool IsInCompositorThread();

protected:
  
  virtual ~CompositorParent();

  void DeferredDestroy();

  virtual PLayerTransactionParent*
    AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                                 const uint64_t& aId,
                                 TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                 bool* aSuccess) override;
  virtual bool DeallocPLayerTransactionParent(PLayerTransactionParent* aLayers) override;
  virtual void ScheduleTask(CancelableTask*, int);
  void CompositeCallback(TimeStamp aScheduleTime);
  void CompositeToTarget(gfx::DrawTarget* aTarget, const nsIntRect* aRect = nullptr);
  void ForceComposeToTarget(gfx::DrawTarget* aTarget, const nsIntRect* aRect = nullptr);

  void SetEGLSurfaceSize(int width, int height);

  void InitializeLayerManager(const nsTArray<LayersBackend>& aBackendHints);
  void PauseComposition();
  void ResumeComposition();
  void ResumeCompositionAndResize(int width, int height);
  void ForceComposition();
  void CancelCurrentCompositeTask();
  void ScheduleSoftwareTimerComposition();

  


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
  nsRefPtr<CompositorVsyncObserver> mCompositorVsyncObserver;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 
