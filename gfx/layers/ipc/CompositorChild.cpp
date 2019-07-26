





#include "CompositorChild.h"
#include "CompositorParent.h"
#include "LayerManagerOGL.h"
#include "mozilla/layers/ShadowLayersChild.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"

using mozilla::layers::ShadowLayersChild;

namespace mozilla {
namespace layers {



class MemoryPressureObserver MOZ_FINAL : public nsIObserver,
                                         public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  explicit MemoryPressureObserver(CompositorChild* cc)
    : mCC(cc)
  {}

private:
  CompositorChild* mCC;
};

NS_IMPL_ISUPPORTS2(MemoryPressureObserver, nsIObserver, nsISupportsWeakReference)

NS_IMETHODIMP
MemoryPressureObserver::Observe(nsISupports *aSubject,
                                const char *aTopic,
                                const PRUnichar *someData)
{
  if (strcmp(aTopic, "memory-pressure") == 0) {
    mCC->SendMemoryPressure();
  }
  return NS_OK;
}

 CompositorChild* CompositorChild::sCompositor;

CompositorChild::CompositorChild(LayerManager *aLayerManager)
  : mLayerManager(aLayerManager)
{
  MOZ_COUNT_CTOR(CompositorChild);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    mMemoryPressureObserver = new MemoryPressureObserver(this);
    obs->AddObserver(mMemoryPressureObserver, "memory-pressure", false);
  }
}

CompositorChild::~CompositorChild()
{
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->RemoveObserver(mMemoryPressureObserver, "memory-pressure");
  }
  MOZ_COUNT_DTOR(CompositorChild);
}

void
CompositorChild::Destroy()
{
  mLayerManager = NULL;
  while (size_t len = ManagedPLayersChild().Length()) {
    ShadowLayersChild* layers =
      static_cast<ShadowLayersChild*>(ManagedPLayersChild()[len - 1]);
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

PLayersChild*
CompositorChild::AllocPLayers(const LayersBackend& aBackendHint,
                              const uint64_t& aId,
                              LayersBackend* aBackend,
                              int* aMaxTextureSize)
{
  return new ShadowLayersChild();
}

bool
CompositorChild::DeallocPLayers(PLayersChild* actor)
{
  delete actor;
  return true;
}

void
CompositorChild::ActorDestroy(ActorDestroyReason aWhy)
{
  MOZ_ASSERT(sCompositor == this);
  sCompositor = NULL;
  
  
  
  
  MessageLoop::current()->PostTask(
    FROM_HERE,
    NewRunnableMethod(this, &CompositorChild::Release));
}


} 
} 

