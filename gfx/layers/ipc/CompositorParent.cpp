





#include <map>

#include "mozilla/DebugOnly.h"

#include "AsyncPanZoomController.h"
#include "AutoOpenSurface.h"
#include "CompositorParent.h"
#include "mozilla/layers/CompositorOGL.h"
#include "LayerTransactionParent.h"
#include "nsIWidget.h"
#include "nsGkAtoms.h"
#include "RenderTrace.h"
#include "gfxPlatform.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/layers/AsyncCompositionManager.h"
#include "mozilla/layers/LayerManagerComposite.h"

using namespace base;
using namespace mozilla;
using namespace mozilla::ipc;
using namespace std;

namespace mozilla {
namespace layers {




static CompositorParent* sCurrentCompositor;
static Thread* sCompositorThread = nullptr;

static int sCompositorThreadRefCount = 0;
static MessageLoop* sMainLoop = nullptr;




static PlatformThreadId sCompositorThreadID = 0;
static MessageLoop* sCompositorLoop = nullptr;

static void DeferredDeleteCompositorParent(CompositorParent* aNowReadyToDie)
{
  aNowReadyToDie->Release();
}

static void DeleteCompositorThread()
{
  if (NS_IsMainThread()){
    delete sCompositorThread;
    sCompositorThread = nullptr;
    sCompositorLoop = nullptr;
    sCompositorThreadID = 0;
  } else {
    sMainLoop->PostTask(FROM_HERE, NewRunnableFunction(&DeleteCompositorThread));
  }
}

static void ReleaseCompositorThread()
{
  if(--sCompositorThreadRefCount == 0) {
    DeleteCompositorThread();
  }
}

void
CompositorParent::StartUpWithExistingThread(MessageLoop* aMsgLoop,
                                            PlatformThreadId aThreadID)
{
  MOZ_ASSERT(!sCompositorThread);
  CreateCompositorMap();
  sCompositorLoop = aMsgLoop;
  sCompositorThreadID = aThreadID;
  sMainLoop = MessageLoop::current();
  sCompositorThreadRefCount = 1;
}

void CompositorParent::StartUp()
{
  
  if (sCompositorThreadID) {
    return;
  }
  MOZ_ASSERT(!sCompositorLoop);
  CreateCompositorMap();
  CreateThread();
  sMainLoop = MessageLoop::current();
}

void CompositorParent::ShutDown()
{
  DestroyThread();
  DestroyCompositorMap();
}

bool CompositorParent::CreateThread()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main Thread!");
  if (sCompositorThread || sCompositorLoop) {
    return true;
  }
  sCompositorThreadRefCount = 1;
  sCompositorThread = new Thread("Compositor");
  if (!sCompositorThread->Start()) {
    delete sCompositorThread;
    sCompositorThread = nullptr;
    return false;
  }
  return true;
}

void CompositorParent::DestroyThread()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main Thread!");
  ReleaseCompositorThread();
}

MessageLoop* CompositorParent::CompositorLoop()
{
  return sCompositorThread ? sCompositorThread->message_loop() : sCompositorLoop;
}

CompositorParent::CompositorParent(nsIWidget* aWidget,
                                   bool aRenderToEGLSurface,
                                   int aSurfaceWidth, int aSurfaceHeight)
  : mWidget(aWidget)
  , mCurrentCompositeTask(NULL)
  , mPaused(false)
  , mRenderToEGLSurface(aRenderToEGLSurface)
  , mEGLSurfaceSize(aSurfaceWidth, aSurfaceHeight)
  , mPauseCompositionMonitor("PauseCompositionMonitor")
  , mResumeCompositionMonitor("ResumeCompositionMonitor")
  , mOverrideComposeReadiness(false)
  , mForceCompositionTask(nullptr)
{
  NS_ABORT_IF_FALSE(sCompositorThread != nullptr || sCompositorThreadID,
                    "The compositor thread must be Initialized before instanciating a COmpositorParent.");
  MOZ_COUNT_CTOR(CompositorParent);
  mCompositorID = 0;
  
  
  
  CompositorLoop()->PostTask(FROM_HERE, NewRunnableFunction(&AddCompositor,
                                                          this, &mCompositorID));

  if (!sCurrentCompositor) {
    sCurrentCompositor = this;
  }
  ++sCompositorThreadRefCount;
}

PlatformThreadId
CompositorParent::CompositorThreadID()
{
  return sCompositorThread ? sCompositorThread->thread_id() : sCompositorThreadID;
}

CompositorParent::~CompositorParent()
{
  MOZ_COUNT_DTOR(CompositorParent);

  if (this == sCurrentCompositor) {
    sCurrentCompositor = NULL;
  }
  ReleaseCompositorThread();
}

void
CompositorParent::Destroy()
{
  NS_ABORT_IF_FALSE(ManagedPLayerTransactionParent().Length() == 0,
                    "CompositorParent destroyed before managed PLayerTransactionParent");

  
  mLayerManager = nullptr;
  mCompositionManager = nullptr;
}

void
CompositorParent::ForceIsFirstPaint()
{
  mCompositionManager->ForceIsFirstPaint();
}

bool
CompositorParent::RecvWillStop()
{
  mPaused = true;
  RemoveCompositor(mCompositorID);

  
  if (mLayerManager) {
    mLayerManager->Destroy();
    mLayerManager = nullptr;
    mCompositionManager = nullptr;
  }

  return true;
}

bool
CompositorParent::RecvStop()
{
  Destroy();
  
  
  
  
  
  this->AddRef(); 
  CompositorLoop()->PostTask(FROM_HERE,
                           NewRunnableFunction(&DeferredDeleteCompositorParent,
                                               this));
  return true;
}

bool
CompositorParent::RecvPause()
{
  PauseComposition();
  return true;
}

bool
CompositorParent::RecvResume()
{
  ResumeComposition();
  return true;
}

bool
CompositorParent::RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                   SurfaceDescriptor* aOutSnapshot)
{
  AutoOpenSurface opener(OPEN_READ_WRITE, aInSnapshot);
  nsRefPtr<gfxContext> target = new gfxContext(opener.Get());
  ComposeToTarget(target);
  *aOutSnapshot = aInSnapshot;
  return true;
}

void
CompositorParent::ActorDestroy(ActorDestroyReason why)
{
  mPaused = true;
  RemoveCompositor(mCompositorID);

  if (mLayerManager) {
    mLayerManager->Destroy();
    mLayerManager = nullptr;
    mCompositionManager = nullptr;
  }
}


void
CompositorParent::ScheduleRenderOnCompositorThread()
{
  CancelableTask *renderTask = NewRunnableMethod(this, &CompositorParent::ScheduleComposition);
  CompositorLoop()->PostTask(FROM_HERE, renderTask);
}

void
CompositorParent::PauseComposition()
{
  NS_ABORT_IF_FALSE(CompositorThreadID() == PlatformThread::CurrentId(),
                    "PauseComposition() can only be called on the compositor thread");

  MonitorAutoLock lock(mPauseCompositionMonitor);

  if (!mPaused) {
    mPaused = true;

    mLayerManager->GetCompositor()->Pause();
  }

  
  lock.NotifyAll();
}

void
CompositorParent::ResumeComposition()
{
  NS_ABORT_IF_FALSE(CompositorThreadID() == PlatformThread::CurrentId(),
                    "ResumeComposition() can only be called on the compositor thread");

  MonitorAutoLock lock(mResumeCompositionMonitor);

  if (!mLayerManager->GetCompositor()->Resume()) {
#ifdef MOZ_WIDGET_ANDROID
    
    
    __android_log_print(ANDROID_LOG_INFO, "CompositorParent", "Unable to renew compositor surface; remaining in paused state");
#endif
    lock.NotifyAll();
    return;
  }

  mPaused = false;

  Composite();

  
  lock.NotifyAll();
}

void
CompositorParent::ForceComposition()
{
  
  mForceCompositionTask = nullptr;
  ScheduleRenderOnCompositorThread();
}

void
CompositorParent::SetEGLSurfaceSize(int width, int height)
{
  NS_ASSERTION(mRenderToEGLSurface, "Compositor created without RenderToEGLSurface provided");
  mEGLSurfaceSize.SizeTo(width, height);
  if (mLayerManager) {
    mLayerManager->GetCompositor()->SetDestinationSurfaceSize(gfx::IntSize(mEGLSurfaceSize.width, mEGLSurfaceSize.height));
  }
}

void
CompositorParent::ResumeCompositionAndResize(int width, int height)
{
  SetEGLSurfaceSize(width, height);
  ResumeComposition();
}





void
CompositorParent::SchedulePauseOnCompositorThread()
{
  MonitorAutoLock lock(mPauseCompositionMonitor);

  CancelableTask *pauseTask = NewRunnableMethod(this,
                                                &CompositorParent::PauseComposition);
  CompositorLoop()->PostTask(FROM_HERE, pauseTask);

  
  lock.Wait();
}

bool
CompositorParent::ScheduleResumeOnCompositorThread(int width, int height)
{
  MonitorAutoLock lock(mResumeCompositionMonitor);

  CancelableTask *resumeTask =
    NewRunnableMethod(this, &CompositorParent::ResumeCompositionAndResize, width, height);
  CompositorLoop()->PostTask(FROM_HERE, resumeTask);

  
  lock.Wait();

  return !mPaused;
}

void
CompositorParent::ScheduleTask(CancelableTask* task, int time)
{
  if (time == 0) {
    MessageLoop::current()->PostTask(FROM_HERE, task);
  } else {
    MessageLoop::current()->PostDelayedTask(FROM_HERE, task, time);
  }
}

void
CompositorParent::NotifyShadowTreeTransaction()
{
  if (mLayerManager) {
    LayerManagerComposite* managerComposite = mLayerManager->AsLayerManagerComposite();
    if (managerComposite) {
      managerComposite->NotifyShadowTreeTransaction();
    }
  }
  ScheduleComposition();
}

void
CompositorParent::ScheduleComposition()
{
  if (mCurrentCompositeTask) {
    return;
  }

  bool initialComposition = mLastCompose.IsNull();
  TimeDuration delta;
  if (!initialComposition)
    delta = TimeStamp::Now() - mLastCompose;

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  mExpectedComposeTime = TimeStamp::Now() + TimeDuration::FromMilliseconds(15);
#endif

  mCurrentCompositeTask = NewRunnableMethod(this, &CompositorParent::Composite);

  
  
  if (!initialComposition && delta.ToMilliseconds() < 15) {
#ifdef COMPOSITOR_PERFORMANCE_WARNING
    mExpectedComposeTime = TimeStamp::Now() + TimeDuration::FromMilliseconds(15 - delta.ToMilliseconds());
#endif
    ScheduleTask(mCurrentCompositeTask, 15 - delta.ToMilliseconds());
  } else {
    ScheduleTask(mCurrentCompositeTask, 0);
  }
}

void
CompositorParent::SetTransformation(float aScale, nsIntPoint aScrollOffset)
{
  mCompositionManager->SetTransformation(aScale, aScrollOffset);
}

void
CompositorParent::Composite()
{
  NS_ABORT_IF_FALSE(CompositorThreadID() == PlatformThread::CurrentId(),
                    "Composite can only be called on the compositor thread");
  mCurrentCompositeTask = nullptr;

  mLastCompose = TimeStamp::Now();

  if (!CanComposite()) {
    return;
  }

  AutoResolveRefLayers resolve(mCompositionManager);
  if (mForceCompositionTask && !mOverrideComposeReadiness) {
    if (mCompositionManager->ReadyForCompose()) {
      mForceCompositionTask->Cancel();
      mForceCompositionTask = nullptr;
    } else {
      return;
    }
  }

  bool requestNextFrame = mCompositionManager->TransformShadowTree(mLastCompose);
  if (requestNextFrame) {
    ScheduleComposition();
  }

  RenderTraceLayers(mLayerManager->GetRoot(), "0000");

  mCompositionManager->ComputeRotation();

#ifdef MOZ_DUMP_PAINTING
  static bool gDumpCompositorTree = false;
  if (gDumpCompositorTree) {
    fprintf(stdout, "Painting --- compositing layer tree:\n");
    mLayerManager->Dump(stdout, "", false);
  }
#endif
  mLayerManager->EndEmptyTransaction();

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  if (mExpectedComposeTime + TimeDuration::FromMilliseconds(15) < TimeStamp::Now()) {
    printf_stderr("Compositor: Composite took %i ms.\n",
                  15 + (int)(TimeStamp::Now() - mExpectedComposeTime).ToMilliseconds());
  }
#endif
}

void
CompositorParent::ComposeToTarget(gfxContext* aTarget)
{
  AutoRestore<bool> override(mOverrideComposeReadiness);
  mOverrideComposeReadiness = true;

  if (!CanComposite()) {
    return;
  }
  mLayerManager->BeginTransactionWithTarget(aTarget);
  
  
  Composite();
}

bool
CompositorParent::CanComposite()
{
  return !(mPaused || !mLayerManager || !mLayerManager->GetRoot());
}



static void
SetShadowProperties(Layer* aLayer)
{
  
  LayerComposite* layerComposite = aLayer->AsLayerComposite();
  
  layerComposite->SetShadowTransform(aLayer->GetBaseTransform());
  layerComposite->SetShadowVisibleRegion(aLayer->GetVisibleRegion());
  layerComposite->SetShadowClipRect(aLayer->GetClipRect());
  layerComposite->SetShadowOpacity(aLayer->GetOpacity());

  for (Layer* child = aLayer->GetFirstChild();
      child; child = child->GetNextSibling()) {
    SetShadowProperties(child);
  }
}

void
CompositorParent::ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                      const TargetConfig& aTargetConfig,
                                      bool isFirstPaint)
{
  if (!isFirstPaint &&
      !mCompositionManager->IsFirstPaint() &&
      mCompositionManager->RequiresReorientation(aTargetConfig.orientation())) {
    if (mForceCompositionTask != NULL) {
      mForceCompositionTask->Cancel();
    }
    mForceCompositionTask = NewRunnableMethod(this, &CompositorParent::ForceComposition);
    ScheduleTask(mForceCompositionTask, gfxPlatform::GetPlatform()->GetOrientationSyncMillis());
  }

  
  
  
  mLayerManager->UpdateRenderBounds(aTargetConfig.clientBounds());

  mCompositionManager->Updated(isFirstPaint, aTargetConfig);
  Layer* root = aLayerTree->GetRoot();
  mLayerManager->SetRoot(root);
  if (root) {
    SetShadowProperties(root);
  }
  ScheduleComposition();
  LayerManagerComposite *layerComposite = mLayerManager->AsLayerManagerComposite();
  if (layerComposite) {
    layerComposite->NotifyShadowTreeTransaction();
  }
}

PLayerTransactionParent*
CompositorParent::AllocPLayerTransaction(const LayersBackend& aBackendHint,
                                         const uint64_t& aId,
                                         TextureFactoryIdentifier* aTextureFactoryIdentifier)
{
  MOZ_ASSERT(aId == 0);

  
  
  nsIntRect rect;
  mWidget->GetClientBounds(rect);

  if (aBackendHint == mozilla::layers::LAYERS_OPENGL) {
    mLayerManager =
      new LayerManagerComposite(new CompositorOGL(mWidget,
                                                  mEGLSurfaceSize.width,
                                                  mEGLSurfaceSize.height,
                                                  mRenderToEGLSurface));
    mWidget = nullptr;
    mLayerManager->SetCompositorID(mCompositorID);

    if (!mLayerManager->Initialize()) {
      NS_ERROR("Failed to init Compositor");
      return nullptr;
    }

    mCompositionManager = new AsyncCompositionManager(mLayerManager);

    *aTextureFactoryIdentifier = mLayerManager->GetTextureFactoryIdentifier();
    return new LayerTransactionParent(mLayerManager, this, 0);
  } else {
    NS_ERROR("Unsupported backend selected for Async Compositor");
    return nullptr;
  }
}

bool
CompositorParent::DeallocPLayerTransaction(PLayerTransactionParent* actor)
{
  delete actor;
  return true;
}


typedef map<uint64_t,CompositorParent*> CompositorMap;
static CompositorMap* sCompositorMap;

void CompositorParent::CreateCompositorMap()
{
  if (sCompositorMap == nullptr) {
    sCompositorMap = new CompositorMap;
  }
}

void CompositorParent::DestroyCompositorMap()
{
  if (sCompositorMap != nullptr) {
    NS_ASSERTION(sCompositorMap->empty(),
                 "The Compositor map should be empty when destroyed>");
    delete sCompositorMap;
    sCompositorMap = nullptr;
  }
}

CompositorParent* CompositorParent::GetCompositor(uint64_t id)
{
  CompositorMap::iterator it = sCompositorMap->find(id);
  return it != sCompositorMap->end() ? it->second : nullptr;
}

void CompositorParent::AddCompositor(CompositorParent* compositor, uint64_t* outID)
{
  static uint64_t sNextID = 1;

  ++sNextID;
  (*sCompositorMap)[sNextID] = compositor;
  *outID = sNextID;
}

CompositorParent* CompositorParent::RemoveCompositor(uint64_t id)
{
  CompositorMap::iterator it = sCompositorMap->find(id);
  if (it == sCompositorMap->end()) {
    return nullptr;
  }
  CompositorParent *retval = it->second;
  sCompositorMap->erase(it);
  return retval;
}

typedef map<uint64_t, CompositorParent::LayerTreeState> LayerTreeMap;
static LayerTreeMap sIndirectLayerTrees;

 uint64_t
CompositorParent::AllocateLayerTreeId()
{
  MOZ_ASSERT(CompositorLoop());
  MOZ_ASSERT(NS_IsMainThread());
  static uint64_t ids;
  return ++ids;
}

static void
EraseLayerState(uint64_t aId)
{
  sIndirectLayerTrees.erase(aId);
}

 void
CompositorParent::DeallocateLayerTreeId(uint64_t aId)
{
  MOZ_ASSERT(NS_IsMainThread());
  CompositorLoop()->PostTask(FROM_HERE,
                             NewRunnableFunction(&EraseLayerState, aId));
}

static void
UpdateControllerForLayersId(uint64_t aLayersId,
                            AsyncPanZoomController* aController)
{
  
  sIndirectLayerTrees[aLayersId].mController =
    already_AddRefed<AsyncPanZoomController>(aController);

  
  
  aController->SetCompositorParent(sCurrentCompositor);
}

 void
CompositorParent::SetPanZoomControllerForLayerTree(uint64_t aLayersId,
                                                   AsyncPanZoomController* aController)
{
  
  aController->AddRef();
  CompositorLoop()->PostTask(FROM_HERE,
                             NewRunnableFunction(&UpdateControllerForLayersId,
                                                 aLayersId,
                                                 aController));
}









class CrossProcessCompositorParent : public PCompositorParent,
                                     public ShadowLayersManager
{
  friend class CompositorParent;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CrossProcessCompositorParent)
public:
  CrossProcessCompositorParent(Transport* aTransport)
    : mTransport(aTransport)
  {}
  virtual ~CrossProcessCompositorParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  
  virtual bool RecvWillStop() MOZ_OVERRIDE { return true; }
  virtual bool RecvStop() MOZ_OVERRIDE { return true; }
  virtual bool RecvPause() MOZ_OVERRIDE { return true; }
  virtual bool RecvResume() MOZ_OVERRIDE { return true; }
  virtual bool RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                SurfaceDescriptor* aOutSnapshot)
  { return true; }

  virtual PLayerTransactionParent*
    AllocPLayerTransaction(const LayersBackend& aBackendType,
                           const uint64_t& aId,
                           TextureFactoryIdentifier* aTextureFactoryIdentifier) MOZ_OVERRIDE;

  virtual bool DeallocPLayerTransaction(PLayerTransactionParent* aLayers) MOZ_OVERRIDE;

  virtual void ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                   const TargetConfig& aTargetConfig,
                                   bool isFirstPaint) MOZ_OVERRIDE;

private:
  void DeferredDestroy();

  
  
  
  nsRefPtr<CrossProcessCompositorParent> mSelfRef;
  Transport* mTransport;
};

static void
OpenCompositor(CrossProcessCompositorParent* aCompositor,
               Transport* aTransport, ProcessHandle aHandle,
               MessageLoop* aIOLoop)
{
  DebugOnly<bool> ok = aCompositor->Open(aTransport, aHandle, aIOLoop);
  MOZ_ASSERT(ok);
}

 PCompositorParent*
CompositorParent::Create(Transport* aTransport, ProcessId aOtherProcess)
{
  nsRefPtr<CrossProcessCompositorParent> cpcp =
    new CrossProcessCompositorParent(aTransport);
  ProcessHandle handle;
  if (!base::OpenProcessHandle(aOtherProcess, &handle)) {
    
    return nullptr;
  }
  cpcp->mSelfRef = cpcp;
  CompositorLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(OpenCompositor, cpcp.get(),
                        aTransport, handle, XRE_GetIOMessageLoop()));
  
  
  return cpcp.get();
}

static void
UpdateIndirectTree(uint64_t aId, Layer* aRoot, const TargetConfig& aTargetConfig, bool isFirstPaint)
{
  sIndirectLayerTrees[aId].mRoot = aRoot;
  sIndirectLayerTrees[aId].mTargetConfig = aTargetConfig;
  if (ContainerLayer* root = aRoot->AsContainerLayer()) {
    if (AsyncPanZoomController* apzc = sIndirectLayerTrees[aId].mController) {
      apzc->NotifyLayersUpdated(root->GetFrameMetrics(), isFirstPaint);
    }
  }
}

 const CompositorParent::LayerTreeState*
CompositorParent::GetIndirectShadowTree(uint64_t aId)
{
  LayerTreeMap::const_iterator cit = sIndirectLayerTrees.find(aId);
  if (sIndirectLayerTrees.end() == cit) {
    return nullptr;
  }
  return &cit->second;
}

static void
RemoveIndirectTree(uint64_t aId)
{
  sIndirectLayerTrees.erase(aId);
}

void
CrossProcessCompositorParent::ActorDestroy(ActorDestroyReason aWhy)
{
  MessageLoop::current()->PostTask(
    FROM_HERE,
    NewRunnableMethod(this, &CrossProcessCompositorParent::DeferredDestroy));
}

PLayerTransactionParent*
CrossProcessCompositorParent::AllocPLayerTransaction(const LayersBackend& aBackendType,
                                                     const uint64_t& aId,
                                                     TextureFactoryIdentifier* aTextureFactoryIdentifier)
{
  MOZ_ASSERT(aId != 0);

  nsRefPtr<LayerManager> lm = sCurrentCompositor->GetLayerManager();
  *aTextureFactoryIdentifier = lm->GetTextureFactoryIdentifier();
  return new LayerTransactionParent(lm->AsLayerManagerComposite(), this, aId);
}

bool
CrossProcessCompositorParent::DeallocPLayerTransaction(PLayerTransactionParent* aLayers)
{
  LayerTransactionParent* slp = static_cast<LayerTransactionParent*>(aLayers);
  RemoveIndirectTree(slp->GetId());
  delete aLayers;
  return true;
}

void
CrossProcessCompositorParent::ShadowLayersUpdated(
  LayerTransactionParent* aLayerTree,
  const TargetConfig& aTargetConfig,
  bool isFirstPaint)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);
  Layer* shadowRoot = aLayerTree->GetRoot();
  if (shadowRoot) {
    SetShadowProperties(shadowRoot);
  }
  UpdateIndirectTree(id, shadowRoot, aTargetConfig, isFirstPaint);

  sCurrentCompositor->NotifyShadowTreeTransaction();
}

void
CrossProcessCompositorParent::DeferredDestroy()
{
  mSelfRef = nullptr;
  
}

CrossProcessCompositorParent::~CrossProcessCompositorParent()
{
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new DeleteTask<Transport>(mTransport));
}

} 
} 
