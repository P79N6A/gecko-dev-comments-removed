





#include "ImageBridgeParent.h"
#include <stdint.h>                     
#include "CompositableHost.h"           
#include "base/message_loop.h"          
#include "base/process.h"               
#include "base/process_util.h"          
#include "base/task.h"                  
#include "base/tracked.h"               
#include "mozilla/gfx/Point.h"                   
#include "mozilla/ipc/MessageChannel.h" 
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/ipc/Transport.h"      
#include "mozilla/layers/CompositableTransactionParent.h"
#include "mozilla/layers/CompositorParent.h"  
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersMessages.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/PCompositableParent.h"
#include "mozilla/layers/PImageBridgeParent.h"
#include "mozilla/layers/Compositor.h"
#include "mozilla/mozalloc.h"           
#include "mozilla/unused.h"
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsTArray.h"                   
#include "nsTArrayForwardDeclare.h"     
#include "nsXULAppAPI.h"                
#include "mozilla/layers/TextureHost.h"
#include "nsThreadUtils.h"

using namespace mozilla::ipc;
using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

ImageBridgeParent::ImageBridgeParent(MessageLoop* aLoop, Transport* aTransport)
  : mMessageLoop(aLoop)
  , mTransport(aTransport)
{
  
  
  CompositableMap::Create();
}

ImageBridgeParent::~ImageBridgeParent()
{
  if (mTransport) {
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new DeleteTask<Transport>(mTransport));
  }
}

LayersBackend
ImageBridgeParent::GetCompositorBackendType() const
{
  return Compositor::GetBackend();
}

void
ImageBridgeParent::ActorDestroy(ActorDestroyReason aWhy)
{
  MessageLoop::current()->PostTask(
    FROM_HERE,
    NewRunnableMethod(this, &ImageBridgeParent::DeferredDestroy));
}

bool
ImageBridgeParent::RecvUpdate(const EditArray& aEdits, EditReplyArray* aReply)
{
  
  
  if (Compositor::GetBackend() == LayersBackend::LAYERS_NONE) {
    return true;
  }

  
  ClearPrevFenceHandles();

  EditReplyVector replyv;
  for (EditArray::index_type i = 0; i < aEdits.Length(); ++i) {
    if (!ReceiveCompositableUpdate(aEdits[i], replyv)) {
      return false;
    }
  }

  aReply->SetCapacity(replyv.size());
  if (replyv.size() > 0) {
    aReply->AppendElements(&replyv.front(), replyv.size());
  }

  
  
  
  LayerManagerComposite::PlatformSyncBeforeReplyUpdate();

  return true;
}

bool
ImageBridgeParent::RecvUpdateNoSwap(const EditArray& aEdits)
{
  InfallibleTArray<EditReply> noReplies;
  bool success = RecvUpdate(aEdits, &noReplies);
  NS_ABORT_IF_FALSE(noReplies.Length() == 0, "RecvUpdateNoSwap requires a sync Update to carry Edits");
  return success;
}

static void
ConnectImageBridgeInParentProcess(ImageBridgeParent* aBridge,
                                  Transport* aTransport,
                                  base::ProcessHandle aOtherProcess)
{
  aBridge->Open(aTransport, aOtherProcess, XRE_GetIOMessageLoop(), ipc::ParentSide);
}

 PImageBridgeParent*
ImageBridgeParent::Create(Transport* aTransport, ProcessId aOtherProcess)
{
  base::ProcessHandle processHandle;
  if (!base::OpenProcessHandle(aOtherProcess, &processHandle)) {
    return nullptr;
  }

  MessageLoop* loop = CompositorParent::CompositorLoop();
  nsRefPtr<ImageBridgeParent> bridge = new ImageBridgeParent(loop, aTransport);
  bridge->mSelfRef = bridge;
  loop->PostTask(FROM_HERE,
                 NewRunnableFunction(ConnectImageBridgeInParentProcess,
                                     bridge.get(), aTransport, processHandle));
  return bridge.get();
}

bool ImageBridgeParent::RecvWillStop()
{
  
  
  
  
  InfallibleTArray<PTextureParent*> textures;
  ManagedPTextureParent(textures);
  for (unsigned int i = 0; i < textures.Length(); ++i) {
    RefPtr<TextureHost> tex = TextureHost::AsTextureHost(textures[i]);
    tex->DeallocateDeviceData();
  }
  return true;
}

bool ImageBridgeParent::RecvStop()
{
  
  
  return true;
}

static  uint64_t GenImageContainerID() {
  static uint64_t sNextImageID = 1;

  ++sNextImageID;
  return sNextImageID;
}

PCompositableParent*
ImageBridgeParent::AllocPCompositableParent(const TextureInfo& aInfo,
                                            uint64_t* aID)
{
  uint64_t id = GenImageContainerID();
  *aID = id;
  return CompositableHost::CreateIPDLActor(this, aInfo, id);
}

bool ImageBridgeParent::DeallocPCompositableParent(PCompositableParent* aActor)
{
  return CompositableHost::DestroyIPDLActor(aActor);
}

PTextureParent*
ImageBridgeParent::AllocPTextureParent(const SurfaceDescriptor& aSharedData,
                                       const TextureFlags& aFlags)
{
  return TextureHost::CreateIPDLActor(this, aSharedData, aFlags);
}

bool
ImageBridgeParent::DeallocPTextureParent(PTextureParent* actor)
{
  return TextureHost::DestroyIPDLActor(actor);
}

void
ImageBridgeParent::SendFenceHandle(AsyncTransactionTracker* aTracker,
                                   PTextureParent* aTexture,
                                   const FenceHandle& aFence)
{
  HoldUntilComplete(aTracker);
  mozilla::unused << SendParentAsyncMessage(OpDeliverFence(aTracker->GetId(),
                                        aTexture, nullptr,
                                        aFence));
}

bool
ImageBridgeParent::RecvChildAsyncMessages(const InfallibleTArray<AsyncChildMessageData>& aMessages)
{
  for (AsyncChildMessageArray::index_type i = 0; i < aMessages.Length(); ++i) {
    const AsyncChildMessageData& message = aMessages[i];

    switch (message.type()) {
      case AsyncChildMessageData::TOpReplyDeliverFence: {
        const OpReplyDeliverFence& op = message.get_OpReplyDeliverFence();
        TransactionCompleteted(op.transactionId());
        break;
      }
      default:
        NS_ERROR("unknown AsyncChildMessageData type");
        return false;
    }
  }
  return true;
}

MessageLoop * ImageBridgeParent::GetMessageLoop() {
  return mMessageLoop;
}

class ReleaseRunnable : public nsRunnable
{
public:
  ReleaseRunnable(ImageBridgeParent* aRef)
    : mRef(aRef)
  {
  }

  NS_IMETHOD Run()
  {
    mRef->Release();
    return NS_OK;
  }

private:
  ImageBridgeParent* mRef;
};

void
ImageBridgeParent::DeferredDestroy()
{
  ImageBridgeParent* self;
  mSelfRef.forget(&self);

  nsCOMPtr<nsIRunnable> runnable = new ReleaseRunnable(self);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
}

IToplevelProtocol*
ImageBridgeParent::CloneToplevel(const InfallibleTArray<ProtocolFdMapping>& aFds,
                                 base::ProcessHandle aPeerProcess,
                                 mozilla::ipc::ProtocolCloneContext* aCtx)
{
  for (unsigned int i = 0; i < aFds.Length(); i++) {
    if (aFds[i].protocolId() == unsigned(GetProtocolId())) {
      Transport* transport = OpenDescriptor(aFds[i].fd(),
                                            Transport::MODE_SERVER);
      PImageBridgeParent* bridge = Create(transport, base::GetProcId(aPeerProcess));
      bridge->CloneManagees(this, aCtx);
      bridge->IToplevelProtocol::SetTransport(transport);
      return bridge;
    }
  }
  return nullptr;
}

bool ImageBridgeParent::IsSameProcess() const
{
  return OtherProcess() == ipc::kInvalidProcessHandle;
}

} 
} 
