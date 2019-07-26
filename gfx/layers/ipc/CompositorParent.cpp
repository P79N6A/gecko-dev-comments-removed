





#include "CompositorParent.h"
#include <stdio.h>                      
#include <stdint.h>                     
#include <map>                          
#include <utility>                      
#include "AutoOpenSurface.h"            
#include "LayerTransactionParent.h"     
#include "RenderTrace.h"                
#include "base/message_loop.h"          
#include "base/process.h"               
#include "base/process_util.h"          
#include "base/task.h"                  
#include "base/thread.h"                
#include "base/tracked.h"               
#include "gfxContext.h"                 
#include "gfxPlatform.h"                
#include "ipc/ShadowLayersManager.h"    
#include "mozilla/AutoRestore.h"        
#include "mozilla/DebugOnly.h"          
#include "mozilla/gfx/2D.h"          
#include "mozilla/gfx/Point.h"          
#include "mozilla/ipc/Transport.h"      
#include "mozilla/layers/APZCTreeManager.h"  
#include "mozilla/layers/AsyncCompositionManager.h"
#include "mozilla/layers/BasicCompositor.h"  
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorOGL.h"  
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/layers/PLayerTransactionParent.h"
#include "mozilla/mozalloc.h"           
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsIWidget.h"                  
#include "nsRect.h"                     
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "nsTraceRefcnt.h"              
#include "nsXULAppAPI.h"                
#ifdef XP_WIN
#include "mozilla/layers/CompositorD3D11.h"
#include "mozilla/layers/CompositorD3D9.h"
#endif
#include "GeckoProfiler.h"
#include "mozilla/ipc/ProtocolTypes.h"

using namespace base;
using namespace mozilla;
using namespace mozilla::ipc;
using namespace mozilla::gfx;
using namespace std;

namespace mozilla {
namespace layers {

CompositorParent::LayerTreeState::LayerTreeState()
  : mParent(nullptr)
{
}

typedef map<uint64_t, CompositorParent::LayerTreeState> LayerTreeMap;
static LayerTreeMap sIndirectLayerTrees;




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
                                   bool aUseExternalSurfaceSize,
                                   int aSurfaceWidth, int aSurfaceHeight)
  : mWidget(aWidget)
  , mCurrentCompositeTask(nullptr)
  , mIsTesting(false)
  , mPaused(false)
  , mUseExternalSurfaceSize(aUseExternalSurfaceSize)
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

  mRootLayerTreeID = AllocateLayerTreeId();
  sIndirectLayerTrees[mRootLayerTreeID].mParent = this;

  mApzcTreeManager = new APZCTreeManager();
  ++sCompositorThreadRefCount;
}

PlatformThreadId
CompositorParent::CompositorThreadID()
{
  return sCompositorThread ? sCompositorThread->thread_id() : sCompositorThreadID;
}

bool
CompositorParent::IsInCompositorThread()
{
  return CompositorThreadID() == PlatformThread::CurrentId();
}

uint64_t
CompositorParent::RootLayerTreeId()
{
  return mRootLayerTreeID;
}

CompositorParent::~CompositorParent()
{
  MOZ_COUNT_DTOR(CompositorParent);

  ReleaseCompositorThread();
}

void
CompositorParent::Destroy()
{
  NS_ABORT_IF_FALSE(ManagedPLayerTransactionParent().Length() == 0,
                    "CompositorParent destroyed before managed PLayerTransactionParent");

  
  mLayerManager = nullptr;
  mCompositionManager = nullptr;
  mApzcTreeManager->ClearTree();
  mApzcTreeManager = nullptr;
  sIndirectLayerTrees.erase(mRootLayerTreeID);
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
    for (LayerTreeMap::iterator it = sIndirectLayerTrees.begin();
         it != sIndirectLayerTrees.end(); it++)
    {
      LayerTreeState* lts = &it->second;
      if (lts->mParent == this) {
        mLayerManager->ClearCachedResources(lts->mRoot);
      }
    }
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
  gfxIntSize size = opener.Size();
  
  
  RefPtr<DrawTarget> target =
    gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(opener.Get(),
                                                           IntSize(size.width,
                                                                   size.height));
  ComposeToTarget(target);
  *aOutSnapshot = aInSnapshot;
  return true;
}

bool
CompositorParent::RecvFlushRendering()
{
  
  
  if (mCurrentCompositeTask) {
    mCurrentCompositeTask->Cancel();
    ComposeToTarget(nullptr);
  }
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
  NS_ASSERTION(mUseExternalSurfaceSize, "Compositor created without UseExternalSurfaceSize provided");
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
CompositorParent::NotifyShadowTreeTransaction(uint64_t aId, bool aIsFirstPaint)
{
  if (mApzcTreeManager &&
      mLayerManager &&
      mLayerManager->GetRoot()) {
    AutoResolveRefLayers resolve(mCompositionManager);
    mApzcTreeManager->UpdatePanZoomControllerTree(this, mLayerManager->GetRoot(), aIsFirstPaint, aId);

    mLayerManager->AsLayerManagerComposite()->NotifyShadowTreeTransaction();
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
CompositorParent::Composite()
{
  if (CanComposite()) {
    mLayerManager->BeginTransaction();
  }
  CompositeInTransaction();
}

void
CompositorParent::CompositeInTransaction()
{
  profiler_tracing("Paint", "Composite", TRACING_INTERVAL_START);
  PROFILER_LABEL("CompositorParent", "Composite");
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

  TimeStamp time = mIsTesting ? mTestTime : mLastCompose;
  bool requestNextFrame = mCompositionManager->TransformShadowTree(time);
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
  profiler_tracing("Paint", "Composite", TRACING_INTERVAL_END);
}

void
CompositorParent::ComposeToTarget(DrawTarget* aTarget)
{
  PROFILER_LABEL("CompositorParent", "ComposeToTarget");
  AutoRestore<bool> override(mOverrideComposeReadiness);
  mOverrideComposeReadiness = true;

  if (!CanComposite()) {
    return;
  }
  mLayerManager->BeginTransactionWithDrawTarget(aTarget);
  
  
  CompositeInTransaction();
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
  layerComposite->SetShadowTransformSetByAnimation(false);
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
    if (mForceCompositionTask != nullptr) {
      mForceCompositionTask->Cancel();
    }
    mForceCompositionTask = NewRunnableMethod(this, &CompositorParent::ForceComposition);
    ScheduleTask(mForceCompositionTask, gfxPlatform::GetPlatform()->GetOrientationSyncMillis());
  }

  
  
  
  mLayerManager->UpdateRenderBounds(aTargetConfig.clientBounds());

  mCompositionManager->Updated(isFirstPaint, aTargetConfig);
  Layer* root = aLayerTree->GetRoot();
  mLayerManager->SetRoot(root);

  if (mApzcTreeManager) {
    AutoResolveRefLayers resolve(mCompositionManager);
    mApzcTreeManager->UpdatePanZoomControllerTree(this, root, isFirstPaint, mRootLayerTreeID);
  }

  if (root) {
    SetShadowProperties(root);
    if (mIsTesting) {
      mCompositionManager->TransformShadowTree(mTestTime);
    }
  }
  ScheduleComposition();
  mLayerManager->NotifyShadowTreeTransaction();
}

void
CompositorParent::InitializeLayerManager(const nsTArray<LayersBackend>& aBackendHints)
{
  NS_ASSERTION(!mLayerManager, "Already initialised mLayerManager");

  for (size_t i = 0; i < aBackendHints.Length(); ++i) {
    RefPtr<LayerManagerComposite> layerManager;
    if (aBackendHints[i] == LAYERS_OPENGL) {
      layerManager =
        new LayerManagerComposite(new CompositorOGL(mWidget,
                                                    mEGLSurfaceSize.width,
                                                    mEGLSurfaceSize.height,
                                                    mUseExternalSurfaceSize));
    } else if (aBackendHints[i] == LAYERS_BASIC) {
      layerManager =
        new LayerManagerComposite(new BasicCompositor(mWidget));
#ifdef XP_WIN
    } else if (aBackendHints[i] == LAYERS_D3D11) {
      layerManager =
        new LayerManagerComposite(new CompositorD3D11(mWidget));
    } else if (aBackendHints[i] == LAYERS_D3D9) {
      layerManager =
        new LayerManagerComposite(new CompositorD3D9(mWidget));
#endif
    }

    if (!layerManager) {
      continue;
    }

    layerManager->SetCompositorID(mCompositorID);

    if (layerManager->Initialize()) {
      mLayerManager = layerManager;
      return;
    }
  }
}

PLayerTransactionParent*
CompositorParent::AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                                               const uint64_t& aId,
                                               TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                               bool *aSuccess)
{
  MOZ_ASSERT(aId == 0);

  
  
  nsIntRect rect;
  mWidget->GetClientBounds(rect);
  InitializeLayerManager(aBackendHints);
  mWidget = nullptr;

  if (!mLayerManager) {
    NS_WARNING("Failed to initialise Compositor");
    *aSuccess = false;
    return new LayerTransactionParent(nullptr, this, 0);
  }

  mCompositionManager = new AsyncCompositionManager(mLayerManager);

  *aTextureFactoryIdentifier = mLayerManager->GetTextureFactoryIdentifier();
  *aSuccess = true;
  return new LayerTransactionParent(mLayerManager, this, 0);
}

bool
CompositorParent::DeallocPLayerTransactionParent(PLayerTransactionParent* actor)
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

 void
CompositorParent::SetTimeAndSampleAnimations(TimeStamp aTime, bool aIsTesting)
{
  if (!sCompositorMap) {
    return;
  }
  for (CompositorMap::iterator it = sCompositorMap->begin(); it != sCompositorMap->end(); ++it) {
    it->second->mIsTesting = aIsTesting;
    it->second->mTestTime = aTime;
    if (it->second->mCompositionManager) {
      it->second->mCompositionManager->TransformShadowTree(aTime);
    }
  }
}

bool
CompositorParent::RecvNotifyChildCreated(const uint64_t& child)
{
  NotifyChildCreated(child);
  return true;
}

void
CompositorParent::NotifyChildCreated(uint64_t aChild)
{
  sIndirectLayerTrees[aChild].mParent = this;
}

 uint64_t
CompositorParent::AllocateLayerTreeId()
{
  MOZ_ASSERT(CompositorLoop());
  MOZ_ASSERT(NS_IsMainThread());
  static uint64_t ids = 0;
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
                            GeckoContentController* aController)
{
  
  sIndirectLayerTrees[aLayersId].mController =
    already_AddRefed<GeckoContentController>(aController);
}

ScopedLayerTreeRegistration::ScopedLayerTreeRegistration(uint64_t aLayersId,
                                                         Layer* aRoot,
                                                         GeckoContentController* aController)
    : mLayersId(aLayersId)
{
  sIndirectLayerTrees[aLayersId].mRoot = aRoot;
  sIndirectLayerTrees[aLayersId].mController = aController;
}

ScopedLayerTreeRegistration::~ScopedLayerTreeRegistration()
{
  sIndirectLayerTrees.erase(mLayersId);
}

 void
CompositorParent::SetControllerForLayerTree(uint64_t aLayersId,
                                            GeckoContentController* aController)
{
  
  aController->AddRef();
  CompositorLoop()->PostTask(FROM_HERE,
                             NewRunnableFunction(&UpdateControllerForLayersId,
                                                 aLayersId,
                                                 aController));
}

 APZCTreeManager*
CompositorParent::GetAPZCTreeManager(uint64_t aLayersId)
{
  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(aLayersId);
  if (state && state->mParent) {
    return state->mParent->mApzcTreeManager;
  }
  return nullptr;
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

  
  virtual IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<mozilla::ipc::ProtocolFdMapping>& aFds,
                base::ProcessHandle aPeerProcess,
                mozilla::ipc::ProtocolCloneContext* aCtx) MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  
  virtual bool RecvWillStop() MOZ_OVERRIDE { return true; }
  virtual bool RecvStop() MOZ_OVERRIDE { return true; }
  virtual bool RecvPause() MOZ_OVERRIDE { return true; }
  virtual bool RecvResume() MOZ_OVERRIDE { return true; }
  virtual bool RecvNotifyChildCreated(const uint64_t& child) MOZ_OVERRIDE;
  virtual bool RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                SurfaceDescriptor* aOutSnapshot)
  { return true; }
  virtual bool RecvFlushRendering() MOZ_OVERRIDE { return true; }

  virtual PLayerTransactionParent*
    AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                                 const uint64_t& aId,
                                 TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                 bool *aSuccess) MOZ_OVERRIDE;

  virtual bool DeallocPLayerTransactionParent(PLayerTransactionParent* aLayers) MOZ_OVERRIDE;

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

IToplevelProtocol*
CompositorParent::CloneToplevel(const InfallibleTArray<mozilla::ipc::ProtocolFdMapping>& aFds,
                                base::ProcessHandle aPeerProcess,
                                mozilla::ipc::ProtocolCloneContext* aCtx)
{
  for (unsigned int i = 0; i < aFds.Length(); i++) {
    if (aFds[i].protocolId() == (unsigned)GetProtocolId()) {
      Transport* transport = OpenDescriptor(aFds[i].fd(),
                                            Transport::MODE_SERVER);
      PCompositorParent* compositor = Create(transport, base::GetProcId(aPeerProcess));
      compositor->CloneManagees(this, aCtx);
      compositor->IToplevelProtocol::SetTransport(transport);
      return compositor;
    }
  }
  return nullptr;
}

static void
UpdateIndirectTree(uint64_t aId, Layer* aRoot, const TargetConfig& aTargetConfig)
{
  sIndirectLayerTrees[aId].mRoot = aRoot;
  sIndirectLayerTrees[aId].mTargetConfig = aTargetConfig;
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
CrossProcessCompositorParent::AllocPLayerTransactionParent(const nsTArray<LayersBackend>&,
                                                           const uint64_t& aId,
                                                           TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                                           bool *aSuccess)
{
  MOZ_ASSERT(aId != 0);

  if (sIndirectLayerTrees[aId].mParent) {
    LayerManagerComposite* lm = sIndirectLayerTrees[aId].mParent->GetLayerManager();
    *aTextureFactoryIdentifier = lm->GetTextureFactoryIdentifier();
    *aSuccess = true;
    return new LayerTransactionParent(lm, this, aId);
  }

  NS_WARNING("Created child without a matching parent?");
  
  
  *aSuccess = true;
  return new LayerTransactionParent(nullptr, this, aId);
}

bool
CrossProcessCompositorParent::DeallocPLayerTransactionParent(PLayerTransactionParent* aLayers)
{
  LayerTransactionParent* slp = static_cast<LayerTransactionParent*>(aLayers);
  RemoveIndirectTree(slp->GetId());
  delete aLayers;
  return true;
}

bool
CrossProcessCompositorParent::RecvNotifyChildCreated(const uint64_t& child)
{
  sIndirectLayerTrees[child].mParent->NotifyChildCreated(child);
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
  UpdateIndirectTree(id, shadowRoot, aTargetConfig);

  sIndirectLayerTrees[id].mParent->NotifyShadowTreeTransaction(id, isFirstPaint);
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

IToplevelProtocol*
CrossProcessCompositorParent::CloneToplevel(const InfallibleTArray<mozilla::ipc::ProtocolFdMapping>& aFds,
                                            base::ProcessHandle aPeerProcess,
                                            mozilla::ipc::ProtocolCloneContext* aCtx)
{
  for (unsigned int i = 0; i < aFds.Length(); i++) {
    if (aFds[i].protocolId() == (unsigned)GetProtocolId()) {
      Transport* transport = OpenDescriptor(aFds[i].fd(),
                                            Transport::MODE_SERVER);
      PCompositorParent* compositor =
        CompositorParent::Create(transport, base::GetProcId(aPeerProcess));
      compositor->CloneManagees(this, aCtx);
      compositor->IToplevelProtocol::SetTransport(transport);
      return compositor;
    }
  }
  return nullptr;
}

} 
} 
