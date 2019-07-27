





#include "ImageBridgeParent.h"
#include <stdint.h>                     
#include "CompositableHost.h"           
#include "base/message_loop.h"          
#include "base/process.h"               
#include "base/task.h"                  
#include "base/tracked.h"               
#include "mozilla/gfx/Point.h"                   
#include "mozilla/Hal.h"                
#include "mozilla/HalTypes.h"           
#include "mozilla/ipc/MessageChannel.h" 
#include "mozilla/ipc/ProtocolUtils.h"
#include "mozilla/ipc/Transport.h"      
#include "mozilla/media/MediaSystemResourceManagerParent.h" 
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
using namespace mozilla::media;

std::map<base::ProcessId, ImageBridgeParent*> ImageBridgeParent::sImageBridges;

MessageLoop* ImageBridgeParent::sMainLoop = nullptr;


CompositorThreadHolder* GetCompositorThreadHolder();

ImageBridgeParent::ImageBridgeParent(MessageLoop* aLoop,
                                     Transport* aTransport,
                                     ProcessId aChildProcessId)
  : mMessageLoop(aLoop)
  , mTransport(aTransport)
  , mSetChildThreadPriority(false)
  , mCompositorThreadHolder(GetCompositorThreadHolder())
{
  MOZ_ASSERT(NS_IsMainThread());
  sMainLoop = MessageLoop::current();

  
  SetMessageLoopToPostDestructionTo(sMainLoop);

  
  
  CompositableMap::Create();
  sImageBridges[aChildProcessId] = this;
  SetOtherProcessId(aChildProcessId);
}

ImageBridgeParent::~ImageBridgeParent()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mTransport) {
    MOZ_ASSERT(XRE_GetIOMessageLoop());
    XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                     new DeleteTask<Transport>(mTransport));
  }

  nsTArray<PImageContainerParent*> parents;
  ManagedPImageContainerParent(parents);
  for (PImageContainerParent* p : parents) {
    delete p;
  }

  sImageBridges.erase(OtherPid());
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
ImageBridgeParent::RecvImageBridgeThreadId(const PlatformThreadId& aThreadId)
{
  MOZ_ASSERT(!mSetChildThreadPriority);
  if (mSetChildThreadPriority) {
    return false;
  }
  mSetChildThreadPriority = true;
#ifdef MOZ_WIDGET_GONK
  hal::SetThreadPriority(aThreadId, hal::THREAD_PRIORITY_COMPOSITOR);
#endif
  return true;
}

class MOZ_STACK_CLASS AutoImageBridgeParentAsyncMessageSender
{
public:
  explicit AutoImageBridgeParentAsyncMessageSender(ImageBridgeParent* aImageBridge)
    : mImageBridge(aImageBridge) {}

  ~AutoImageBridgeParentAsyncMessageSender()
  {
    mImageBridge->SendPendingAsyncMessages();
  }
private:
  ImageBridgeParent* mImageBridge;
};

bool
ImageBridgeParent::RecvUpdate(EditArray&& aEdits, EditReplyArray* aReply)
{
  AutoImageBridgeParentAsyncMessageSender autoAsyncMessageSender(this);

  
  
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
ImageBridgeParent::RecvUpdateNoSwap(EditArray&& aEdits)
{
  InfallibleTArray<EditReply> noReplies;
  bool success = RecvUpdate(Move(aEdits), &noReplies);
  MOZ_ASSERT(noReplies.Length() == 0, "RecvUpdateNoSwap requires a sync Update to carry Edits");
  return success;
}

static void
ConnectImageBridgeInParentProcess(ImageBridgeParent* aBridge,
                                  Transport* aTransport,
                                  base::ProcessId aOtherPid)
{
  aBridge->Open(aTransport, aOtherPid, XRE_GetIOMessageLoop(), ipc::ParentSide);
}

 PImageBridgeParent*
ImageBridgeParent::Create(Transport* aTransport, ProcessId aChildProcessId)
{
  MessageLoop* loop = CompositorParent::CompositorLoop();
  nsRefPtr<ImageBridgeParent> bridge = new ImageBridgeParent(loop, aTransport, aChildProcessId);
  bridge->mSelfRef = bridge;
  loop->PostTask(FROM_HERE,
                 NewRunnableFunction(ConnectImageBridgeInParentProcess,
                                     bridge.get(), aTransport, aChildProcessId));
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

static void
ReleaseImageBridgeParent(ImageBridgeParent* aImageBridgeParent)
{
  RELEASE_MANUALLY(aImageBridgeParent);
}

bool ImageBridgeParent::RecvStop()
{
  
  

  
  
  
  
  ADDREF_MANUALLY(this);
  MessageLoop::current()->PostTask(
    FROM_HERE,
    NewRunnableFunction(&ReleaseImageBridgeParent, this));
  return true;
}

static  uint64_t GenImageContainerID() {
  static uint64_t sNextImageID = 1;

  ++sNextImageID;
  return sNextImageID;
}

PCompositableParent*
ImageBridgeParent::AllocPCompositableParent(const TextureInfo& aInfo,
                                            PImageContainerParent* aImageContainer,
                                            uint64_t* aID)
{
  uint64_t id = GenImageContainerID();
  *aID = id;
  return CompositableHost::CreateIPDLActor(this, aInfo, id, aImageContainer);
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

PMediaSystemResourceManagerParent*
ImageBridgeParent::AllocPMediaSystemResourceManagerParent()
{
  return new mozilla::media::MediaSystemResourceManagerParent();
}

bool
ImageBridgeParent::DeallocPMediaSystemResourceManagerParent(PMediaSystemResourceManagerParent* aActor)
{
  MOZ_ASSERT(aActor);
  delete static_cast<mozilla::media::MediaSystemResourceManagerParent*>(aActor);
  return true;
}

PImageContainerParent*
ImageBridgeParent::AllocPImageContainerParent()
{
  return new ImageContainerParent();
}

bool
ImageBridgeParent::DeallocPImageContainerParent(PImageContainerParent* actor)
{
  delete actor;
  return true;
}

void
ImageBridgeParent::SendAsyncMessage(const InfallibleTArray<AsyncParentMessageData>& aMessage)
{
  mozilla::unused << SendParentAsyncMessages(aMessage);
}

bool
ImageBridgeParent::RecvChildAsyncMessages(InfallibleTArray<AsyncChildMessageData>&& aMessages)
{
  return true;
}

class ProcessIdComparator
{
public:
  bool Equals(const ImageCompositeNotification& aA,
              const ImageCompositeNotification& aB) const
  {
    return aA.imageContainerParent()->OtherPid() == aB.imageContainerParent()->OtherPid();
  }
  bool LessThan(const ImageCompositeNotification& aA,
                const ImageCompositeNotification& aB) const
  {
    return aA.imageContainerParent()->OtherPid() < aB.imageContainerParent()->OtherPid();
  }
};

 bool
ImageBridgeParent::NotifyImageComposites(nsTArray<ImageCompositeNotification>& aNotifications)
{
  
  
  aNotifications.Sort(ProcessIdComparator());
  uint32_t i = 0;
  bool ok = true;
  while (i < aNotifications.Length()) {
    nsAutoTArray<ImageCompositeNotification,1> notifications;
    notifications.AppendElement(aNotifications[i]);
    uint32_t end = i + 1;
    ProcessId pid = aNotifications[i].imageContainerParent()->OtherPid();
    while (end < aNotifications.Length() &&
           aNotifications[end].imageContainerParent()->OtherPid() == pid) {
      notifications.AppendElement(aNotifications[end]);
      ++end;
    }
    if (!GetInstance(pid)->SendDidComposite(notifications)) {
      ok = false;
    }
    i = end;
  }
  return ok;
}

MessageLoop * ImageBridgeParent::GetMessageLoop() const {
  return mMessageLoop;
}

void
ImageBridgeParent::DeferredDestroy()
{
  mCompositorThreadHolder = nullptr;
  mSelfRef = nullptr;
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
  return OtherPid() == base::GetCurrentProcId();
}

void
ImageBridgeParent::ReplyRemoveTexture(const OpReplyRemoveTexture& aReply)
{
  mPendingAsyncMessage.push_back(aReply);
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
ImageBridgeParent::SendFenceHandleIfPresent(PTextureParent* aTexture,
                                            CompositableHost* aCompositableHost)
{
  RefPtr<TextureHost> texture = TextureHost::AsTextureHost(aTexture);
  if (!texture) {
    return;
  }

  
  if (aCompositableHost && aCompositableHost->GetCompositor()) {
    FenceHandle fence = aCompositableHost->GetCompositor()->GetReleaseFence();
    if (fence.IsValid()) {
      mPendingAsyncMessage.push_back(OpDeliverFence(aTexture, nullptr,
                                                    fence));
    }
  }

  
  FenceHandle fence = texture->GetAndResetReleaseFenceHandle();
  if (fence.IsValid()) {
    mPendingAsyncMessage.push_back(OpDeliverFence(aTexture, nullptr,
                                                  fence));
  }
}

void
ImageBridgeParent::AppendDeliverFenceMessage(uint64_t aDestHolderId,
                                             uint64_t aTransactionId,
                                             PTextureParent* aTexture,
                                             CompositableHost* aCompositableHost)
{
  RefPtr<TextureHost> texture = TextureHost::AsTextureHost(aTexture);
  if (!texture) {
    return;
  }

  
  if (aCompositableHost && aCompositableHost->GetCompositor()) {
    FenceHandle fence = aCompositableHost->GetCompositor()->GetReleaseFence();
    if (fence.IsValid()) {
      mPendingAsyncMessage.push_back(OpDeliverFenceToTracker(aDestHolderId,
                                                             aTransactionId,
                                                             fence));
    }
  }

  
  FenceHandle fence = texture->GetAndResetReleaseFenceHandle();
  if (fence.IsValid()) {
    mPendingAsyncMessage.push_back(OpDeliverFenceToTracker(aDestHolderId,
                                                           aTransactionId,
                                                           fence));
  }
}

 void
ImageBridgeParent::AppendDeliverFenceMessage(base::ProcessId aChildProcessId,
                                             uint64_t aDestHolderId,
                                             uint64_t aTransactionId,
                                             PTextureParent* aTexture,
                                             CompositableHost* aCompositableHost)
{
  ImageBridgeParent* imageBridge = ImageBridgeParent::GetInstance(aChildProcessId);
  if (!imageBridge) {
    return;
  }
  imageBridge->AppendDeliverFenceMessage(aDestHolderId,
                                         aTransactionId,
                                         aTexture,
                                         aCompositableHost);
}

 void
ImageBridgeParent::SendPendingAsyncMessages(base::ProcessId aChildProcessId)
{
  ImageBridgeParent* imageBridge = ImageBridgeParent::GetInstance(aChildProcessId);
  if (!imageBridge) {
    return;
  }
  imageBridge->SendPendingAsyncMessages();
}

} 
} 
