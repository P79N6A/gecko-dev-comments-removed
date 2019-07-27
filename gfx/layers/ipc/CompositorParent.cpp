





#include "mozilla/layers/CompositorParent.h"
#include <stdio.h>                      
#include <stdint.h>                     
#include <map>                          
#include <utility>                      
#include "LayerTransactionParent.h"     
#include "RenderTrace.h"                
#include "base/message_loop.h"          
#include "base/process.h"               
#include "base/task.h"                  
#include "base/thread.h"                
#include "base/tracked.h"               
#include "gfxContext.h"                 
#include "gfxPlatform.h"                
#ifdef MOZ_WIDGET_GTK
#include "gfxPlatformGtk.h"             
#endif
#include "gfxPrefs.h"                   
#include "mozilla/AutoRestore.h"        
#include "mozilla/ClearOnShutdown.h"    
#include "mozilla/DebugOnly.h"          
#include "mozilla/gfx/2D.h"          
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"          
#include "mozilla/ipc/Transport.h"      
#include "mozilla/layers/APZCTreeManager.h"  
#include "mozilla/layers/APZThreadUtils.h"  
#include "mozilla/layers/AsyncCompositionManager.h"
#include "mozilla/layers/BasicCompositor.h"  
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/CompositorLRU.h"  
#include "mozilla/layers/CompositorOGL.h"  
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/FrameUniformityData.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/layers/PLayerTransactionParent.h"
#include "mozilla/layers/ShadowLayersManager.h" 
#include "mozilla/mozalloc.h"           
#include "mozilla/Telemetry.h"
#ifdef MOZ_WIDGET_GTK
#include "basic/X11BasicCompositor.h" 
#endif
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsIWidget.h"                  
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "nsXULAppAPI.h"                
#ifdef XP_WIN
#include "mozilla/layers/CompositorD3D11.h"
#include "mozilla/layers/CompositorD3D9.h"
#endif
#include "GeckoProfiler.h"
#include "mozilla/ipc/ProtocolTypes.h"
#include "mozilla/unused.h"
#include "mozilla/Hal.h"
#include "mozilla/HalTypes.h"
#include "mozilla/StaticPtr.h"
#ifdef MOZ_ENABLE_PROFILER_SPS
#include "ProfilerMarkers.h"
#endif
#include "mozilla/VsyncDispatcher.h"

#ifdef MOZ_WIDGET_GONK
#include "GeckoTouchDispatcher.h"
#endif

namespace mozilla {
namespace layers {

using namespace mozilla::ipc;
using namespace mozilla::gfx;
using namespace std;

using base::ProcessId;
using base::Thread;

CompositorParent::LayerTreeState::LayerTreeState()
  : mParent(nullptr)
  , mLayerManager(nullptr)
  , mCrossProcessParent(nullptr)
  , mLayerTree(nullptr)
  , mUpdatedPluginDataAvailable(false)
{
}

CompositorParent::LayerTreeState::~LayerTreeState()
{
  if (mController) {
    mController->Destroy();
  }
}

typedef map<uint64_t, CompositorParent::LayerTreeState> LayerTreeMap;
static LayerTreeMap sIndirectLayerTrees;
static StaticAutoPtr<mozilla::Monitor> sIndirectLayerTreesLock;

static void EnsureLayerTreeMapReady()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!sIndirectLayerTreesLock) {
    sIndirectLayerTreesLock = new Monitor("IndirectLayerTree");
    mozilla::ClearOnShutdown(&sIndirectLayerTreesLock);
  }
}








typedef map<uint64_t,CompositorParent*> CompositorMap;
static CompositorMap* sCompositorMap;

static void CreateCompositorMap()
{
  MOZ_ASSERT(!sCompositorMap);
  sCompositorMap = new CompositorMap;
}

static void DestroyCompositorMap()
{
  MOZ_ASSERT(sCompositorMap);
  MOZ_ASSERT(sCompositorMap->empty());
  delete sCompositorMap;
  sCompositorMap = nullptr;
}


void ReleaseImageBridgeParentSingleton();

CompositorThreadHolder::CompositorThreadHolder()
  : mCompositorThread(CreateCompositorThread())
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(CompositorThreadHolder);
}

CompositorThreadHolder::~CompositorThreadHolder()
{
  MOZ_ASSERT(NS_IsMainThread());

  MOZ_COUNT_DTOR(CompositorThreadHolder);

  DestroyCompositorThread(mCompositorThread);
}

static StaticRefPtr<CompositorThreadHolder> sCompositorThreadHolder;
static bool sFinishedCompositorShutDown = false;

CompositorThreadHolder* GetCompositorThreadHolder()
{
  return sCompositorThreadHolder;
}

 Thread*
CompositorThreadHolder::CreateCompositorThread()
{
  MOZ_ASSERT(NS_IsMainThread());

  MOZ_ASSERT(!sCompositorThreadHolder, "The compositor thread has already been started!");

  Thread* compositorThread = new Thread("Compositor");

  Thread::Options options;
  


  options.transient_hang_timeout = 128; 
  


  options.permanent_hang_timeout = 2048; 
#if defined(_WIN32)
  


  options.message_loop_type = MessageLoop::TYPE_UI;
#endif

  if (!compositorThread->StartWithOptions(options)) {
    delete compositorThread;
    return nullptr;
  }

  EnsureLayerTreeMapReady();
  CreateCompositorMap();

  return compositorThread;
}

 void
CompositorThreadHolder::DestroyCompositorThread(Thread* aCompositorThread)
{
  MOZ_ASSERT(NS_IsMainThread());

  MOZ_ASSERT(!sCompositorThreadHolder, "We shouldn't be destroying the compositor thread yet.");

  DestroyCompositorMap();
  delete aCompositorThread;
  sFinishedCompositorShutDown = true;
}

static Thread* CompositorThread() {
  return sCompositorThreadHolder ? sCompositorThreadHolder->GetCompositorThread() : nullptr;
}

static void SetThreadPriority()
{
  hal::SetCurrentThreadPriority(hal::THREAD_PRIORITY_COMPOSITOR);
}

CompositorScheduler::CompositorScheduler(CompositorParent* aCompositorParent)
  : mCompositorParent(aCompositorParent)
  , mCurrentCompositeTask(nullptr)
{
}

CompositorScheduler::~CompositorScheduler()
{
  MOZ_ASSERT(!mCompositorParent);
}

void
CompositorScheduler::CancelCurrentCompositeTask()
{
  if (mCurrentCompositeTask) {
    mCurrentCompositeTask->Cancel();
    mCurrentCompositeTask = nullptr;
  }
}

void
CompositorScheduler::ScheduleTask(CancelableTask* aTask, int aTime)
{
  MOZ_ASSERT(CompositorParent::CompositorLoop());
  MOZ_ASSERT(aTime >= 0);
  CompositorParent::CompositorLoop()->PostDelayedTask(FROM_HERE, aTask, aTime);
}

void
CompositorScheduler::ResumeComposition()
{
  mLastCompose = TimeStamp::Now();
  ComposeToTarget(nullptr);
}

void
CompositorScheduler::ForceComposeToTarget(gfx::DrawTarget* aTarget, const IntRect* aRect)
{
  mLastCompose = TimeStamp::Now();
  ComposeToTarget(aTarget, aRect);
}

void
CompositorScheduler::ComposeToTarget(gfx::DrawTarget* aTarget, const IntRect* aRect)
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  MOZ_ASSERT(mCompositorParent);
  mCompositorParent->CompositeToTarget(aTarget, aRect);
}

void
CompositorScheduler::Destroy()
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  CancelCurrentCompositeTask();
  mCompositorParent = nullptr;
}

CompositorSoftwareTimerScheduler::CompositorSoftwareTimerScheduler(CompositorParent* aCompositorParent)
  : CompositorScheduler(aCompositorParent)
{
}

CompositorSoftwareTimerScheduler::~CompositorSoftwareTimerScheduler()
{
  MOZ_ASSERT(!mCurrentCompositeTask);
}



static const int32_t kDefaultFrameRate = 60;

static int32_t
CalculateCompositionFrameRate()
{
  int32_t compositionFrameRatePref = gfxPrefs::LayersCompositionFrameRate();
  if (compositionFrameRatePref < 0) {
    
    int32_t layoutFrameRatePref = gfxPrefs::LayoutFrameRate();
    if (layoutFrameRatePref < 0) {
      
      
      return kDefaultFrameRate;
    }
    return layoutFrameRatePref;
  }
  return compositionFrameRatePref;
}

void
CompositorSoftwareTimerScheduler::ScheduleComposition()
{
  if (mCurrentCompositeTask) {
    return;
  }

  bool initialComposition = mLastCompose.IsNull();
  TimeDuration delta;
  if (!initialComposition) {
    delta = TimeStamp::Now() - mLastCompose;
  }

  int32_t rate = CalculateCompositionFrameRate();

  
  TimeDuration minFrameDelta = TimeDuration::FromMilliseconds(
    rate == 0 ? 0.0 : std::max(0.0, 1000.0 / rate));

  mCurrentCompositeTask = NewRunnableMethod(this,
                                            &CompositorSoftwareTimerScheduler::CallComposite);

  if (!initialComposition && delta < minFrameDelta) {
    TimeDuration delay = minFrameDelta - delta;
#ifdef COMPOSITOR_PERFORMANCE_WARNING
    mExpectedComposeStartTime = TimeStamp::Now() + delay;
#endif
    ScheduleTask(mCurrentCompositeTask, delay.ToMilliseconds());
  } else {
#ifdef COMPOSITOR_PERFORMANCE_WARNING
    mExpectedComposeStartTime = TimeStamp::Now();
#endif
    ScheduleTask(mCurrentCompositeTask, 0);
  }
}

bool
CompositorSoftwareTimerScheduler::NeedsComposite()
{
  return mCurrentCompositeTask ? true : false;
}

void
CompositorSoftwareTimerScheduler::CallComposite()
{
  Composite(TimeStamp::Now());
}

void
CompositorSoftwareTimerScheduler::Composite(TimeStamp aTimestamp)
{
  mCurrentCompositeTask = nullptr;
  mLastCompose = aTimestamp;
  ComposeToTarget(nullptr);
}

CompositorVsyncScheduler::Observer::Observer(CompositorVsyncScheduler* aOwner)
  : mMutex("CompositorVsyncScheduler.Observer.Mutex")
  , mOwner(aOwner)
{
}

CompositorVsyncScheduler::Observer::~Observer()
{
  MOZ_ASSERT(!mOwner);
}

bool
CompositorVsyncScheduler::Observer::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  MutexAutoLock lock(mMutex);
  if (!mOwner) {
    return false;
  }
  return mOwner->NotifyVsync(aVsyncTimestamp);
}

void
CompositorVsyncScheduler::Observer::Destroy()
{
  MutexAutoLock lock(mMutex);
  mOwner = nullptr;
}

CompositorVsyncScheduler::CompositorVsyncScheduler(CompositorParent* aCompositorParent, nsIWidget* aWidget)
  : CompositorScheduler(aCompositorParent)
  , mNeedsComposite(false)
  , mIsObservingVsync(false)
  , mVsyncNotificationsSkipped(0)
  , mCompositorParent(aCompositorParent)
  , mCurrentCompositeTaskMonitor("CurrentCompositeTaskMonitor")
  , mSetNeedsCompositeMonitor("SetNeedsCompositeMonitor")
  , mSetNeedsCompositeTask(nullptr)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWidget != nullptr);
  mVsyncObserver = new Observer(this);
  mCompositorVsyncDispatcher = aWidget->GetCompositorVsyncDispatcher();
#ifdef MOZ_WIDGET_GONK
  GeckoTouchDispatcher::GetInstance()->SetCompositorVsyncScheduler(this);
#endif
}

CompositorVsyncScheduler::~CompositorVsyncScheduler()
{
  MOZ_ASSERT(!mIsObservingVsync);
  MOZ_ASSERT(!mVsyncObserver);
  
  mCompositorParent = nullptr;
  mCompositorVsyncDispatcher = nullptr;
}

void
CompositorVsyncScheduler::Destroy()
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  UnobserveVsync();
  mVsyncObserver->Destroy();
  mVsyncObserver = nullptr;
  CancelCurrentSetNeedsCompositeTask();
  CompositorScheduler::Destroy();
}

void
CompositorVsyncScheduler::ScheduleComposition()
{
  SetNeedsComposite(true);
}

void
CompositorVsyncScheduler::CancelCurrentSetNeedsCompositeTask()
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  MonitorAutoLock lock(mSetNeedsCompositeMonitor);
  if (mSetNeedsCompositeTask) {
    mSetNeedsCompositeTask->Cancel();
    mSetNeedsCompositeTask = nullptr;
  }
  mNeedsComposite = false;
}








void
CompositorVsyncScheduler::SetNeedsComposite(bool aNeedsComposite)
{
  if (!CompositorParent::IsInCompositorThread()) {
    MonitorAutoLock lock(mSetNeedsCompositeMonitor);
    mSetNeedsCompositeTask = NewRunnableMethod(this,
                                              &CompositorVsyncScheduler::SetNeedsComposite,
                                              aNeedsComposite);
    ScheduleTask(mSetNeedsCompositeTask, 0);
    return;
  } else {
    MonitorAutoLock lock(mSetNeedsCompositeMonitor);
    mSetNeedsCompositeTask = nullptr;
  }

  mNeedsComposite = aNeedsComposite;
  if (!mIsObservingVsync && mNeedsComposite) {
    ObserveVsync();
  }
}

bool
CompositorVsyncScheduler::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  
  MOZ_ASSERT(!CompositorParent::IsInCompositorThread());
  MOZ_ASSERT(!NS_IsMainThread());

  MonitorAutoLock lock(mCurrentCompositeTaskMonitor);
  if (mCurrentCompositeTask == nullptr) {
    mCurrentCompositeTask = NewRunnableMethod(this,
                                              &CompositorVsyncScheduler::Composite,
                                              aVsyncTimestamp);
    ScheduleTask(mCurrentCompositeTask, 0);
  }
  return true;
}

void
CompositorVsyncScheduler::CancelCurrentCompositeTask()
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread() || NS_IsMainThread());
  MonitorAutoLock lock(mCurrentCompositeTaskMonitor);
  CompositorScheduler::CancelCurrentCompositeTask();
}

void
CompositorVsyncScheduler::Composite(TimeStamp aVsyncTimestamp)
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  {
    MonitorAutoLock lock(mCurrentCompositeTaskMonitor);
    mCurrentCompositeTask = nullptr;
  }

  DispatchTouchEvents(aVsyncTimestamp);

  if (mNeedsComposite) {
    mNeedsComposite = false;
    mLastCompose = aVsyncTimestamp;
    ComposeToTarget(nullptr);
    mVsyncNotificationsSkipped = 0;
  } else if (mVsyncNotificationsSkipped++ > gfxPrefs::CompositorUnobserveCount()) {
    UnobserveVsync();
  }
}

void
CompositorVsyncScheduler::OnForceComposeToTarget()
{
  









  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  mVsyncNotificationsSkipped = 0;
}

void
CompositorVsyncScheduler::ForceComposeToTarget(gfx::DrawTarget* aTarget, const IntRect* aRect)
{
  OnForceComposeToTarget();
  CompositorScheduler::ForceComposeToTarget(aTarget, aRect);
}

bool
CompositorVsyncScheduler::NeedsComposite()
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  return mNeedsComposite;
}

void
CompositorVsyncScheduler::ObserveVsync()
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  mCompositorVsyncDispatcher->SetCompositorVsyncObserver(mVsyncObserver);
  mIsObservingVsync = true;
}

void
CompositorVsyncScheduler::UnobserveVsync()
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  mCompositorVsyncDispatcher->SetCompositorVsyncObserver(nullptr);
  mIsObservingVsync = false;
}

void
CompositorVsyncScheduler::DispatchTouchEvents(TimeStamp aVsyncTimestamp)
{
#ifdef MOZ_WIDGET_GONK
  GeckoTouchDispatcher::GetInstance()->NotifyVsync(aVsyncTimestamp);
#endif
}

void CompositorParent::StartUp()
{
  MOZ_ASSERT(NS_IsMainThread(), "Should be on the main Thread!");
  MOZ_ASSERT(!sCompositorThreadHolder, "The compositor thread has already been started!");

  sCompositorThreadHolder = new CompositorThreadHolder();
}

void CompositorParent::ShutDown()
{
  MOZ_ASSERT(NS_IsMainThread(), "Should be on the main Thread!");
  MOZ_ASSERT(sCompositorThreadHolder, "The compositor thread has already been shut down!");

  ReleaseImageBridgeParentSingleton();

  sCompositorThreadHolder = nullptr;

  
  
  while (!sFinishedCompositorShutDown) {
    NS_ProcessNextEvent(nullptr, true);
  }
}

MessageLoop* CompositorParent::CompositorLoop()
{
  return CompositorThread() ? CompositorThread()->message_loop() : nullptr;
}

static bool
IsInCompositorAsapMode()
{
  
  
  return gfxPrefs::LayersCompositionFrameRate() == 0 &&
            !gfxPlatform::IsInLayoutAsapMode();
}

static bool
UseVsyncComposition()
{
  return gfxPrefs::VsyncAlignedCompositor()
          && gfxPrefs::HardwareVsyncEnabled()
          && !IsInCompositorAsapMode()
          && !gfxPlatform::IsInLayoutAsapMode();
}

CompositorParent::CompositorParent(nsIWidget* aWidget,
                                   bool aUseExternalSurfaceSize,
                                   int aSurfaceWidth, int aSurfaceHeight)
  : mWidget(aWidget)
  , mIsTesting(false)
  , mPendingTransaction(0)
  , mPaused(false)
  , mUseExternalSurfaceSize(aUseExternalSurfaceSize)
  , mEGLSurfaceSize(aSurfaceWidth, aSurfaceHeight)
  , mPauseCompositionMonitor("PauseCompositionMonitor")
  , mResumeCompositionMonitor("ResumeCompositionMonitor")
  , mOverrideComposeReadiness(false)
  , mForceCompositionTask(nullptr)
  , mCompositorThreadHolder(sCompositorThreadHolder)
  , mCompositorScheduler(nullptr)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(CompositorThread(),
             "The compositor thread must be Initialized before instanciating a CompositorParent.");
  MOZ_COUNT_CTOR(CompositorParent);
  mCompositorID = 0;
  
  
  
  MOZ_ASSERT(CompositorLoop());
  CompositorLoop()->PostTask(FROM_HERE, NewRunnableFunction(&AddCompositor,
                                                          this, &mCompositorID));

  CompositorLoop()->PostTask(FROM_HERE, NewRunnableFunction(SetThreadPriority));

  mRootLayerTreeID = AllocateLayerTreeId();

  { 
    MonitorAutoLock lock(*sIndirectLayerTreesLock);
    sIndirectLayerTrees[mRootLayerTreeID].mParent = this;
  }

  
  
  if (gfxPlatform::AsyncPanZoomEnabled() &&
      (aWidget->WindowType() == eWindowType_toplevel || aWidget->WindowType() == eWindowType_child)) {
    mApzcTreeManager = new APZCTreeManager();
  }

  if (UseVsyncComposition()) {
    gfxDebugOnce() << "Enabling vsync compositor";
    mCompositorScheduler = new CompositorVsyncScheduler(this, aWidget);
  } else {
    mCompositorScheduler = new CompositorSoftwareTimerScheduler(this);
  }

  gfxPlatform::GetPlatform()->ComputeTileSize();
}

bool
CompositorParent::IsInCompositorThread()
{
  return CompositorThread() && CompositorThread()->thread_id() == PlatformThread::CurrentId();
}

uint64_t
CompositorParent::RootLayerTreeId()
{
  return mRootLayerTreeID;
}

CompositorParent::~CompositorParent()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_DTOR(CompositorParent);
}

void
CompositorParent::Destroy()
{
  MOZ_ASSERT(ManagedPLayerTransactionParent().Length() == 0,
             "CompositorParent destroyed before managed PLayerTransactionParent");

  MOZ_ASSERT(mPaused); 
  
  mLayerManager = nullptr;
  if (mCompositor) {
    mCompositor->Destroy();
  }
  mCompositor = nullptr;

  mCompositionManager = nullptr;
  if (mApzcTreeManager) {
    mApzcTreeManager->ClearTree();
    mApzcTreeManager = nullptr;
  }
  { 
    MonitorAutoLock lock(*sIndirectLayerTreesLock);
    sIndirectLayerTrees.erase(mRootLayerTreeID);
  }

  mCompositorScheduler->Destroy();
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
    MonitorAutoLock lock(*sIndirectLayerTreesLock);
    for (LayerTreeMap::iterator it = sIndirectLayerTrees.begin();
         it != sIndirectLayerTrees.end(); it++)
    {
      LayerTreeState* lts = &it->second;
      if (lts->mParent == this) {
        mLayerManager->ClearCachedResources(lts->mRoot);
        lts->mLayerManager = nullptr;
        lts->mParent = nullptr;
      }
    }
    mLayerManager->Destroy();
    mLayerManager = nullptr;
    mCompositionManager = nullptr;
  }

  return true;
}

void CompositorParent::DeferredDestroy()
{
  MOZ_ASSERT(!NS_IsMainThread());
  mCompositorThreadHolder = nullptr;
  Release();
}

bool
CompositorParent::RecvStop()
{
  Destroy();
  
  
  
  
  
  this->AddRef(); 
  MessageLoop::current()->PostTask(FROM_HERE,
                                   NewRunnableMethod(this,&CompositorParent::DeferredDestroy));
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
                                   const gfx::IntRect& aRect)
{
  RefPtr<DrawTarget> target = GetDrawTargetForDescriptor(aInSnapshot, gfx::BackendType::CAIRO);
  ForceComposeToTarget(target, &aRect);
  return true;
}

bool
CompositorParent::RecvMakeWidgetSnapshot(const SurfaceDescriptor& aInSnapshot)
{
  if (!mCompositor || !mCompositor->GetWidget()) {
    return false;
  }

  RefPtr<DrawTarget> target = GetDrawTargetForDescriptor(aInSnapshot, gfx::BackendType::CAIRO);
  mCompositor->GetWidget()->CaptureWidgetOnScreen(target);
  return true;
}

bool
CompositorParent::RecvFlushRendering()
{
  if (mCompositorScheduler->NeedsComposite())
  {
    CancelCurrentCompositeTask();
    ForceComposeToTarget(nullptr);
  }
  return true;
}

bool
CompositorParent::RecvGetTileSize(int32_t* aWidth, int32_t* aHeight)
{
  *aWidth = gfxPlatform::GetPlatform()->GetTileWidth();
  *aHeight = gfxPlatform::GetPlatform()->GetTileHeight();
  return true;
}

bool
CompositorParent::RecvNotifyRegionInvalidated(const nsIntRegion& aRegion)
{
  if (mLayerManager) {
    mLayerManager->AddInvalidRegion(aRegion);
  }
  return true;
}

bool
CompositorParent::RecvStartFrameTimeRecording(const int32_t& aBufferSize, uint32_t* aOutStartIndex)
{
  if (mLayerManager) {
    *aOutStartIndex = mLayerManager->StartFrameTimeRecording(aBufferSize);
  } else {
    *aOutStartIndex = 0;
  }
  return true;
}

bool
CompositorParent::RecvStopFrameTimeRecording(const uint32_t& aStartIndex,
                                             InfallibleTArray<float>* intervals)
{
  if (mLayerManager) {
    mLayerManager->StopFrameTimeRecording(aStartIndex, *intervals);
  }
  return true;
}

void
CompositorParent::ActorDestroy(ActorDestroyReason why)
{
  CancelCurrentCompositeTask();
  if (mForceCompositionTask) {
    mForceCompositionTask->Cancel();
    mForceCompositionTask = nullptr;
  }
  mPaused = true;
  RemoveCompositor(mCompositorID);

  if (mLayerManager) {
    mLayerManager->Destroy();
    mLayerManager = nullptr;
    { 
      MonitorAutoLock lock(*sIndirectLayerTreesLock);
      sIndirectLayerTrees[mRootLayerTreeID].mLayerManager = nullptr;
    }
    mCompositionManager = nullptr;
    mCompositor = nullptr;
  }
}


void
CompositorParent::ScheduleRenderOnCompositorThread()
{
  CancelableTask *renderTask = NewRunnableMethod(this, &CompositorParent::ScheduleComposition);
  MOZ_ASSERT(CompositorLoop());
  CompositorLoop()->PostTask(FROM_HERE, renderTask);
}

void
CompositorParent::PauseComposition()
{
  MOZ_ASSERT(IsInCompositorThread(),
             "PauseComposition() can only be called on the compositor thread");

  MonitorAutoLock lock(mPauseCompositionMonitor);

  if (!mPaused) {
    mPaused = true;

    mCompositor->Pause();
    DidComposite();
  }

  
  lock.NotifyAll();
}

void
CompositorParent::ResumeComposition()
{
  MOZ_ASSERT(IsInCompositorThread(),
             "ResumeComposition() can only be called on the compositor thread");

  MonitorAutoLock lock(mResumeCompositionMonitor);

  if (!mCompositor->Resume()) {
#ifdef MOZ_WIDGET_ANDROID
    
    
    __android_log_print(ANDROID_LOG_INFO, "CompositorParent", "Unable to renew compositor surface; remaining in paused state");
#endif
    lock.NotifyAll();
    return;
  }

  mPaused = false;

  mCompositorScheduler->ResumeComposition();

  
  lock.NotifyAll();
}

void
CompositorParent::ForceComposition()
{
  
  mForceCompositionTask = nullptr;
  ScheduleRenderOnCompositorThread();
}

void
CompositorParent::CancelCurrentCompositeTask()
{
  mCompositorScheduler->CancelCurrentCompositeTask();
}

void
CompositorParent::SetEGLSurfaceSize(int width, int height)
{
  NS_ASSERTION(mUseExternalSurfaceSize, "Compositor created without UseExternalSurfaceSize provided");
  mEGLSurfaceSize.SizeTo(width, height);
  if (mCompositor) {
    mCompositor->SetDestinationSurfaceSize(gfx::IntSize(mEGLSurfaceSize.width, mEGLSurfaceSize.height));
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
  MOZ_ASSERT(CompositorLoop());
  CompositorLoop()->PostTask(FROM_HERE, pauseTask);

  
  lock.Wait();
}

bool
CompositorParent::ScheduleResumeOnCompositorThread()
{
  MonitorAutoLock lock(mResumeCompositionMonitor);

  CancelableTask *resumeTask =
    NewRunnableMethod(this, &CompositorParent::ResumeComposition);
  MOZ_ASSERT(CompositorLoop());
  CompositorLoop()->PostTask(FROM_HERE, resumeTask);

  
  lock.Wait();

  return !mPaused;
}

bool
CompositorParent::ScheduleResumeOnCompositorThread(int width, int height)
{
  MonitorAutoLock lock(mResumeCompositionMonitor);

  CancelableTask *resumeTask =
    NewRunnableMethod(this, &CompositorParent::ResumeCompositionAndResize, width, height);
  MOZ_ASSERT(CompositorLoop());
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
CompositorParent::NotifyShadowTreeTransaction(uint64_t aId, bool aIsFirstPaint,
    bool aScheduleComposite, uint32_t aPaintSequenceNumber,
    bool aIsRepeatTransaction)
{
  if (mApzcTreeManager &&
      !aIsRepeatTransaction &&
      mLayerManager &&
      mLayerManager->GetRoot()) {
    AutoResolveRefLayers resolve(mCompositionManager);
    mApzcTreeManager->UpdateHitTestingTree(this, mLayerManager->GetRoot(),
        aIsFirstPaint, aId, aPaintSequenceNumber);

    mLayerManager->NotifyShadowTreeTransaction();
  }
  if (aScheduleComposite) {
    ScheduleComposition();
  }
}

void
CompositorParent::ScheduleComposition()
{
  MOZ_ASSERT(IsInCompositorThread());
  if (mPaused) {
    return;
  }

  mCompositorScheduler->ScheduleComposition();
}



 void
CompositorParent::SetShadowProperties(Layer* aLayer)
{
  if (Layer* maskLayer = aLayer->GetMaskLayer()) {
    SetShadowProperties(maskLayer);
  }
  for (size_t i = 0; i < aLayer->GetAncestorMaskLayerCount(); i++) {
    SetShadowProperties(aLayer->GetAncestorMaskLayerAt(i));
  }

  
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
CompositorParent::CompositeToTarget(DrawTarget* aTarget, const gfx::IntRect* aRect)
{
  profiler_tracing("Paint", "Composite", TRACING_INTERVAL_START);
  PROFILER_LABEL("CompositorParent", "Composite",
    js::ProfileEntry::Category::GRAPHICS);

  MOZ_ASSERT(IsInCompositorThread(),
             "Composite can only be called on the compositor thread");
  TimeStamp start = TimeStamp::Now();

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  TimeDuration scheduleDelta = TimeStamp::Now() - mCompositorScheduler->GetExpectedComposeStartTime();
  if (scheduleDelta > TimeDuration::FromMilliseconds(2) ||
      scheduleDelta < TimeDuration::FromMilliseconds(-2)) {
    printf_stderr("Compositor: Compose starting off schedule by %4.1f ms\n",
                  scheduleDelta.ToMilliseconds());
  }
#endif

  if (!CanComposite()) {
    DidComposite();
    return;
  }

  AutoResolveRefLayers resolve(mCompositionManager);

  if (aTarget) {
    mLayerManager->BeginTransactionWithDrawTarget(aTarget, *aRect);
  } else {
    mLayerManager->BeginTransaction();
  }

  SetShadowProperties(mLayerManager->GetRoot());

  if (mForceCompositionTask && !mOverrideComposeReadiness) {
    if (mCompositionManager->ReadyForCompose()) {
      mForceCompositionTask->Cancel();
      mForceCompositionTask = nullptr;
    } else {
      return;
    }
  }

  mCompositionManager->ComputeRotation();

  TimeStamp time = mIsTesting ? mTestTime : mCompositorScheduler->GetLastComposeTime();
  bool requestNextFrame = mCompositionManager->TransformShadowTree(time);
  if (requestNextFrame) {
    ScheduleComposition();
  }

  RenderTraceLayers(mLayerManager->GetRoot(), "0000");

#ifdef MOZ_DUMP_PAINTING
  if (gfxPrefs::DumpHostLayers()) {
    printf_stderr("Painting --- compositing layer tree:\n");
    mLayerManager->Dump();
  }
#endif
  mLayerManager->SetDebugOverlayWantsNextFrame(false);
  mLayerManager->EndEmptyTransaction();

  if (!aTarget) {
    DidComposite();
  }

  if (mLayerManager->DebugOverlayWantsNextFrame()) {
    ScheduleComposition();
  }

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  TimeDuration executionTime = TimeStamp::Now() - mCompositorScheduler->GetLastComposeTime();
  TimeDuration frameBudget = TimeDuration::FromMilliseconds(15);
  int32_t frameRate = CalculateCompositionFrameRate();
  if (frameRate > 0) {
    frameBudget = TimeDuration::FromSeconds(1.0 / frameRate);
  }
  if (executionTime > frameBudget) {
    printf_stderr("Compositor: Composite execution took %4.1f ms\n",
                  executionTime.ToMilliseconds());
  }
#endif

  
  if (gfxPrefs::LayersCompositionFrameRate() == 0
    || mLayerManager->GetCompositor()->GetDiagnosticTypes() & DiagnosticTypes::FLASH_BORDERS) {
    
    ScheduleComposition();
  }

  mozilla::Telemetry::AccumulateTimeDelta(mozilla::Telemetry::COMPOSITE_TIME, start);
  profiler_tracing("Paint", "Composite", TRACING_INTERVAL_END);
}

void
CompositorParent::ForceComposeToTarget(DrawTarget* aTarget, const gfx::IntRect* aRect)
{
  PROFILER_LABEL("CompositorParent", "ForceComposeToTarget",
    js::ProfileEntry::Category::GRAPHICS);

  AutoRestore<bool> override(mOverrideComposeReadiness);
  mOverrideComposeReadiness = true;
  mCompositorScheduler->ForceComposeToTarget(aTarget, aRect);
}

bool
CompositorParent::CanComposite()
{
  return mLayerManager &&
         mLayerManager->GetRoot() &&
         !mPaused;
}

void
CompositorParent::ScheduleRotationOnCompositorThread(const TargetConfig& aTargetConfig,
                                                     bool aIsFirstPaint)
{
  MOZ_ASSERT(IsInCompositorThread());

  if (!aIsFirstPaint &&
      !mCompositionManager->IsFirstPaint() &&
      mCompositionManager->RequiresReorientation(aTargetConfig.orientation())) {
    if (mForceCompositionTask != nullptr) {
      mForceCompositionTask->Cancel();
    }
    mForceCompositionTask = NewRunnableMethod(this, &CompositorParent::ForceComposition);
    ScheduleTask(mForceCompositionTask, gfxPrefs::OrientationSyncMillis());
  }
}

void
CompositorParent::ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                      const uint64_t& aTransactionId,
                                      const TargetConfig& aTargetConfig,
                                      const InfallibleTArray<PluginWindowData>& aUnused,
                                      bool aIsFirstPaint,
                                      bool aScheduleComposite,
                                      uint32_t aPaintSequenceNumber,
                                      bool aIsRepeatTransaction)
{
  ScheduleRotationOnCompositorThread(aTargetConfig, aIsFirstPaint);

  
  
  
  mLayerManager->UpdateRenderBounds(aTargetConfig.naturalBounds());
  mLayerManager->SetRegionToClear(aTargetConfig.clearRegion());

  mCompositionManager->Updated(aIsFirstPaint, aTargetConfig);
  Layer* root = aLayerTree->GetRoot();
  mLayerManager->SetRoot(root);

  if (mApzcTreeManager && !aIsRepeatTransaction) {
    AutoResolveRefLayers resolve(mCompositionManager);
    mApzcTreeManager->UpdateHitTestingTree(this, root, aIsFirstPaint,
        mRootLayerTreeID, aPaintSequenceNumber);
  }

#ifdef DEBUG
  if (aTransactionId <= mPendingTransaction) {
    
    
    printf_stderr("CRASH: aTransactionId %" PRIu64 " <= mPendingTransaction %" PRIu64 "\n",
      aTransactionId, mPendingTransaction);
  }
#endif
  MOZ_ASSERT(aTransactionId > mPendingTransaction);
  mPendingTransaction = aTransactionId;

  if (root) {
    SetShadowProperties(root);
  }
  if (aScheduleComposite) {
    ScheduleComposition();
    if (mPaused) {
      DidComposite();
    }
  }
  mLayerManager->NotifyShadowTreeTransaction();
}

void
CompositorParent::ForceComposite(LayerTransactionParent* aLayerTree)
{
  ScheduleComposition();
}

bool
CompositorParent::SetTestSampleTime(LayerTransactionParent* aLayerTree,
                                    const TimeStamp& aTime)
{
  if (aTime.IsNull()) {
    return false;
  }

  mIsTesting = true;
  mTestTime = aTime;

  bool testComposite = mCompositionManager &&
                       mCompositorScheduler->NeedsComposite();

  
  if (testComposite) {
    AutoResolveRefLayers resolve(mCompositionManager);
    bool requestNextFrame = mCompositionManager->TransformShadowTree(aTime);
    if (!requestNextFrame) {
      CancelCurrentCompositeTask();
      
      DidComposite();
    }
  }

  return true;
}

void
CompositorParent::LeaveTestMode(LayerTransactionParent* aLayerTree)
{
  mIsTesting = false;
}

void
CompositorParent::ApplyAsyncProperties(LayerTransactionParent* aLayerTree)
{
  
  
  

  
  if (aLayerTree->GetRoot()) {
    AutoResolveRefLayers resolve(mCompositionManager);
    SetShadowProperties(mLayerManager->GetRoot());

    TimeStamp time = mIsTesting ? mTestTime : mCompositorScheduler->GetLastComposeTime();
    bool requestNextFrame =
      mCompositionManager->TransformShadowTree(time,
        AsyncCompositionManager::TransformsToSkip::APZ);
    if (!requestNextFrame) {
      CancelCurrentCompositeTask();
      
      DidComposite();
    }
  }
}

bool
CompositorParent::RecvGetFrameUniformity(FrameUniformityData* aOutData)
{
  mCompositionManager->GetFrameUniformity(aOutData);
  return true;
}

bool
CompositorParent::RecvRequestOverfill()
{
  uint32_t overfillRatio = mCompositor->GetFillRatio();
  unused << SendOverfill(overfillRatio);
  return true;
}

void
CompositorParent::FlushApzRepaints(const LayerTransactionParent* aLayerTree)
{
  MOZ_ASSERT(mApzcTreeManager);
  uint64_t layersId = aLayerTree->GetId();
  if (layersId == 0) {
    
    
    layersId = mRootLayerTreeID;
  }
  mApzcTreeManager->FlushApzRepaints(layersId);
}

void
CompositorParent::GetAPZTestData(const LayerTransactionParent* aLayerTree,
                                 APZTestData* aOutData)
{
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  *aOutData = sIndirectLayerTrees[mRootLayerTreeID].mApzTestData;
}

class NotifyAPZConfirmedTargetTask : public Task
{
public:
  explicit NotifyAPZConfirmedTargetTask(const nsRefPtr<APZCTreeManager>& aAPZCTM,
                                        const uint64_t& aInputBlockId,
                                        const nsTArray<ScrollableLayerGuid>& aTargets)
   : mAPZCTM(aAPZCTM),
     mInputBlockId(aInputBlockId),
     mTargets(aTargets)
  {
  }

  virtual void Run() override {
    mAPZCTM->SetTargetAPZC(mInputBlockId, mTargets);
  }

private:
  nsRefPtr<APZCTreeManager> mAPZCTM;
  uint64_t mInputBlockId;
  nsTArray<ScrollableLayerGuid> mTargets;
};

void
CompositorParent::SetConfirmedTargetAPZC(const LayerTransactionParent* aLayerTree,
                                         const uint64_t& aInputBlockId,
                                         const nsTArray<ScrollableLayerGuid>& aTargets)
{
  if (!mApzcTreeManager) {
    return;
  }
  APZThreadUtils::RunOnControllerThread(new NotifyAPZConfirmedTargetTask(
    mApzcTreeManager, aInputBlockId, aTargets));
}

void
CompositorParent::InitializeLayerManager(const nsTArray<LayersBackend>& aBackendHints)
{
  NS_ASSERTION(!mLayerManager, "Already initialised mLayerManager");
  NS_ASSERTION(!mCompositor,   "Already initialised mCompositor");

  for (size_t i = 0; i < aBackendHints.Length(); ++i) {
    RefPtr<Compositor> compositor;
    if (aBackendHints[i] == LayersBackend::LAYERS_OPENGL) {
      compositor = new CompositorOGL(mWidget,
                                     mEGLSurfaceSize.width,
                                     mEGLSurfaceSize.height,
                                     mUseExternalSurfaceSize);
    } else if (aBackendHints[i] == LayersBackend::LAYERS_BASIC) {
#ifdef MOZ_WIDGET_GTK
      if (gfxPlatformGtk::GetPlatform()->UseXRender()) {
        compositor = new X11BasicCompositor(mWidget);
      } else
#endif
      {
        compositor = new BasicCompositor(mWidget);
      }
#ifdef XP_WIN
    } else if (aBackendHints[i] == LayersBackend::LAYERS_D3D11) {
      compositor = new CompositorD3D11(mWidget);
    } else if (aBackendHints[i] == LayersBackend::LAYERS_D3D9) {
      compositor = new CompositorD3D9(this, mWidget);
#endif
    }

    if (!compositor) {
      
      
      continue;
    }

    compositor->SetCompositorID(mCompositorID);
    RefPtr<LayerManagerComposite> layerManager = new LayerManagerComposite(compositor);

    if (layerManager->Initialize()) {
      mLayerManager = layerManager;
      MOZ_ASSERT(compositor);
      mCompositor = compositor;
      MonitorAutoLock lock(*sIndirectLayerTreesLock);
      sIndirectLayerTrees[mRootLayerTreeID].mLayerManager = layerManager;
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

  gfx::IntRect rect;
  mWidget->GetClientBounds(rect);
  InitializeLayerManager(aBackendHints);

  if (!mLayerManager) {
    NS_WARNING("Failed to initialise Compositor");
    *aSuccess = false;
    LayerTransactionParent* p = new LayerTransactionParent(nullptr, this, 0);
    p->AddIPDLReference();
    return p;
  }

  mCompositionManager = new AsyncCompositionManager(mLayerManager);
  *aSuccess = true;

  *aTextureFactoryIdentifier = mCompositor->GetTextureFactoryIdentifier();
  LayerTransactionParent* p = new LayerTransactionParent(mLayerManager, this, 0);
  p->AddIPDLReference();
  return p;
}

bool
CompositorParent::DeallocPLayerTransactionParent(PLayerTransactionParent* actor)
{
  static_cast<LayerTransactionParent*>(actor)->ReleaseIPDLReference();
  return true;
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

bool
CompositorParent::RecvNotifyChildCreated(const uint64_t& child)
{
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  NotifyChildCreated(child);
  return true;
}

void
CompositorParent::NotifyChildCreated(const uint64_t& aChild)
{
  sIndirectLayerTreesLock->AssertCurrentThreadOwns();
  sIndirectLayerTrees[aChild].mParent = this;
  sIndirectLayerTrees[aChild].mLayerManager = mLayerManager;
}

bool
CompositorParent::RecvAdoptChild(const uint64_t& child)
{
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  NotifyChildCreated(child);
  if (sIndirectLayerTrees[child].mLayerTree) {
    sIndirectLayerTrees[child].mLayerTree->mLayerManager = mLayerManager;
  }
  if (sIndirectLayerTrees[child].mRoot) {
    sIndirectLayerTrees[child].mRoot->AsLayerComposite()->SetLayerManager(mLayerManager);
  }
  return true;
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
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  sIndirectLayerTrees.erase(aId);
}

 void
CompositorParent::DeallocateLayerTreeId(uint64_t aId)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  
  if (!CompositorLoop()) {
    gfxCriticalError() << "Attempting to post to a invalid Compositor Loop";
    return;
  }
  CompositorLoop()->PostTask(FROM_HERE,
                             NewRunnableFunction(&EraseLayerState, aId));
}

 void
CompositorParent::SwapLayerTreeObservers(uint64_t aLayerId, uint64_t aOtherLayerId)
{
  EnsureLayerTreeMapReady();
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  NS_ASSERTION(sIndirectLayerTrees.find(aLayerId) != sIndirectLayerTrees.end(),
    "SwapLayerTrees missing layer 1");
  NS_ASSERTION(sIndirectLayerTrees.find(aOtherLayerId) != sIndirectLayerTrees.end(),
    "SwapLayerTrees missing layer 2");
  std::swap(sIndirectLayerTrees[aLayerId].mLayerTreeReadyObserver,
    sIndirectLayerTrees[aOtherLayerId].mLayerTreeReadyObserver);
}

static void
UpdateControllerForLayersId(uint64_t aLayersId,
                            GeckoContentController* aController)
{
  
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  sIndirectLayerTrees[aLayersId].mController =
    already_AddRefed<GeckoContentController>(aController);
}

ScopedLayerTreeRegistration::ScopedLayerTreeRegistration(uint64_t aLayersId,
                                                         Layer* aRoot,
                                                         GeckoContentController* aController)
    : mLayersId(aLayersId)
{
  EnsureLayerTreeMapReady();
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  sIndirectLayerTrees[aLayersId].mRoot = aRoot;
  sIndirectLayerTrees[aLayersId].mController = aController;
}

ScopedLayerTreeRegistration::~ScopedLayerTreeRegistration()
{
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
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

float
CompositorParent::ComputeRenderIntegrity()
{
  if (mLayerManager) {
    return mLayerManager->ComputeRenderIntegrity();
  }

  return 1.0f;
}

static void
InsertVsyncProfilerMarker(TimeStamp aVsyncTimestamp)
{
#ifdef MOZ_ENABLE_PROFILER_SPS
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  VsyncPayload* payload = new VsyncPayload(aVsyncTimestamp);
  PROFILER_MARKER_PAYLOAD("VsyncTimestamp", payload);
#endif
}

 void
CompositorParent::PostInsertVsyncProfilerMarker(TimeStamp aVsyncTimestamp)
{
  
  if (profiler_is_active() && sCompositorThreadHolder) {
    CompositorLoop()->PostTask(FROM_HERE,
      NewRunnableFunction(InsertVsyncProfilerMarker, aVsyncTimestamp));
  }
}

 void
CompositorParent::RequestNotifyLayerTreeReady(uint64_t aLayersId, CompositorUpdateObserver* aObserver)
{
  EnsureLayerTreeMapReady();
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  sIndirectLayerTrees[aLayersId].mLayerTreeReadyObserver = aObserver;
}

 void
CompositorParent::RequestNotifyLayerTreeCleared(uint64_t aLayersId, CompositorUpdateObserver* aObserver)
{
  EnsureLayerTreeMapReady();
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  sIndirectLayerTrees[aLayersId].mLayerTreeClearedObserver = aObserver;
}









class CrossProcessCompositorParent final : public PCompositorParent,
                                           public ShadowLayersManager
{
  friend class CompositorParent;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(CrossProcessCompositorParent)
public:
  explicit CrossProcessCompositorParent(Transport* aTransport)
    : mTransport(aTransport)
    , mCompositorThreadHolder(sCompositorThreadHolder)
    , mNotifyAfterRemotePaint(false)
  {
    MOZ_ASSERT(NS_IsMainThread());
    gfxPlatform::GetPlatform()->ComputeTileSize();
  }

  
  virtual IToplevelProtocol*
  CloneToplevel(const InfallibleTArray<mozilla::ipc::ProtocolFdMapping>& aFds,
                base::ProcessHandle aPeerProcess,
                mozilla::ipc::ProtocolCloneContext* aCtx) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  
  virtual bool RecvRequestOverfill() override { return true; }
  virtual bool RecvWillStop() override { return true; }
  virtual bool RecvStop() override { return true; }
  virtual bool RecvPause() override { return true; }
  virtual bool RecvResume() override { return true; }
  virtual bool RecvNotifyHidden(const uint64_t& id) override;
  virtual bool RecvNotifyVisible(const uint64_t& id) override;
  virtual bool RecvNotifyChildCreated(const uint64_t& child) override;
  virtual bool RecvAdoptChild(const uint64_t& child) override { return false; }
  virtual bool RecvMakeSnapshot(const SurfaceDescriptor& aInSnapshot,
                                const gfx::IntRect& aRect) override
  { return true; }
  virtual bool RecvMakeWidgetSnapshot(const SurfaceDescriptor& aInSnapshot) override
  { return true; }
  virtual bool RecvFlushRendering() override { return true; }
  virtual bool RecvNotifyRegionInvalidated(const nsIntRegion& aRegion) override { return true; }
  virtual bool RecvStartFrameTimeRecording(const int32_t& aBufferSize, uint32_t* aOutStartIndex) override { return true; }
  virtual bool RecvStopFrameTimeRecording(const uint32_t& aStartIndex, InfallibleTArray<float>* intervals) override  { return true; }
  virtual bool RecvGetTileSize(int32_t* aWidth, int32_t* aHeight) override
  {
    *aWidth = gfxPlatform::GetPlatform()->GetTileWidth();
    *aHeight = gfxPlatform::GetPlatform()->GetTileHeight();
    return true;
  }

  virtual bool RecvGetFrameUniformity(FrameUniformityData* aOutData) override
  {
    
    
    MOZ_ASSERT(false);
    return true;
  }

  


  virtual bool RecvRequestNotifyAfterRemotePaint() override;

  virtual PLayerTransactionParent*
    AllocPLayerTransactionParent(const nsTArray<LayersBackend>& aBackendHints,
                                 const uint64_t& aId,
                                 TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                 bool *aSuccess) override;

  virtual bool DeallocPLayerTransactionParent(PLayerTransactionParent* aLayers) override;

  virtual void ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                   const uint64_t& aTransactionId,
                                   const TargetConfig& aTargetConfig,
                                   const InfallibleTArray<PluginWindowData>& aPlugins,
                                   bool aIsFirstPaint,
                                   bool aScheduleComposite,
                                   uint32_t aPaintSequenceNumber,
                                   bool aIsRepeatTransaction) override;
  virtual void ForceComposite(LayerTransactionParent* aLayerTree) override;
  virtual void NotifyClearCachedResources(LayerTransactionParent* aLayerTree) override;
  virtual bool SetTestSampleTime(LayerTransactionParent* aLayerTree,
                                 const TimeStamp& aTime) override;
  virtual void LeaveTestMode(LayerTransactionParent* aLayerTree) override;
  virtual void ApplyAsyncProperties(LayerTransactionParent* aLayerTree)
               override;
  virtual void FlushApzRepaints(const LayerTransactionParent* aLayerTree) override;
  virtual void GetAPZTestData(const LayerTransactionParent* aLayerTree,
                              APZTestData* aOutData) override;
  virtual void SetConfirmedTargetAPZC(const LayerTransactionParent* aLayerTree,
                                      const uint64_t& aInputBlockId,
                                      const nsTArray<ScrollableLayerGuid>& aTargets) override;

  virtual AsyncCompositionManager* GetCompositionManager(LayerTransactionParent* aParent) override;

  void DidComposite(uint64_t aId);

private:
  
  virtual ~CrossProcessCompositorParent();

  void DeferredDestroy();

  
  
  
  nsRefPtr<CrossProcessCompositorParent> mSelfRef;
  Transport* mTransport;

  nsRefPtr<CompositorThreadHolder> mCompositorThreadHolder;
  
  
  bool mNotifyAfterRemotePaint;
};

void
CompositorParent::DidComposite()
{
  if (mPendingTransaction) {
    unused << SendDidComposite(0, mPendingTransaction);
    mPendingTransaction = 0;
  }

  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  for (LayerTreeMap::iterator it = sIndirectLayerTrees.begin();
       it != sIndirectLayerTrees.end(); it++) {
    LayerTreeState* lts = &it->second;
    if (lts->mParent == this && lts->mCrossProcessParent) {
      static_cast<CrossProcessCompositorParent*>(lts->mCrossProcessParent)->DidComposite(it->first);
    }
  }
}

static void
OpenCompositor(CrossProcessCompositorParent* aCompositor,
               Transport* aTransport, ProcessId aOtherPid,
               MessageLoop* aIOLoop)
{
  DebugOnly<bool> ok = aCompositor->Open(aTransport, aOtherPid, aIOLoop);
  MOZ_ASSERT(ok);
}

 PCompositorParent*
CompositorParent::Create(Transport* aTransport, ProcessId aOtherPid)
{
  gfxPlatform::InitLayersIPC();

  nsRefPtr<CrossProcessCompositorParent> cpcp =
    new CrossProcessCompositorParent(aTransport);

  cpcp->mSelfRef = cpcp;
  CompositorLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(OpenCompositor, cpcp.get(),
                        aTransport, aOtherPid, XRE_GetIOMessageLoop()));
  
  
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
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  sIndirectLayerTrees[aId].mRoot = aRoot;
  sIndirectLayerTrees[aId].mTargetConfig = aTargetConfig;
}

 CompositorParent::LayerTreeState*
CompositorParent::GetIndirectShadowTree(uint64_t aId)
{
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  LayerTreeMap::iterator cit = sIndirectLayerTrees.find(aId);
  if (sIndirectLayerTrees.end() == cit) {
    return nullptr;
  }
  return &cit->second;
}

bool
CrossProcessCompositorParent::RecvNotifyHidden(const uint64_t& id)
{
  nsRefPtr<CompositorLRU> lru = CompositorLRU::GetSingleton();
  lru->Add(this, id);
  return true;
}

bool
CrossProcessCompositorParent::RecvNotifyVisible(const uint64_t& id)
{
  nsRefPtr<CompositorLRU> lru = CompositorLRU::GetSingleton();
  lru->Remove(this, id);
  return true;
}

bool
CrossProcessCompositorParent::RecvRequestNotifyAfterRemotePaint()
{
  mNotifyAfterRemotePaint = true;
  return true;
}

void
CrossProcessCompositorParent::ActorDestroy(ActorDestroyReason aWhy)
{
  nsRefPtr<CompositorLRU> lru = CompositorLRU::GetSingleton();
  lru->Remove(this);

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

  MonitorAutoLock lock(*sIndirectLayerTreesLock);

  CompositorParent::LayerTreeState* state = nullptr;
  LayerTreeMap::iterator itr = sIndirectLayerTrees.find(aId);
  if (sIndirectLayerTrees.end() != itr) {
    state = &itr->second;
  }

  if (state && state->mLayerManager) {
    state->mCrossProcessParent = this;
    LayerManagerComposite* lm = state->mLayerManager;
    *aTextureFactoryIdentifier = lm->GetCompositor()->GetTextureFactoryIdentifier();
    *aSuccess = true;
    LayerTransactionParent* p = new LayerTransactionParent(lm, this, aId);
    p->AddIPDLReference();
    sIndirectLayerTrees[aId].mLayerTree = p;
    return p;
  }

  NS_WARNING("Created child without a matching parent?");
  
  
  *aSuccess = true;
  LayerTransactionParent* p = new LayerTransactionParent(nullptr, this, aId);
  p->AddIPDLReference();
  return p;
}

bool
CrossProcessCompositorParent::DeallocPLayerTransactionParent(PLayerTransactionParent* aLayers)
{
  LayerTransactionParent* slp = static_cast<LayerTransactionParent*>(aLayers);
  EraseLayerState(slp->GetId());
  static_cast<LayerTransactionParent*>(aLayers)->ReleaseIPDLReference();
  return true;
}

bool
CrossProcessCompositorParent::RecvNotifyChildCreated(const uint64_t& child)
{
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  for (LayerTreeMap::iterator it = sIndirectLayerTrees.begin();
       it != sIndirectLayerTrees.end(); it++) {
    CompositorParent::LayerTreeState* lts = &it->second;
    if (lts->mParent && lts->mCrossProcessParent == this) {
      lts->mParent->NotifyChildCreated(child);
      return true;
    }
  }
  return false;
}

void
CrossProcessCompositorParent::ShadowLayersUpdated(
  LayerTransactionParent* aLayerTree,
  const uint64_t& aTransactionId,
  const TargetConfig& aTargetConfig,
  const InfallibleTArray<PluginWindowData>& aPlugins,
  bool aIsFirstPaint,
  bool aScheduleComposite,
  uint32_t aPaintSequenceNumber,
  bool aIsRepeatTransaction)
{
  uint64_t id = aLayerTree->GetId();

  MOZ_ASSERT(id != 0);

  CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(id);
  if (!state) {
    return;
  }
  MOZ_ASSERT(state->mParent);
  state->mParent->ScheduleRotationOnCompositorThread(aTargetConfig, aIsFirstPaint);

  Layer* shadowRoot = aLayerTree->GetRoot();
  if (shadowRoot) {
    CompositorParent::SetShadowProperties(shadowRoot);
  }
  UpdateIndirectTree(id, shadowRoot, aTargetConfig);

  
  state->mPluginData = aPlugins;
  state->mUpdatedPluginDataAvailable = true;

  state->mParent->NotifyShadowTreeTransaction(id, aIsFirstPaint, aScheduleComposite,
      aPaintSequenceNumber, aIsRepeatTransaction);

  
  if(mNotifyAfterRemotePaint)  {
    unused << SendRemotePaintIsReady();
    mNotifyAfterRemotePaint = false;
  }

  if (state->mLayerTreeReadyObserver) {
    nsRefPtr<CompositorUpdateObserver> observer = state->mLayerTreeReadyObserver;
    state->mLayerTreeReadyObserver = nullptr;
    observer->ObserveUpdate(id, true);
  }

  aLayerTree->SetPendingTransactionId(aTransactionId);
}

#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK)

static void
UpdatePluginWindowState(uint64_t aId)
{
  CompositorParent::LayerTreeState& lts = sIndirectLayerTrees[aId];
  if (!lts.mPluginData.Length() && !lts.mUpdatedPluginDataAvailable) {
    return;
  }

  bool shouldComposePlugin = !!lts.mRoot &&
                             !!lts.mRoot->GetParent();

  bool shouldHidePlugin = (!lts.mRoot ||
                           !lts.mRoot->GetParent()) &&
                          !lts.mUpdatedPluginDataAvailable;
  if (shouldComposePlugin) {
    if (!lts.mPluginData.Length()) {
      
      
      
      
      nsTArray<uintptr_t> aVisibleIdList;
      uintptr_t parentWidget = (uintptr_t)lts.mParent->GetWidget();
      unused << lts.mParent->SendUpdatePluginVisibility(parentWidget,
                                                        aVisibleIdList);
      lts.mUpdatedPluginDataAvailable = false;
      return;
    }

    
    
    
    LayerTransactionParent* layerTree = lts.mLayerTree;
    Layer* contentRoot = layerTree->GetRoot();
    if (contentRoot) {
      nsIntPoint offset;
      nsIntRegion visibleRegion;
      if (contentRoot->GetVisibleRegionRelativeToRootLayer(visibleRegion,
                                                           &offset)) {
        unused <<
          lts.mParent->SendUpdatePluginConfigurations(offset, visibleRegion,
                                                      lts.mPluginData);
        lts.mUpdatedPluginDataAvailable = false;
      } else {
        shouldHidePlugin = true;
      }
    }
  }

  
  if (shouldHidePlugin) {
    
    for (uint32_t pluginsIdx = 0; pluginsIdx < lts.mPluginData.Length();
         pluginsIdx++) {
      lts.mPluginData[pluginsIdx].visible() = false;
    }
    nsIntPoint offset;
    nsIntRegion region;
    unused << lts.mParent->SendUpdatePluginConfigurations(offset,
                                                          region,
                                                          lts.mPluginData);
    
    
    lts.mPluginData.Clear();
  }
}
#endif 

void
CrossProcessCompositorParent::DidComposite(uint64_t aId)
{
  sIndirectLayerTreesLock->AssertCurrentThreadOwns();
  LayerTransactionParent *layerTree = sIndirectLayerTrees[aId].mLayerTree;
  if (layerTree && layerTree->GetPendingTransactionId()) {
    unused << SendDidComposite(aId, layerTree->GetPendingTransactionId());
    layerTree->SetPendingTransactionId(0);
  }
#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK)
  UpdatePluginWindowState(aId);
#endif
}

void
CrossProcessCompositorParent::ForceComposite(LayerTransactionParent* aLayerTree)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);
  CompositorParent* parent;
  { 
    MonitorAutoLock lock(*sIndirectLayerTreesLock);
    parent = sIndirectLayerTrees[id].mParent;
  }
  if (parent) {
    parent->ForceComposite(aLayerTree);
  }
}

void
CrossProcessCompositorParent::NotifyClearCachedResources(LayerTransactionParent* aLayerTree)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);

  nsRefPtr<CompositorUpdateObserver> observer;
  { 
    MonitorAutoLock lock(*sIndirectLayerTreesLock);
    observer = sIndirectLayerTrees[id].mLayerTreeClearedObserver;
    sIndirectLayerTrees[id].mLayerTreeClearedObserver = nullptr;
  }
  if (observer) {
    observer->ObserveUpdate(id, false);
  }
}

bool
CrossProcessCompositorParent::SetTestSampleTime(
  LayerTransactionParent* aLayerTree, const TimeStamp& aTime)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);
  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(id);
  if (!state) {
    return false;
  }

  MOZ_ASSERT(state->mParent);
  return state->mParent->SetTestSampleTime(aLayerTree, aTime);
}

void
CrossProcessCompositorParent::LeaveTestMode(LayerTransactionParent* aLayerTree)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);
  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(id);
  if (!state) {
    return;
  }

  MOZ_ASSERT(state->mParent);
  state->mParent->LeaveTestMode(aLayerTree);
}

void
CrossProcessCompositorParent::ApplyAsyncProperties(
    LayerTransactionParent* aLayerTree)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);
  const CompositorParent::LayerTreeState* state =
    CompositorParent::GetIndirectShadowTree(id);
  if (!state) {
    return;
  }

  MOZ_ASSERT(state->mParent);
  state->mParent->ApplyAsyncProperties(aLayerTree);
}

void
CrossProcessCompositorParent::FlushApzRepaints(const LayerTransactionParent* aLayerTree)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);
  const CompositorParent::LayerTreeState* state =
    CompositorParent::GetIndirectShadowTree(id);
  if (!state) {
    return;
  }

  MOZ_ASSERT(state->mParent);
  state->mParent->FlushApzRepaints(aLayerTree);
}

void
CrossProcessCompositorParent::GetAPZTestData(const LayerTransactionParent* aLayerTree,
                                             APZTestData* aOutData)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);
  MonitorAutoLock lock(*sIndirectLayerTreesLock);
  *aOutData = sIndirectLayerTrees[id].mApzTestData;
}

void
CrossProcessCompositorParent::SetConfirmedTargetAPZC(const LayerTransactionParent* aLayerTree,
                                                     const uint64_t& aInputBlockId,
                                                     const nsTArray<ScrollableLayerGuid>& aTargets)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);
  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(id);
  if (!state) {
    return;
  }

  MOZ_ASSERT(state->mParent);
  state->mParent->SetConfirmedTargetAPZC(aLayerTree, aInputBlockId, aTargets);
}

AsyncCompositionManager*
CrossProcessCompositorParent::GetCompositionManager(LayerTransactionParent* aLayerTree)
{
  uint64_t id = aLayerTree->GetId();
  const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(id);
  if (!state) {
    return nullptr;
  }

  MOZ_ASSERT(state->mParent);
  return state->mParent->GetCompositionManager(aLayerTree);
}

void
CrossProcessCompositorParent::DeferredDestroy()
{
  mCompositorThreadHolder = nullptr;
  mSelfRef = nullptr;
}

CrossProcessCompositorParent::~CrossProcessCompositorParent()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_GetIOMessageLoop());
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
