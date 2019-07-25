







































#include "CompositorChild.h"
#include "CompositorParent.h"
#include "Compositor.h"
#include "LayerManagerOGL.h"
#include "mozilla/layers/ShadowLayersChild.h"
#include "base/thread.h"

using mozilla::layers::ShadowLayersChild;
using namespace mozilla::layers::compositor;

namespace mozilla {
namespace layers {

CompositorChild::CompositorChild(Thread *aCompositorThread, LayerManager *aLayerManager)
  : mCompositorThread(aCompositorThread)
  , mLayerManager(aLayerManager)
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
  mLayerManager = NULL;
  size_t numChildren = ManagedPLayersChild().Length();
  NS_ABORT_IF_FALSE(0 == numChildren || 1 == numChildren,
                    "compositor must only have 0 or 1 layer forwarder");

  if (numChildren) {
    ShadowLayersChild* layers =
      static_cast<ShadowLayersChild*>(ManagedPLayersChild()[0]);
    layers->Destroy();
  }
  SendStop();
}

CompositorChild*
CompositorChild::CreateCompositor(LayerManager *aLayerManager,
                                  CompositorParent *aCompositorParent)
{
  Thread* compositorThread = new Thread("CompositorThread");
  if (compositorThread->Start()) {
    MessageLoop *parentMessageLoop = MessageLoop::current();
    MessageLoop *childMessageLoop = compositorThread->message_loop();
    CompositorChild *compositorChild = new CompositorChild(compositorThread, aLayerManager);
    mozilla::ipc::AsyncChannel *parentChannel =
      aCompositorParent->GetIPCChannel();
    mozilla::ipc::AsyncChannel *childChannel =
      compositorChild->GetIPCChannel();
    mozilla::ipc::AsyncChannel::Side childSide =
      mozilla::ipc::AsyncChannel::Child;

    compositorChild->Open(parentChannel, childMessageLoop, childSide);
    compositorChild->SendInit();

    return compositorChild;
  }

  return NULL;
}

bool
CompositorChild::RecvNativeContextCreated(const NativeContext &aNativeContext)
{
  void *nativeContext = (void*)aNativeContext.nativeContext();
  ShadowNativeContextUserData *userData = new ShadowNativeContextUserData(nativeContext);
  mLayerManager->AsShadowManager()->SetUserData(&sShadowNativeContext, userData);
  return true;
}

PLayersChild*
CompositorChild::AllocPLayers(const LayersBackend &backend, const WidgetDescriptor &widget)
{
  return new ShadowLayersChild();;
}

bool
CompositorChild::DeallocPLayers(PLayersChild* actor)
{
  delete actor;
  return true;
}

} 
} 

