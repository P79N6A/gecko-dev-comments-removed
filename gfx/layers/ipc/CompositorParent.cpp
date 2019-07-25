





#include <map>

#include "base/basictypes.h"

#if defined(MOZ_WIDGET_ANDROID)
# include <android/log.h>
# include "AndroidBridge.h"
#endif

#include "BasicLayers.h"
#include "CompositorParent.h"
#include "LayerManagerOGL.h"
#include "nsGkAtoms.h"
#include "nsIWidget.h"
#include "RenderTrace.h"
#include "ShadowLayersParent.h"

using namespace base;
using namespace mozilla::ipc;
using namespace std;

namespace mozilla {
namespace layers {




static CompositorParent* sCurrentCompositor;
static Thread* sCompositorThread = nsnull;






static Layer* GetIndirectShadowTree(uint64_t aId);

void CompositorParent::StartUp()
{
  CreateCompositorMap();
  CreateThread();
}

void CompositorParent::ShutDown()
{
  DestroyThread();
  DestroyCompositorMap();
}

bool CompositorParent::CreateThread()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main Thread!");
  if (sCompositorThread) {
    return true;
  }
  sCompositorThread = new Thread("Compositor");
  if (!sCompositorThread->Start()) {
    delete sCompositorThread;
    sCompositorThread = nsnull;
    return false;
  }
  return true;
}

void CompositorParent::DestroyThread()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main Thread!");
  if (sCompositorThread) {
    delete sCompositorThread;
    sCompositorThread = nsnull;
  }
}

MessageLoop* CompositorParent::CompositorLoop()
{
  return sCompositorThread ? sCompositorThread->message_loop() : nsnull;
}

CompositorParent::CompositorParent(nsIWidget* aWidget,
                                   bool aRenderToEGLSurface,
                                   int aSurfaceWidth, int aSurfaceHeight)
  : mWidget(aWidget)
  , mCurrentCompositeTask(NULL)
  , mPaused(false)
  , mXScale(1.0)
  , mYScale(1.0)
  , mIsFirstPaint(false)
  , mLayersUpdated(false)
  , mRenderToEGLSurface(aRenderToEGLSurface)
  , mEGLSurfaceSize(aSurfaceWidth, aSurfaceHeight)
  , mPauseCompositionMonitor("PauseCompositionMonitor")
  , mResumeCompositionMonitor("ResumeCompositionMonitor")
{
  NS_ABORT_IF_FALSE(sCompositorThread != nsnull, 
                    "The compositor thread must be Initialized before instanciating a COmpositorParent.");
  MOZ_COUNT_CTOR(CompositorParent);
  mCompositorID = 0;
  
  
  
  CompositorLoop()->PostTask(FROM_HERE, NewRunnableFunction(&AddCompositor, 
                                                          this, &mCompositorID));

  sCurrentCompositor = this;
}

PlatformThreadId
CompositorParent::CompositorThreadID()
{
  return sCompositorThread->thread_id();
}

CompositorParent::~CompositorParent()
{
  MOZ_COUNT_DTOR(CompositorParent);

  if (this == sCurrentCompositor) {
    sCurrentCompositor = NULL;
  }
}

void
CompositorParent::Destroy()
{
  NS_ABORT_IF_FALSE(ManagedPLayersParent().Length() == 0,
                    "CompositorParent destroyed before managed PLayersParent");

  
  mLayerManager = NULL;
}

bool
CompositorParent::RecvWillStop()
{
  mPaused = true;
  RemoveCompositor(mCompositorID);

  
  mLayerManager->Destroy();

  return true;
}

bool
CompositorParent::RecvStop()
{
  Destroy();
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

#ifdef MOZ_WIDGET_ANDROID
    static_cast<LayerManagerOGL*>(mLayerManager.get())->gl()->ReleaseSurface();
#endif
  }

  
  lock.NotifyAll();
}

void
CompositorParent::ResumeComposition()
{
  NS_ABORT_IF_FALSE(CompositorThreadID() == PlatformThread::CurrentId(),
                    "ResumeComposition() can only be called on the compositor thread");

  MonitorAutoLock lock(mResumeCompositionMonitor);

  mPaused = false;

#ifdef MOZ_WIDGET_ANDROID
  static_cast<LayerManagerOGL*>(mLayerManager.get())->gl()->RenewSurface();
#endif

  Composite();

  
  lock.NotifyAll();
}

void
CompositorParent::SetEGLSurfaceSize(int width, int height)
{
  NS_ASSERTION(mRenderToEGLSurface, "Compositor created without RenderToEGLSurface ar provided");
  mEGLSurfaceSize.SizeTo(width, height);
  if (mLayerManager) {
    static_cast<LayerManagerOGL*>(mLayerManager.get())->SetSurfaceSize(mEGLSurfaceSize.width, mEGLSurfaceSize.height);
  }
}

void
CompositorParent::ResumeCompositionAndResize(int width, int height)
{
  mWidgetSize.width = width;
  mWidgetSize.height = height;
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

void
CompositorParent::ScheduleResumeOnCompositorThread(int width, int height)
{
  MonitorAutoLock lock(mResumeCompositionMonitor);

  CancelableTask *resumeTask =
    NewRunnableMethod(this, &CompositorParent::ResumeCompositionAndResize, width, height);
  CompositorLoop()->PostTask(FROM_HERE, resumeTask);

  
  lock.Wait();
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
  mXScale = aScale;
  mYScale = aScale;
  mScrollOffset = aScrollOffset;
}








class NS_STACK_CLASS AutoResolveRefLayers {
public:
  




  AutoResolveRefLayers(Layer* aRoot) : mRoot(aRoot)
  { WalkTheTree<Resolve>(mRoot, nsnull); }

  ~AutoResolveRefLayers()
  { WalkTheTree<Detach>(mRoot, nsnull); }

private:
  enum Op { Resolve, Detach };
  template<Op OP>
  void WalkTheTree(Layer* aLayer, Layer* aParent)
  {
    if (RefLayer* ref = aLayer->AsRefLayer()) {
      if (Layer* referent = GetIndirectShadowTree(ref->GetReferentId())) {
        if (OP == Resolve) {
          ref->ConnectReferentLayer(referent);
          TemporarilyCompensateForContentScrollOffset(ref, referent);
        } else {
          ref->DetachReferentLayer(referent);
        }
      }
    }
    for (Layer* child = aLayer->GetFirstChild();
         child; child = child->GetNextSibling()) {
      WalkTheTree<OP>(child, aLayer);
    }
  }

  
  
  void TemporarilyCompensateForContentScrollOffset(Layer* aContainer,
                                                   Layer* aShadowContent)
  {
    ContainerLayer* c = aShadowContent->AsContainerLayer();
    if (!c) {
      return;
    }
    const FrameMetrics& fm = c->GetFrameMetrics();
    gfx3DMatrix m(aContainer->GetTransform());
    m.Translate(gfxPoint3D(-fm.mViewportScrollOffset.x,
                           -fm.mViewportScrollOffset.y, 0));
    aContainer->AsShadowLayer()->SetShadowTransform(m);
  }

  Layer* mRoot;

  AutoResolveRefLayers(const AutoResolveRefLayers&) MOZ_DELETE;
  AutoResolveRefLayers& operator=(const AutoResolveRefLayers&) MOZ_DELETE;
};

void
CompositorParent::Composite()
{
  NS_ABORT_IF_FALSE(CompositorThreadID() == PlatformThread::CurrentId(),
                    "Composite can only be called on the compositor thread");
  mCurrentCompositeTask = NULL;

  mLastCompose = TimeStamp::Now();

  if (mPaused || !mLayerManager || !mLayerManager->GetRoot()) {
    return;
  }

  Layer* aLayer = mLayerManager->GetRoot();
  AutoResolveRefLayers resolve(aLayer);

  TransformShadowTree();

  RenderTraceLayers(aLayer, "0000");

  mLayerManager->EndEmptyTransaction();

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  if (mExpectedComposeTime + TimeDuration::FromMilliseconds(15) < TimeStamp::Now()) {
    printf_stderr("Compositor: Composite took %i ms.\n",
                  15 + (int)(TimeStamp::Now() - mExpectedComposeTime).ToMilliseconds());
  }
#endif
}



Layer*
CompositorParent::GetPrimaryScrollableLayer()
{
  Layer* root = mLayerManager->GetRoot();

  nsTArray<Layer*> queue;
  queue.AppendElement(root);
  while (queue.Length()) {
    ContainerLayer* containerLayer = queue[0]->AsContainerLayer();
    queue.RemoveElementAt(0);
    if (!containerLayer) {
      continue;
    }

    const FrameMetrics& frameMetrics = containerLayer->GetFrameMetrics();
    if (frameMetrics.IsScrollable()) {
      return containerLayer;
    }

    Layer* child = containerLayer->GetFirstChild();
    while (child) {
      queue.AppendElement(child);
      child = child->GetNextSibling();
    }
  }

  return root;
}

static void
Translate2D(gfx3DMatrix& aTransform, const gfxPoint& aOffset)
{
  aTransform._41 += aOffset.x;
  aTransform._42 += aOffset.y;
}

void
CompositorParent::TransformFixedLayers(Layer* aLayer,
                                       const gfxPoint& aTranslation,
                                       const gfxPoint& aScaleDiff)
{
  if (aLayer->GetIsFixedPosition() &&
      !aLayer->GetParent()->GetIsFixedPosition()) {
    
    
    
    const gfxPoint& anchor = aLayer->GetFixedPositionAnchor();
    gfxPoint translation(aTranslation.x - (anchor.x - anchor.x / aScaleDiff.x),
                         aTranslation.y - (anchor.y - anchor.y / aScaleDiff.y));

    gfx3DMatrix layerTransform = aLayer->GetTransform();
    Translate2D(layerTransform, translation);
    ShadowLayer* shadow = aLayer->AsShadowLayer();
    shadow->SetShadowTransform(layerTransform);

    const nsIntRect* clipRect = aLayer->GetClipRect();
    if (clipRect) {
      nsIntRect transformedClipRect(*clipRect);
      transformedClipRect.MoveBy(translation.x, translation.y);
      shadow->SetShadowClipRect(&transformedClipRect);
    }
  }

  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    TransformFixedLayers(child, aTranslation, aScaleDiff);
  }
}



static void
SetShadowProperties(Layer* aLayer)
{
  
  ShadowLayer* shadow = aLayer->AsShadowLayer();
  shadow->SetShadowTransform(aLayer->GetTransform());
  shadow->SetShadowVisibleRegion(aLayer->GetVisibleRegion());
  shadow->SetShadowClipRect(aLayer->GetClipRect());

  for (Layer* child = aLayer->GetFirstChild();
      child; child = child->GetNextSibling()) {
    SetShadowProperties(child);
  }
}

void
CompositorParent::TransformShadowTree()
{
  Layer* layer = GetPrimaryScrollableLayer();
  ShadowLayer* shadow = layer->AsShadowLayer();
  ContainerLayer* container = layer->AsContainerLayer();

  const FrameMetrics& metrics = container->GetFrameMetrics();
  const gfx3DMatrix& rootTransform = mLayerManager->GetRoot()->GetTransform();
  const gfx3DMatrix& currentTransform = layer->GetTransform();

  float rootScaleX = rootTransform.GetXScale();
  float rootScaleY = rootTransform.GetYScale();

  if (mIsFirstPaint) {
    mContentRect = metrics.mContentRect;
    SetFirstPaintViewport(metrics.mViewportScrollOffset,
                          1/rootScaleX,
                          mContentRect,
                          metrics.mCSSContentRect);
    mIsFirstPaint = false;
  } else if (!metrics.mContentRect.IsEqualEdges(mContentRect)) {
    mContentRect = metrics.mContentRect;
    SetPageRect(metrics.mCSSContentRect);
  }

  
  
  
  nsIntRect displayPort = metrics.mDisplayPort;
  nsIntPoint scrollOffset = metrics.mViewportScrollOffset;
  displayPort.x += scrollOffset.x;
  displayPort.y += scrollOffset.y;

  SyncViewportInfo(displayPort, 1/rootScaleX, mLayersUpdated,
                   mScrollOffset, mXScale, mYScale);
  mLayersUpdated = false;

  
  
  
  
  
  
  float tempScaleDiffX = rootScaleX * mXScale;
  float tempScaleDiffY = rootScaleY * mYScale;

  nsIntPoint metricsScrollOffset(0, 0);
  if (metrics.IsScrollable())
    metricsScrollOffset = metrics.mViewportScrollOffset;

  nsIntPoint scrollCompensation(
    (mScrollOffset.x / tempScaleDiffX - metricsScrollOffset.x) * mXScale,
    (mScrollOffset.y / tempScaleDiffY - metricsScrollOffset.y) * mYScale);
  ViewTransform treeTransform(-scrollCompensation, mXScale, mYScale);
  shadow->SetShadowTransform(gfx3DMatrix(treeTransform) * currentTransform);

  
  
  gfxPoint offset;
  gfxPoint scaleDiff;

  
  
  
  if (mContentRect.width * tempScaleDiffX < mWidgetSize.width) {
    offset.x = -metricsScrollOffset.x;
    scaleDiff.x = NS_MIN(1.0f, mWidgetSize.width / (float)mContentRect.width);
  } else {
    offset.x = clamped(mScrollOffset.x / tempScaleDiffX, (float)mContentRect.x,
                       mContentRect.XMost() - mWidgetSize.width / tempScaleDiffX) -
               metricsScrollOffset.x;
    scaleDiff.x = tempScaleDiffX;
  }

  if (mContentRect.height * tempScaleDiffY < mWidgetSize.height) {
    offset.y = -metricsScrollOffset.y;
    scaleDiff.y = NS_MIN(1.0f, mWidgetSize.height / (float)mContentRect.height);
  } else {
    offset.y = clamped(mScrollOffset.y / tempScaleDiffY, (float)mContentRect.y,
                       mContentRect.YMost() - mWidgetSize.height / tempScaleDiffY) -
               metricsScrollOffset.y;
    scaleDiff.y = tempScaleDiffY;
  }

  TransformFixedLayers(layer, offset, scaleDiff);
}

void
CompositorParent::SetFirstPaintViewport(const nsIntPoint& aOffset, float aZoom,
                                        const nsIntRect& aPageRect, const gfx::Rect& aCssPageRect)
{
#ifdef MOZ_WIDGET_ANDROID
  AndroidBridge::Bridge()->SetFirstPaintViewport(aOffset, aZoom, aPageRect, aCssPageRect);
#endif
}

void
CompositorParent::SetPageRect(const gfx::Rect& aCssPageRect)
{
#ifdef MOZ_WIDGET_ANDROID
  AndroidBridge::Bridge()->SetPageRect(aCssPageRect);
#endif
}

void
CompositorParent::SyncViewportInfo(const nsIntRect& aDisplayPort,
                                   float aDisplayResolution, bool aLayersUpdated,
                                   nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY)
{
#ifdef MOZ_WIDGET_ANDROID
  AndroidBridge::Bridge()->SyncViewportInfo(aDisplayPort, aDisplayResolution, aLayersUpdated,
                                            aScrollOffset, aScaleX, aScaleY);
#endif
}

void
CompositorParent::ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                      bool isFirstPaint)
{
  mIsFirstPaint = mIsFirstPaint || isFirstPaint;
  mLayersUpdated = true;
  Layer* root = aLayerTree->GetRoot();
  mLayerManager->SetRoot(root);
  if (root) {
    SetShadowProperties(root);
  }
  ScheduleComposition();
}

PLayersParent*
CompositorParent::AllocPLayers(const LayersBackend& aBackendHint,
                               const uint64_t& aId,
                               LayersBackend* aBackend,
                               int32_t* aMaxTextureSize)
{
  MOZ_ASSERT(aId == 0);

  
  
  nsIntRect rect;
  mWidget->GetBounds(rect);
  mWidgetSize.width = rect.width;
  mWidgetSize.height = rect.height;

  *aBackend = aBackendHint;

  if (aBackendHint == LayerManager::LAYERS_OPENGL) {
    nsRefPtr<LayerManagerOGL> layerManager;
    layerManager =
      new LayerManagerOGL(mWidget, mEGLSurfaceSize.width, mEGLSurfaceSize.height, mRenderToEGLSurface);
    mWidget = NULL;
    mLayerManager = layerManager;
    ShadowLayerManager* shadowManager = layerManager->AsShadowManager();
    if (shadowManager) {
      shadowManager->SetCompositorID(mCompositorID);  
    }
    
    if (!layerManager->Initialize()) {
      NS_ERROR("Failed to init OGL Layers");
      return NULL;
    }

    ShadowLayerManager* slm = layerManager->AsShadowManager();
    if (!slm) {
      return NULL;
    }
    *aMaxTextureSize = layerManager->GetMaxTextureSize();
    return new ShadowLayersParent(slm, this, 0);
  } else if (aBackendHint == LayerManager::LAYERS_BASIC) {
    nsRefPtr<LayerManager> layerManager = new BasicShadowLayerManager(mWidget);
    mWidget = NULL;
    mLayerManager = layerManager;
    ShadowLayerManager* slm = layerManager->AsShadowManager();
    if (!slm) {
      return NULL;
    }
    *aMaxTextureSize = layerManager->GetMaxTextureSize();
    return new ShadowLayersParent(slm, this, 0);
  } else {
    NS_ERROR("Unsupported backend selected for Async Compositor");
    return NULL;
  }
}

bool
CompositorParent::DeallocPLayers(PLayersParent* actor)
{
  delete actor;
  return true;
}


typedef map<PRUint64,CompositorParent*> CompositorMap;
static CompositorMap* sCompositorMap;

void CompositorParent::CreateCompositorMap()
{
  if (sCompositorMap == nsnull) {
    sCompositorMap = new CompositorMap;
  }
}

void CompositorParent::DestroyCompositorMap()
{
  if (sCompositorMap != nsnull) {
    NS_ASSERTION(sCompositorMap->empty(), 
                 "The Compositor map should be empty when destroyed>");
    delete sCompositorMap;
    sCompositorMap = nsnull;
  }
}

CompositorParent* CompositorParent::GetCompositor(PRUint64 id)
{
  CompositorMap::iterator it = sCompositorMap->find(id);
  return it != sCompositorMap->end() ? it->second : nsnull;
}

void CompositorParent::AddCompositor(CompositorParent* compositor, PRUint64* outID)
{
  static PRUint64 sNextID = 1;
  
  ++sNextID;
  (*sCompositorMap)[sNextID] = compositor;
  *outID = sNextID;
}

CompositorParent* CompositorParent::RemoveCompositor(PRUint64 id)
{
  CompositorMap::iterator it = sCompositorMap->find(id);
  if (it == sCompositorMap->end()) {
    return nsnull;
  }
  sCompositorMap->erase(it);
  return it->second;
}

typedef map<uint64_t, RefPtr<Layer> > LayerTreeMap;
static LayerTreeMap sIndirectLayerTrees;

 uint64_t
CompositorParent::AllocateLayerTreeId()
{
  MOZ_ASSERT(CompositorLoop());
  MOZ_ASSERT(NS_IsMainThread());
  static uint64_t ids;
  return ++ids;
}









class CrossProcessCompositorParent : public PCompositorParent,
                                     public ShadowLayersManager
{
  friend class CompositorParent;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CrossProcessCompositorParent)
public:
  CrossProcessCompositorParent() {}
  virtual ~CrossProcessCompositorParent() {}

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  
  virtual bool RecvWillStop() MOZ_OVERRIDE { return true; }
  virtual bool RecvStop() MOZ_OVERRIDE { return true; }
  virtual bool RecvPause() MOZ_OVERRIDE { return true; }
  virtual bool RecvResume() MOZ_OVERRIDE { return true; }

  virtual PLayersParent* AllocPLayers(const LayersBackend& aBackendType,
                                      const uint64_t& aId,
                                      LayersBackend* aBackend,
                                      int32_t* aMaxTextureSize) MOZ_OVERRIDE;
  virtual bool DeallocPLayers(PLayersParent* aLayers) MOZ_OVERRIDE;

  virtual void ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                   bool isFirstPaint) MOZ_OVERRIDE;

private:
  void DeferredDestroy();

  
  
  
  nsRefPtr<CrossProcessCompositorParent> mSelfRef;
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
    new CrossProcessCompositorParent();
  ProcessHandle handle;
  if (!base::OpenProcessHandle(aOtherProcess, &handle)) {
    
    return nsnull;
  }
  cpcp->mSelfRef = cpcp;
  CompositorLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(OpenCompositor, cpcp.get(),
                        aTransport, handle, XRE_GetIOMessageLoop()));
  
  
  return cpcp.get();
}

static void
UpdateIndirectTree(uint64_t aId, Layer* aRoot)
{
  sIndirectLayerTrees[aId] = aRoot;
}


static Layer*
GetIndirectShadowTree(uint64_t aId)
{
  LayerTreeMap::const_iterator cit = sIndirectLayerTrees.find(aId);
  if (sIndirectLayerTrees.end() == cit) {
    return nsnull;
  }
  return cit->second;
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

PLayersParent*
CrossProcessCompositorParent::AllocPLayers(const LayersBackend& aBackendType,
                                           const uint64_t& aId,
                                           LayersBackend* aBackend,
                                           int32_t* aMaxTextureSize)
{
  MOZ_ASSERT(aId != 0);

  nsRefPtr<LayerManager> lm = sCurrentCompositor->GetLayerManager();
  *aBackend = lm->GetBackendType();
  *aMaxTextureSize = lm->GetMaxTextureSize();
  return new ShadowLayersParent(lm->AsShadowManager(), this, aId);
}
 
bool
CrossProcessCompositorParent::DeallocPLayers(PLayersParent* aLayers)
{
  ShadowLayersParent* slp = static_cast<ShadowLayersParent*>(aLayers);
  RemoveIndirectTree(slp->GetId());
  delete aLayers;
  return true;
}

void
CrossProcessCompositorParent::ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                                  bool isFirstPaint)
{
  uint64_t id = aLayerTree->GetId();
  MOZ_ASSERT(id != 0);
  Layer* shadowRoot = aLayerTree->GetRoot();
  if (shadowRoot) {
    SetShadowProperties(shadowRoot);
  }
  UpdateIndirectTree(id, shadowRoot);

  sCurrentCompositor->ScheduleComposition();
}

void
CrossProcessCompositorParent::DeferredDestroy()
{
  mSelfRef = NULL;
  
}

} 
} 

