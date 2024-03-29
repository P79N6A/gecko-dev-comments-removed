





#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/CompositorParent.h"
#include <stddef.h>                     
#include "ClientLayerManager.h"         
#include "base/message_loop.h"          
#include "base/task.h"                  
#include "base/tracked.h"               
#include "mozilla/layers/LayerTransactionChild.h"
#include "mozilla/layers/PLayerTransactionChild.h"
#include "mozilla/mozalloc.h"           
#include "nsDebug.h"                    
#include "nsIObserver.h"                
#include "nsISupportsImpl.h"            
#include "nsTArray.h"                   
#include "nsXULAppAPI.h"                
#include "FrameLayerBuilder.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/unused.h"
#include "mozilla/DebugOnly.h"
#if defined(XP_WIN)
#include "WinUtils.h"
#endif

using mozilla::layers::LayerTransactionChild;
using mozilla::dom::TabChildBase;
using mozilla::unused;

namespace mozilla {
namespace layers {

 CompositorChild* CompositorChild::sCompositor;

Atomic<int32_t> CompositableForwarder::sSerialCounter(0);

CompositorChild::CompositorChild(ClientLayerManager *aLayerManager)
  : mLayerManager(aLayerManager)
  , mCanSend(false)
{
}

CompositorChild::~CompositorChild()
{
  if (mCanSend) {
    gfxCriticalError() << "CompositorChild was not deinitialized";
  }
}

static void DeferredDestroyCompositor(nsRefPtr<CompositorParent> aCompositorParent,
                                      nsRefPtr<CompositorChild> aCompositorChild)
{
    
    
    
}

void
CompositorChild::Destroy()
{
  
  MOZ_ASSERT(mRefCnt != 0);

  if (!mCanSend) {
    return;
  }

  mCanSend = false;

  
  
  
  nsRefPtr<CompositorChild> selfRef = this;

  SendWillStop();
  
  
  
  
  
  
  

  

  if (mLayerManager) {
    mLayerManager->Destroy();
    mLayerManager = nullptr;
  }

  
  
  for (int i = ManagedPLayerTransactionChild().Length() - 1; i >= 0; --i) {
    RefPtr<LayerTransactionChild> layers =
      static_cast<LayerTransactionChild*>(ManagedPLayerTransactionChild()[i]);
    layers->Destroy();
  }

  SendStop();

  
  
  MessageLoop::current()->PostTask(FROM_HERE,
             NewRunnableFunction(DeferredDestroyCompositor, mCompositorParent, selfRef));
}

bool
CompositorChild::LookupCompositorFrameMetrics(const FrameMetrics::ViewID aId,
                                              FrameMetrics& aFrame)
{
  SharedFrameMetricsData* data = mFrameMetricsTable.Get(aId);
  if (data) {
    data->CopyFrameMetrics(&aFrame);
    return true;
  }
  return false;
}

 PCompositorChild*
CompositorChild::Create(Transport* aTransport, ProcessId aOtherPid)
{
  
  MOZ_ASSERT(!sCompositor);

  nsRefPtr<CompositorChild> child(new CompositorChild(nullptr));
  if (!child->Open(aTransport, aOtherPid, XRE_GetIOMessageLoop(), ipc::ChildSide)) {
    NS_RUNTIMEABORT("Couldn't Open() Compositor channel.");
    return nullptr;
  }

  child->mCanSend = true;

  
  sCompositor = child.forget().take();

  int32_t width;
  int32_t height;
  sCompositor->SendGetTileSize(&width, &height);
  gfxPlatform::GetPlatform()->SetTileSize(width, height);

  
  return sCompositor;
}

bool
CompositorChild::OpenSameProcess(CompositorParent* aParent)
{
  MOZ_ASSERT(aParent);

  mCompositorParent = aParent;
  mCanSend = Open(mCompositorParent->GetIPCChannel(),
                  CompositorParent::CompositorLoop(),
                  ipc::ChildSide);
  return mCanSend;
}

 CompositorChild*
CompositorChild::Get()
{
  
  MOZ_ASSERT(!XRE_IsParentProcess());
  return sCompositor;
}

PLayerTransactionChild*
CompositorChild::AllocPLayerTransactionChild(const nsTArray<LayersBackend>& aBackendHints,
                                             const uint64_t& aId,
                                             TextureFactoryIdentifier*,
                                             bool*)
{
  MOZ_ASSERT(mCanSend);
  LayerTransactionChild* c = new LayerTransactionChild(aId);
  c->AddIPDLReference();
  return c;
}

 PLDHashOperator
CompositorChild::RemoveSharedMetricsForLayersId(const uint64_t& aKey,
                                                nsAutoPtr<SharedFrameMetricsData>& aData,
                                                void* aLayerTransactionChild)
{
  uint64_t childId = static_cast<LayerTransactionChild*>(aLayerTransactionChild)->GetId();
  if (aData->GetLayersId() == childId) {
    return PLDHashOperator::PL_DHASH_REMOVE;
  }
  return PLDHashOperator::PL_DHASH_NEXT;
}

bool
CompositorChild::DeallocPLayerTransactionChild(PLayerTransactionChild* actor)
{
  mFrameMetricsTable.Enumerate(RemoveSharedMetricsForLayersId, actor);
  static_cast<LayerTransactionChild*>(actor)->ReleaseIPDLReference();
  return true;
}

bool
CompositorChild::RecvInvalidateAll()
{
  if (mLayerManager) {
    FrameLayerBuilder::InvalidateAllLayers(mLayerManager);
  }
  return true;
}

#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK)
static void CalculatePluginClip(const gfx::IntRect& aBounds,
                                const nsTArray<gfx::IntRect>& aPluginClipRects,
                                const nsIntPoint& aContentOffset,
                                const nsIntRegion& aParentLayerVisibleRegion,
                                nsTArray<gfx::IntRect>& aResult,
                                gfx::IntRect& aVisibleBounds,
                                bool& aPluginIsVisible)
{
  aPluginIsVisible = true;
  
  nsIntRegion contentVisibleRegion(aBounds);
  
  for (uint32_t idx = 0; idx < aPluginClipRects.Length(); idx++) {
    gfx::IntRect rect = aPluginClipRects[idx];
    
    rect.MoveBy(aBounds.x, aBounds.y);
    contentVisibleRegion.AndWith(rect);
  }
  
  nsIntRegion region = aParentLayerVisibleRegion;
  region.MoveBy(-aContentOffset.x, -aContentOffset.y);
  contentVisibleRegion.AndWith(region);
  if (contentVisibleRegion.IsEmpty()) {
    aPluginIsVisible = false;
    return;
  }
  
  contentVisibleRegion.MoveBy(-aBounds.x, -aBounds.y);
  nsIntRegionRectIterator iter(contentVisibleRegion);
  for (const gfx::IntRect* rgnRect = iter.Next(); rgnRect; rgnRect = iter.Next()) {
    aResult.AppendElement(*rgnRect);
    aVisibleBounds.UnionRect(aVisibleBounds, *rgnRect);
  }
}
#endif

bool
CompositorChild::RecvUpdatePluginConfigurations(const nsIntPoint& aContentOffset,
                                                const nsIntRegion& aParentLayerVisibleRegion,
                                                nsTArray<PluginWindowData>&& aPlugins)
{
#if !defined(XP_WIN) && !defined(MOZ_WIDGET_GTK)
  NS_NOTREACHED("CompositorChild::RecvUpdatePluginConfigurations calls "
                "unexpected on this platform.");
  return false;
#else
  
  
  
  DebugOnly<nsresult> rv;
  MOZ_ASSERT(NS_IsMainThread());

  
  nsTArray<uintptr_t> visiblePluginIds;
  nsIWidget* parent = nullptr;
  for (uint32_t pluginsIdx = 0; pluginsIdx < aPlugins.Length(); pluginsIdx++) {
    nsIWidget* widget =
      nsIWidget::LookupRegisteredPluginWindow(aPlugins[pluginsIdx].windowId());
    if (!widget) {
      NS_WARNING("Unexpected, plugin id not found!");
      continue;
    }
    if (!parent) {
      parent = widget->GetParent();
    }
    bool isVisible = aPlugins[pluginsIdx].visible();
    if (widget && !widget->Destroyed()) {
      gfx::IntRect bounds;
      gfx::IntRect visibleBounds;
      
      if (isVisible) {
        
        
        
        bounds = aPlugins[pluginsIdx].bounds();
        rv = widget->Resize(aContentOffset.x + bounds.x,
                            aContentOffset.y + bounds.y,
                            bounds.width, bounds.height, false);
        NS_ASSERTION(NS_SUCCEEDED(rv), "widget call failure");
        nsTArray<gfx::IntRect> rectsOut;
        
        CalculatePluginClip(bounds, aPlugins[pluginsIdx].clip(),
                            aContentOffset, aParentLayerVisibleRegion,
                            rectsOut, visibleBounds, isVisible);
        
        rv = widget->SetWindowClipRegion(rectsOut, false);
        NS_ASSERTION(NS_SUCCEEDED(rv), "widget call failure");
      }

      rv = widget->Enable(isVisible);
      NS_ASSERTION(NS_SUCCEEDED(rv), "widget call failure");

      
      rv = widget->Show(isVisible);
      NS_ASSERTION(NS_SUCCEEDED(rv), "widget call failure");

      
      if (isVisible) {
        
        gfx::IntRect bounds = aPlugins[pluginsIdx].bounds();
        gfx::IntRect rect(0, 0, bounds.width, bounds.height);
#if defined(XP_WIN)
        
        
        
        mozilla::widget::WinUtils::InvalidatePluginAsWorkaround(widget, visibleBounds);
#else
        rv = widget->Invalidate(visibleBounds);
        NS_ASSERTION(NS_SUCCEEDED(rv), "widget call failure");
#endif
        visiblePluginIds.AppendElement(aPlugins[pluginsIdx].windowId());
      }
    }
  }
  
  
  nsIWidget::UpdateRegisteredPluginWindowVisibility((uintptr_t)parent, visiblePluginIds);
  return true;
#endif 
}

bool
CompositorChild::RecvUpdatePluginVisibility(const uintptr_t& aOwnerWidget,
                                            nsTArray<uintptr_t>&& aVisibleIdList)
{
#if !defined(XP_WIN) && !defined(MOZ_WIDGET_GTK)
  NS_NOTREACHED("CompositorChild::RecvUpdatePluginVisibility calls "
                "unexpected on this platform.");
  return false;
#else
  MOZ_ASSERT(NS_IsMainThread());
  nsIWidget::UpdateRegisteredPluginWindowVisibility(aOwnerWidget, aVisibleIdList);
  return true;
#endif 
}

bool
CompositorChild::RecvDidComposite(const uint64_t& aId, const uint64_t& aTransactionId)
{
  if (mLayerManager) {
    MOZ_ASSERT(aId == 0);
    mLayerManager->DidComposite(aTransactionId);
  } else if (aId != 0) {
    dom::TabChild *child = dom::TabChild::GetFrom(aId);
    if (child) {
      child->DidComposite(aTransactionId);
    }
  }
  return true;
}

bool
CompositorChild::RecvOverfill(const uint32_t &aOverfill)
{
  for (size_t i = 0; i < mOverfillObservers.Length(); i++) {
    mOverfillObservers[i]->RunOverfillCallback(aOverfill);
  }
  mOverfillObservers.Clear();
  return true;
}

void
CompositorChild::AddOverfillObserver(ClientLayerManager* aLayerManager)
{
  MOZ_ASSERT(aLayerManager);
  mOverfillObservers.AppendElement(aLayerManager);
}

bool
CompositorChild::RecvClearCachedResources(const uint64_t& aId)
{
  dom::TabChild* child = dom::TabChild::GetFrom(aId);
  if (child) {
    child->ClearCachedResources();
  }
  return true;
}

void
CompositorChild::ActorDestroy(ActorDestroyReason aWhy)
{
  MOZ_ASSERT(!mCanSend);
  MOZ_ASSERT(sCompositor == this);

#ifdef MOZ_B2G
  
  
  
  
  if (aWhy == AbnormalShutdown) {
    NS_RUNTIMEABORT("ActorDestroy by IPC channel failure at CompositorChild");
  }
#endif

  MessageLoop::current()->PostTask(
    FROM_HERE,
    NewRunnableMethod(this, &CompositorChild::Release));
}

bool
CompositorChild::RecvSharedCompositorFrameMetrics(
    const mozilla::ipc::SharedMemoryBasic::Handle& metrics,
    const CrossProcessMutexHandle& handle,
    const uint64_t& aLayersId,
    const uint32_t& aAPZCId)
{
  SharedFrameMetricsData* data = new SharedFrameMetricsData(
    metrics, handle, aLayersId, aAPZCId);
  mFrameMetricsTable.Put(data->GetViewID(), data);
  return true;
}

bool
CompositorChild::RecvReleaseSharedCompositorFrameMetrics(
    const ViewID& aId,
    const uint32_t& aAPZCId)
{
  SharedFrameMetricsData* data = mFrameMetricsTable.Get(aId);
  
  
  
  if (data && (data->GetAPZCId() == aAPZCId)) {
    mFrameMetricsTable.Remove(aId);
  }
  return true;
}

CompositorChild::SharedFrameMetricsData::SharedFrameMetricsData(
    const ipc::SharedMemoryBasic::Handle& metrics,
    const CrossProcessMutexHandle& handle,
    const uint64_t& aLayersId,
    const uint32_t& aAPZCId)
  : mMutex(nullptr)
  , mLayersId(aLayersId)
  , mAPZCId(aAPZCId)
{
  mBuffer = new ipc::SharedMemoryBasic(metrics);
  mBuffer->Map(sizeof(FrameMetrics));
  mMutex = new CrossProcessMutex(handle);
  MOZ_COUNT_CTOR(SharedFrameMetricsData);
}

CompositorChild::SharedFrameMetricsData::~SharedFrameMetricsData()
{
  
  
  delete mMutex;
  mBuffer = nullptr;
  MOZ_COUNT_DTOR(SharedFrameMetricsData);
}

void
CompositorChild::SharedFrameMetricsData::CopyFrameMetrics(FrameMetrics* aFrame)
{
  FrameMetrics* frame = static_cast<FrameMetrics*>(mBuffer->memory());
  MOZ_ASSERT(frame);
  mMutex->Lock();
  *aFrame = *frame;
  mMutex->Unlock();
}

FrameMetrics::ViewID
CompositorChild::SharedFrameMetricsData::GetViewID()
{
  FrameMetrics* frame = static_cast<FrameMetrics*>(mBuffer->memory());
  MOZ_ASSERT(frame);
  
  
  return frame->GetScrollId();
}

uint64_t
CompositorChild::SharedFrameMetricsData::GetLayersId() const
{
  return mLayersId;
}

uint32_t
CompositorChild::SharedFrameMetricsData::GetAPZCId()
{
  return mAPZCId;
}


bool
CompositorChild::RecvRemotePaintIsReady()
{
  
  
  
  
  MOZ_LAYERS_LOG(("[RemoteGfx] CompositorChild received RemotePaintIsReady"));
  nsRefPtr<nsISupports> iTabChildBase(do_QueryReferent(mWeakTabChild));
  if (!iTabChildBase) {
    MOZ_LAYERS_LOG(("[RemoteGfx] Note: TabChild was released before RemotePaintIsReady. "
        "MozAfterRemotePaint will not be sent to listener."));
    return true;
  }
  TabChildBase* tabChildBase = static_cast<TabChildBase*>(iTabChildBase.get());
  TabChild* tabChild = static_cast<TabChild*>(tabChildBase);
  MOZ_ASSERT(tabChild);
  unused << tabChild->SendRemotePaintIsReady();
  mWeakTabChild = nullptr;
  return true;
}


void
CompositorChild::RequestNotifyAfterRemotePaint(TabChild* aTabChild)
{
  MOZ_ASSERT(aTabChild, "NULL TabChild not allowed in CompositorChild::RequestNotifyAfterRemotePaint");
  mWeakTabChild = do_GetWeakReference( static_cast<dom::TabChildBase*>(aTabChild) );
  unused << SendRequestNotifyAfterRemotePaint();
}

void
CompositorChild::CancelNotifyAfterRemotePaint(TabChild* aTabChild)
{
  nsRefPtr<nsISupports> iTabChildBase(do_QueryReferent(mWeakTabChild));
  if (!iTabChildBase) {
    return;
  }
  TabChildBase* tabChildBase = static_cast<TabChildBase*>(iTabChildBase.get());
  TabChild* tabChild = static_cast<TabChild*>(tabChildBase);
  if (tabChild == aTabChild) {
    mWeakTabChild = nullptr;
  }
}

bool
CompositorChild::SendWillStop()
{
  return PCompositorChild::SendWillStop();
}

bool
CompositorChild::SendPause()
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendPause();
}

bool
CompositorChild::SendResume()
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendResume();
}

bool
CompositorChild::SendNotifyHidden(const uint64_t& id)
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendNotifyHidden(id);
}

bool
CompositorChild::SendNotifyVisible(const uint64_t& id)
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendNotifyVisible(id);
}

bool
CompositorChild::SendNotifyChildCreated(const uint64_t& id)
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendNotifyChildCreated(id);
}

bool
CompositorChild::SendAdoptChild(const uint64_t& id)
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendAdoptChild(id);
}

bool
CompositorChild::SendMakeSnapshot(const SurfaceDescriptor& inSnapshot, const gfx::IntRect& dirtyRect)
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendMakeSnapshot(inSnapshot, dirtyRect);
}

bool
CompositorChild::SendFlushRendering()
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendFlushRendering();
}

bool
CompositorChild::SendGetTileSize(int32_t* tileWidth, int32_t* tileHeight)
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendGetTileSize(tileWidth, tileHeight);
}

bool
CompositorChild::SendStartFrameTimeRecording(const int32_t& bufferSize, uint32_t* startIndex)
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendStartFrameTimeRecording(bufferSize, startIndex);
}

bool
CompositorChild::SendStopFrameTimeRecording(const uint32_t& startIndex, nsTArray<float>* intervals)
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendStopFrameTimeRecording(startIndex, intervals);
}

bool
CompositorChild::SendNotifyRegionInvalidated(const nsIntRegion& region)
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendNotifyRegionInvalidated(region);
}

bool
CompositorChild::SendRequestNotifyAfterRemotePaint()
{
  MOZ_ASSERT(mCanSend);
  if (!mCanSend) {
    return true;
  }
  return PCompositorChild::SendRequestNotifyAfterRemotePaint();
}


} 
} 

