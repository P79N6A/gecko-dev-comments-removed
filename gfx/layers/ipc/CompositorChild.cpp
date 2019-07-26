





#include "CompositorChild.h"
#include "CompositorParent.h"
#include "LayerManagerOGL.h"
#include "mozilla/layers/LayerTransactionChild.h"

using mozilla::layers::LayerTransactionChild;

namespace mozilla {
namespace layers {

 CompositorChild* CompositorChild::sCompositor;

CompositorChild::CompositorChild(LayerManager *aLayerManager)
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
  mLayerManager = NULL;
  while (size_t len = ManagedPLayerTransactionChild().Length()) {
    LayerTransactionChild* layers =
      static_cast<LayerTransactionChild*>(ManagedPLayerTransactionChild()[len - 1]);
    layers->Destroy();
  }
  SendStop();
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
  if (!child->Open(aTransport, handle, XRE_GetIOMessageLoop(),
                AsyncChannel::Child)) {
    NS_RUNTIMEABORT("Couldn't Open() Compositor channel.");
    return nullptr;
  }
  
  return sCompositor = child.forget().get();
}

 PCompositorChild*
CompositorChild::Get()
{
  
  MOZ_ASSERT(XRE_GetProcessType() != GeckoProcessType_Default);
  return sCompositor;
}

PLayerTransactionChild*
CompositorChild::AllocPLayerTransaction(const LayersBackend& aBackendHint,
                                        const uint64_t& aId,
                                        TextureFactoryIdentifier*)
{
  return new LayerTransactionChild();
}

bool
CompositorChild::DeallocPLayerTransaction(PLayerTransactionChild* actor)
{
  delete actor;
  return true;
}

void
CompositorChild::ActorDestroy(ActorDestroyReason aWhy)
{
  MOZ_ASSERT(sCompositor == this);

  if (aWhy == AbnormalShutdown) {
    NS_RUNTIMEABORT("ActorDestroy by IPC channel failure at CompositorChild");
  }

  sCompositor = NULL;
  
  
  
  
  MessageLoop::current()->PostTask(
    FROM_HERE,
    NewRunnableMethod(this, &CompositorChild::Release));
}


} 
} 

