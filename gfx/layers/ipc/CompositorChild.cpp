





#include "CompositorChild.h"
#include <stddef.h>                     
#include "ClientLayerManager.h"         
#include "base/message_loop.h"          
#include "base/process_util.h"          
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

using mozilla::layers::LayerTransactionChild;
using mozilla::dom::TabChildBase;
using mozilla::unused;

namespace mozilla {
namespace layers {

 CompositorChild* CompositorChild::sCompositor;

Atomic<int32_t> CompositableForwarder::sSerialCounter(0);

CompositorChild::CompositorChild(ClientLayerManager *aLayerManager)
  : mLayerManager(aLayerManager)
{
  MOZ_COUNT_CTOR(CompositorChild);
}

CompositorChild::~CompositorChild()
{
  MOZ_COUNT_DTOR(CompositorChild);
}

void
CompositorChild::Destroy()
{
  mLayerManager->Destroy();
  mLayerManager = nullptr;
  while (size_t len = ManagedPLayerTransactionChild().Length()) {
    RefPtr<LayerTransactionChild> layers =
      static_cast<LayerTransactionChild*>(ManagedPLayerTransactionChild()[len - 1]);
    layers->Destroy();
  }
  SendStop();
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
CompositorChild::Create(Transport* aTransport, ProcessId aOtherProcess)
{
  
  MOZ_ASSERT(!sCompositor);

  nsRefPtr<CompositorChild> child(new CompositorChild(nullptr));
  ProcessHandle handle;
  if (!base::OpenProcessHandle(aOtherProcess, &handle)) {
    
    NS_RUNTIMEABORT("Couldn't OpenProcessHandle() to parent process.");
    return nullptr;
  }
  if (!child->Open(aTransport, handle, XRE_GetIOMessageLoop(), ipc::ChildSide)) {
    NS_RUNTIMEABORT("Couldn't Open() Compositor channel.");
    return nullptr;
  }
  
  return sCompositor = child.forget().take();
}

 CompositorChild*
CompositorChild::Get()
{
  
  MOZ_ASSERT(XRE_GetProcessType() != GeckoProcessType_Default);
  return sCompositor;
}

PLayerTransactionChild*
CompositorChild::AllocPLayerTransactionChild(const nsTArray<LayersBackend>& aBackendHints,
                                             const uint64_t& aId,
                                             TextureFactoryIdentifier*,
                                             bool*)
{
  LayerTransactionChild* c = new LayerTransactionChild();
  c->AddIPDLReference();
  return c;
}

bool
CompositorChild::DeallocPLayerTransactionChild(PLayerTransactionChild* actor)
{
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

void
CompositorChild::ActorDestroy(ActorDestroyReason aWhy)
{
  MOZ_ASSERT(sCompositor == this);

#ifdef MOZ_B2G
  
  
  
  
  if (aWhy == AbnormalShutdown) {
    NS_RUNTIMEABORT("ActorDestroy by IPC channel failure at CompositorChild");
  }
#endif
  if (sCompositor) {
    sCompositor->Release();
    sCompositor = nullptr;
  }
  
  
  
  
  MessageLoop::current()->PostTask(
    FROM_HERE,
    NewRunnableMethod(this, &CompositorChild::Release));
}

bool
CompositorChild::RecvSharedCompositorFrameMetrics(
    const mozilla::ipc::SharedMemoryBasic::Handle& metrics,
    const CrossProcessMutexHandle& handle,
    const uint32_t& aAPZCId)
{
  SharedFrameMetricsData* data = new SharedFrameMetricsData(metrics, handle, aAPZCId);
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
    const uint32_t& aAPZCId) :
    mMutex(nullptr),
    mAPZCId(aAPZCId)
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

} 
} 

