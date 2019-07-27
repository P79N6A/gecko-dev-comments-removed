






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
  NS_ABORT_IF_FALSE(0 == ManagedPLayerChild().Length(),
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
        if (mForwarder) {
          mForwarder->HoldTransactionsToRespond(op.transactionId());
        } else {
          
          InfallibleTArray<AsyncChildMessageData> replies;
          replies.AppendElement(OpReplyDeliverFence(op.transactionId()));
          SendChildAsyncMessages(replies);
        }
        break;
      }
      case AsyncParentMessageData::TOpReplyDeliverFence: {
        const OpReplyDeliverFence& op = message.get_OpReplyDeliverFence();
        TransactionCompleteted(op.transactionId());
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
  HoldUntilComplete(aTracker);
  InfallibleTArray<AsyncChildMessageData> messages;
  messages.AppendElement(OpDeliverFenceFromChild(aTracker->GetId(),
                                                 nullptr, aTexture,
                                                 FenceHandleFromChild(aFence)));
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
