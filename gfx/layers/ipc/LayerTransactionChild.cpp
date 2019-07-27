






#include "LayerTransactionChild.h"
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/PCompositableChild.h"  
#include "mozilla/layers/PLayerChild.h"  
#include "mozilla/layers/ShadowLayers.h"  
#include "mozilla/mozalloc.h"           
#include "nsDebug.h"                    
#include "nsTArray.h"                   
#include "mozilla/layers/TextureClient.h"

namespace mozilla {
namespace layers {


void
LayerTransactionChild::Destroy()
{
  if (!IPCOpen()) {
    return;
  }
  
  
  
  
  
  
  mDestroyed = true;
  MOZ_ASSERT(0 == ManagedPLayerChild().Length(),
             "layers should have been cleaned up by now");

  for (size_t i = 0; i < ManagedPTextureChild().Length(); ++i) {
    TextureClient* texture = TextureClient::AsTextureClient(ManagedPTextureChild()[i]);
    if (texture) {
      texture->ForceRemove();
    }
  }

  SendShutdown();
}


PLayerChild*
LayerTransactionChild::AllocPLayerChild()
{
  
  NS_RUNTIMEABORT("not reached");
  return nullptr;
}

bool
LayerTransactionChild::DeallocPLayerChild(PLayerChild* actor)
{
  delete actor;
  return true;
}

PCompositableChild*
LayerTransactionChild::AllocPCompositableChild(const TextureInfo& aInfo)
{
  MOZ_ASSERT(!mDestroyed);
  return CompositableClient::CreateIPDLActor();
}

bool
LayerTransactionChild::DeallocPCompositableChild(PCompositableChild* actor)
{
  return CompositableClient::DestroyIPDLActor(actor);
}

bool
LayerTransactionChild::RecvParentAsyncMessages(InfallibleTArray<AsyncParentMessageData>&& aMessages)
{
  for (AsyncParentMessageArray::index_type i = 0; i < aMessages.Length(); ++i) {
    const AsyncParentMessageData& message = aMessages[i];

    switch (message.type()) {
      case AsyncParentMessageData::TOpDeliverFence: {
        const OpDeliverFence& op = message.get_OpDeliverFence();
        FenceHandle fence = op.fence();
        PTextureChild* child = op.textureChild();

        RefPtr<TextureClient> texture = TextureClient::AsTextureClient(child);
        if (texture) {
          texture->SetReleaseFenceHandle(fence);
        }
        break;
      }
      case AsyncParentMessageData::TOpReplyRemoveTexture: {
        const OpReplyRemoveTexture& op = message.get_OpReplyRemoveTexture();

        AsyncTransactionTrackersHolder::TransactionCompleteted(op.holderId(),
                                                               op.transactionId());
        break;
      }
      default:
        NS_ERROR("unknown AsyncParentMessageData type");
        return false;
    }
  }
  return true;
}

void
LayerTransactionChild::SendFenceHandle(AsyncTransactionTracker* aTracker,
                                       PTextureChild* aTexture,
                                       const FenceHandle& aFence)
{
  InfallibleTArray<AsyncChildMessageData> messages;
  messages.AppendElement(OpDeliverFenceFromChild(nullptr, aTexture,
                                                 aFence));
  SendChildAsyncMessages(messages);
}

void
LayerTransactionChild::ActorDestroy(ActorDestroyReason why)
{
  mDestroyed = true;
  DestroyAsyncTransactionTrackersHolder();
#ifdef MOZ_B2G
  
  
  
  
  if (why == AbnormalShutdown) {
    NS_RUNTIMEABORT("ActorDestroy by IPC channel failure at LayerTransactionChild");
  }
#endif
}

PTextureChild*
LayerTransactionChild::AllocPTextureChild(const SurfaceDescriptor&,
                                          const TextureFlags&)
{
  MOZ_ASSERT(!mDestroyed);
  return TextureClient::CreateIPDLActor();
}

bool
LayerTransactionChild::DeallocPTextureChild(PTextureChild* actor)
{
  return TextureClient::DestroyIPDLActor(actor);
}

}  
}  
