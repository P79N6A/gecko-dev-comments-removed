





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
#include "mozilla/layers/TextureHostOGL.h"  
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

namespace mozilla {
namespace layers {

using namespace mozilla::ipc;
using namespace mozilla::gfx;

std::map<base::ProcessId, ImageBridgeParent*> ImageBridgeParent::sImageBridges;

MessageLoop* ImageBridgeParent::sMainLoop = nullptr;

ImageBridgeParent::ImageBridgeParent(MessageLoop* aLoop,
                                     Transport* aTransport,
                                     ProcessId aChildProcessId)
  : mMessageLoop(aLoop)
  , mTransport(aTransport)
  , mChildProcessId(aChildProcessId)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  CompositableMap::Create();
  sImageBridges[aChildProcessId] = this;
  sMainLoop = MessageLoop::current();
}

ImageBridgeParent::~ImageBridgeParent()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mTransport) {
    MOZ_ASSERT(XRE_GetIOMessageLoop());
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new DeleteTask<Transport>(mTransport));
  }

  sImageBridges.erase(mChildProcessId);
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

  if (!IsSameProcess()) {
    
    
    
    LayerManagerComposite::PlatformSyncBeforeReplyUpdate();
  }

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
ImageBridgeParent::Create(Transport* aTransport, ProcessId aChildProcessId)
{
  base::ProcessHandle processHandle;
  if (!base::OpenProcessHandle(aChildProcessId, &processHandle)) {
    return nullptr;
  }

  MessageLoop* loop = CompositorParent::CompositorLoop();
  nsRefPtr<ImageBridgeParent> bridge = new ImageBridgeParent(loop, aTransport, aChildProcessId);
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
  InfallibleTArray<AsyncParentMessageData> messages;
  messages.AppendElement(OpDeliverFence(aTracker->GetId(),
                                        aTexture, nullptr,
                                        aFence));
  mozilla::unused << SendParentAsyncMessages(messages);
}

void
ImageBridgeParent::SendAsyncMessage(const InfallibleTArray<AsyncParentMessageData>& aMessage)
{
  mozilla::unused << SendParentAsyncMessages(aMessage);
}

bool
ImageBridgeParent::RecvChildAsyncMessages(const InfallibleTArray<AsyncChildMessageData>& aMessages)
{
  for (AsyncChildMessageArray::index_type i = 0; i < aMessages.Length(); ++i) {
    const AsyncChildMessageData& message = aMessages[i];

    switch (message.type()) {
      case AsyncChildMessageData::TOpDeliverFenceFromChild: {
        const OpDeliverFenceFromChild& op = message.get_OpDeliverFenceFromChild();
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
        FenceHandle fence = FenceHandle(op.fence());
        PTextureParent* parent = op.textureParent();

        TextureHostOGL* hostOGL = nullptr;
        RefPtr<TextureHost> texture = TextureHost::AsTextureHost(parent);
        if (texture) {
          hostOGL = texture->AsHostOGL();
        }
        if (hostOGL) {
          hostOGL->SetAcquireFence(fence.mFence);
        }
#endif
        
        InfallibleTArray<AsyncParentMessageData> replies;
        replies.AppendElement(OpReplyDeliverFence(op.transactionId()));
        mozilla::unused << SendParentAsyncMessages(replies);
        break;
      }
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

MessageLoop * ImageBridgeParent::GetMessageLoop() const {
  return mMessageLoop;
}

static void
DeferredReleaseImageBridgeParentOnMainThread(ImageBridgeParent* aDyingImageBridgeParent)
{
  aDyingImageBridgeParent->Release();
}

void
ImageBridgeParent::DeferredDestroy()
{
  ImageBridgeParent* self;
  mSelfRef.forget(&self);

  sMainLoop->PostTask(
    FROM_HERE,
    NewRunnableFunction(&DeferredReleaseImageBridgeParentOnMainThread, this));
}

ImageBridgeParent*
ImageBridgeParent::GetInstance(ProcessId aId)
{
  NS_ASSERTION(sImageBridges.count(aId) == 1, "ImageBridgeParent for the process");
  return sImageBridges[aId];
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

void
ImageBridgeParent::ReplyRemoveTexture(const OpReplyRemoveTexture& aReply)
{
  InfallibleTArray<AsyncParentMessageData> messages;
  messages.AppendElement(aReply);
  mozilla::unused << SendParentAsyncMessages(messages);
}

 void
ImageBridgeParent::ReplyRemoveTexture(base::ProcessId aChildProcessId,
                                      const OpReplyRemoveTexture& aReply)
{
  ImageBridgeParent* imageBridge = ImageBridgeParent::GetInstance(aChildProcessId);
  if (!imageBridge) {
    return;
  }
  imageBridge->ReplyRemoveTexture(aReply);
}

 void
ImageBridgeParent::SendFenceHandleToTrackerIfPresent(uint64_t aDestHolderId,
                                                     uint64_t aTransactionId,
                                                     PTextureParent* aTexture)
{
  RefPtr<TextureHost> texture = TextureHost::AsTextureHost(aTexture);
  if (!texture) {
    return;
  }
  FenceHandle fence = texture->GetAndResetReleaseFenceHandle();
  if (!fence.IsValid()) {
    return;
  }

  RefPtr<FenceDeliveryTracker> tracker = new FenceDeliveryTracker(fence);
  HoldUntilComplete(tracker);
  InfallibleTArray<AsyncParentMessageData> messages;
  messages.AppendElement(OpDeliverFenceToTracker(tracker->GetId(),
                                                 aDestHolderId,
                                                 aTransactionId,
                                                 fence));
  mozilla::unused << SendParentAsyncMessages(messages);
}

 void
ImageBridgeParent::SendFenceHandleToTrackerIfPresent(base::ProcessId aChildProcessId,
                                                     uint64_t aDestHolderId,
                                                     uint64_t aTransactionId,
                                                     PTextureParent* aTexture)
{
  ImageBridgeParent* imageBridge = ImageBridgeParent::GetInstance(aChildProcessId);
  if (!imageBridge) {
    return;
  }
  imageBridge->SendFenceHandleToTrackerIfPresent(aDestHolderId,
                                                 aTransactionId,
                                                 aTexture);
}


} 
} 
