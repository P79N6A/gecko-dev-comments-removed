






































#include "CompositorChild.h"
#include "CompositorParent.h"
#include "mozilla/layers/ShadowLayersChild.h"
#include "base/thread.h"

using mozilla::layers::ShadowLayersChild;

namespace mozilla {
namespace layers {

CompositorChild::CompositorChild(Thread *aCompositorThread)
  : mCompositorThread(aCompositorThread)
{

  MOZ_COUNT_CTOR(CompositorChild);
}

CompositorChild::~CompositorChild()
{
  MOZ_COUNT_DTOR(CompositorChild);
}

CompositorChild*
CompositorChild::CreateCompositor()
{
  Thread* compositorThread = new Thread("CompositorThread");
  if (compositorThread->Start()) {
    MessageLoop *parentMessageLoop = MessageLoop::current();
    MessageLoop *childMessageLoop = compositorThread->message_loop();
    CompositorParent *compositorParent = new CompositorParent();
    CompositorChild *compositorChild = new CompositorChild(compositorThread);
    mozilla::ipc::AsyncChannel *parentChannel =
      compositorParent->GetIPCChannel();
    mozilla::ipc::AsyncChannel *childChannel =
      compositorChild->GetIPCChannel();
    mozilla::ipc::AsyncChannel::Side childSide =
      mozilla::ipc::AsyncChannel::Child;

    compositorChild->Open(parentChannel, childMessageLoop, childSide);
    compositorChild->CallInit();
    return compositorChild;
  }

  return NULL;
}

PLayersChild*
CompositorChild::AllocPLayers(const LayersBackend &backend, const WidgetDescriptor &widget)
{
  return new ShadowLayersChild();
}

bool
CompositorChild::DeallocPLayers(PLayersChild* actor)
{
  delete actor;
  return true;
}

} 
} 

