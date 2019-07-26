





#ifndef mozilla_layers_CompositorParent_h
#define mozilla_layers_CompositorParent_h









#include <stdint.h>                     
#include "Layers.h"                     
#include "ShadowLayersManager.h"        
#include "base/basictypes.h"            
#include "base/platform_thread.h"       
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

class CompositorParent MOZ_FINAL : public PCompositorParent,
                                   public ShadowLayersManager
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositorParent)

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
                                SurfaceDescriptor* aOutSnapshot) MOZ_OVERRIDE;
  virtual bool RecvFlushRendering() MOZ_OVERRIDE;

  virtual bool RecvNotifyRegionInvalidated(const nsIntRegion& aRegion) MOZ_OVERRIDE;
  virtual bool RecvStartFrameTimeRecording(const int32_t& aBufferSize, uint32_t* aOutStartIndex) MOZ_OVERRIDE;
  virtual bool RecvStopFrameTimeRecording(const uint32_t& aStartIndex, InfallibleTArray<float>* intervals) MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  virtual void ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                   const TargetConfig& aTargetConfig,
                                   bool aIsFirstPaint,
                                   bool aScheduleComposite,
                                   uint32_t aPaintSequenceNumber) MOZ_OVERRIDE;
  virtual void ForceComposite(LayerTransactionParent* aLayerTree) MOZ_OVERRIDE;
  virtual bool SetTestSampleTime(LayerTransactionParent* aLayerTree,
                                 const TimeStamp& aTime) MOZ_OVERRIDE;
  virtual void LeaveTestMode(LayerTransactionParent* aLayerTree) MOZ_OVERRIDE;
  virtual void GetAPZTestData(const LayerTransactionParent* aLayerTree,
                              APZTestData* aOutData) MOZ_OVERRIDE;
  virtual AsyncCompositionManager* GetCompositionManager(LayerTransactionParent* aLayerTree) MOZ_OVERRIDE { return mCompositionManager; }

  






  void ForceIsFirstPaint();
  void Destroy();

  void NotifyChildCreated(uint64_t aChild);

  void AsyncRender();

  
  void ScheduleRenderOnCompositorThread();
  void SchedulePauseOnCompositorThread();
  



  bool ScheduleResumeOnCompositorThread(int width, int height);

  virtual void ScheduleComposition();
  void NotifyShadowTreeTransaction(uint64_t aId, bool aIsFirstPaint,
      bool aScheduleComposite, uint32_t aPaintSequenceNumber);

  



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

  





  static void StartUpWithExistingThread(MessageLoop* aMsgLoop,
                                        PlatformThreadId aThreadID);

  struct LayerTreeState {
    LayerTreeState();
    nsRefPtr<Layer> mRoot;
    nsRefPtr<GeckoContentController> mController;
    CompositorParent* mParent;
    LayerManagerComposite* mLayerManager;
    
    
    
    PCompositorParent* mCrossProcessParent;
    TargetConfig mTargetConfig;
    APZTestData mApzTestData;
  };

  




  static LayerTreeState* GetIndirectShadowTree(uint64_t aId);

  float ComputeRenderIntegrity();

  


  static bool IsInCompositorThread();

private:
  
  virtual ~CompositorParent();

  virtual PLayerTransactionParent*
    AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                                 const uint64_t& aId,
                                 TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                 bool* aSuccess) MOZ_OVERRIDE;
  virtual bool DeallocPLayerTransactionParent(PLayerTransactionParent* aLayers) MOZ_OVERRIDE;
  virtual void ScheduleTask(CancelableTask*, int);
  void Composite();
  void CompositeToTarget(gfx::DrawTarget* aTarget);
  void ForceComposeToTarget(gfx::DrawTarget* aTarget);

  void SetEGLSurfaceSize(int width, int height);

  void InitializeLayerManager(const nsTArray<LayersBackend>& aBackendHints);
  void PauseComposition();
  void ResumeComposition();
  void ResumeCompositionAndResize(int width, int height);
  void ForceComposition();
  void CancelCurrentCompositeTask();

  inline static PlatformThreadId CompositorThreadID();

  






  static void CreateCompositorMap();
  static void DestroyCompositorMap();

  








  static bool CreateThread();

  






  static void DestroyThread();

  


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

  bool mWantDidCompositeEvent;

  DISALLOW_EVIL_CONSTRUCTORS(CompositorParent);
};

} 
} 

#endif 
