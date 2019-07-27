






#include "CompositableTransactionParent.h"
#include "CompositableHost.h"           
#include "CompositorParent.h"           
#include "GLContext.h"                  
#include "Layers.h"                     
#include "RenderTrace.h"                
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/ContentHost.h"  
#include "mozilla/layers/ImageBridgeParent.h" 
#include "mozilla/layers/SharedBufferManagerParent.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/layers/TextureHostOGL.h"  
#include "mozilla/layers/TiledContentHost.h"
#include "mozilla/layers/PaintedLayerComposite.h"
#include "mozilla/mozalloc.h"           
#include "mozilla/unused.h"
#include "nsDebug.h"                    
#include "nsRegion.h"                   

namespace mozilla {
namespace layers {

class ClientTiledLayerBuffer;
class Compositor;

template<typename Op>
CompositableHost* AsCompositable(const Op& op)
{
  return CompositableHost::FromIPDLActor(op.compositableParent());
}












template<typename T>
bool ScheduleComposition(const T& op)
{
  CompositableHost* comp = AsCompositable(op);
  uint64_t id = comp->GetCompositorID();
  if (!comp || !id) {
    return false;
  }
  CompositorParent* cp = CompositorParent::GetCompositor(id);
  if (!cp) {
    return false;
  }
  cp->ScheduleComposition();
  return true;
}

#if defined(DEBUG) || defined(MOZ_WIDGET_GONK)
static bool ValidatePictureRect(const mozilla::gfx::IntSize& aSize,
                                const nsIntRect& aPictureRect)
{
  return nsIntRect(0, 0, aSize.width, aSize.height).Contains(aPictureRect) &&
      !aPictureRect.IsEmpty();
}
#endif

bool
CompositableParentManager::ReceiveCompositableUpdate(const CompositableOperation& aEdit,
                                                     EditReplyVector& replyv)
{
  switch (aEdit.type()) {
    case CompositableOperation::TOpPaintTextureRegion: {
      MOZ_LAYERS_LOG(("[ParentSide] Paint PaintedLayer"));

      const OpPaintTextureRegion& op = aEdit.get_OpPaintTextureRegion();
      CompositableHost* compositable = AsCompositable(op);
      Layer* layer = compositable->GetLayer();
      if (!layer || layer->GetType() != Layer::TYPE_PAINTED) {
        return false;
      }
      PaintedLayerComposite* thebes = static_cast<PaintedLayerComposite*>(layer);

      const ThebesBufferData& bufferData = op.bufferData();

      RenderTraceInvalidateStart(thebes, "FF00FF", op.updatedRegion().GetBounds());

      nsIntRegion frontUpdatedRegion;
      if (!compositable->UpdateThebes(bufferData,
                                      op.updatedRegion(),
                                      thebes->GetValidRegion(),
                                      &frontUpdatedRegion))
      {
        return false;
      }
      replyv.push_back(
        OpContentBufferSwap(op.compositableParent(), nullptr, frontUpdatedRegion));

      RenderTraceInvalidateEnd(thebes, "FF00FF");
      break;
    }
    case CompositableOperation::TOpUseTiledLayerBuffer: {
      MOZ_LAYERS_LOG(("[ParentSide] Paint TiledLayerBuffer"));
      const OpUseTiledLayerBuffer& op = aEdit.get_OpUseTiledLayerBuffer();
      TiledContentHost* compositable = AsCompositable(op)->AsTiledContentHost();

      NS_ASSERTION(compositable, "The compositable is not tiled");

      const SurfaceDescriptorTiles& tileDesc = op.tileLayerDescriptor();
      bool success = compositable->UseTiledLayerBuffer(this, tileDesc);
      if (!success) {
        return false;
      }
      break;
    }
    case CompositableOperation::TOpRemoveTexture: {
      const OpRemoveTexture& op = aEdit.get_OpRemoveTexture();
      CompositableHost* compositable = AsCompositable(op);
      RefPtr<TextureHost> tex = TextureHost::AsTextureHost(op.textureParent());

      MOZ_ASSERT(tex.get());
      compositable->RemoveTextureHost(tex);
      
      SendFenceHandleIfPresent(op.textureParent(), compositable);
      break;
    }
    case CompositableOperation::TOpRemoveTextureAsync: {
      const OpRemoveTextureAsync& op = aEdit.get_OpRemoveTextureAsync();
      CompositableHost* compositable = AsCompositable(op);
      RefPtr<TextureHost> tex = TextureHost::AsTextureHost(op.textureParent());

      MOZ_ASSERT(tex.get());
      compositable->RemoveTextureHost(tex);

      if (!IsAsync() && ImageBridgeParent::GetInstance(GetChildProcessId())) {
        
        ImageBridgeParent::AppendDeliverFenceMessage(
                             GetChildProcessId(),
                             op.holderId(),
                             op.transactionId(),
                             op.textureParent(),
                             compositable);

        
        
        ImageBridgeParent::ReplyRemoveTexture(
                             GetChildProcessId(),
                             OpReplyRemoveTexture(op.holderId(),
                                                  op.transactionId()));
      } else {
        
        SendFenceHandleIfPresent(op.textureParent(), compositable);

        ReplyRemoveTexture(OpReplyRemoveTexture(op.holderId(),
                                                op.transactionId()));
      }
      break;
    }
    case CompositableOperation::TOpUseTexture: {
      const OpUseTexture& op = aEdit.get_OpUseTexture();
      CompositableHost* compositable = AsCompositable(op);

      nsAutoTArray<CompositableHost::TimedTexture,4> textures;
      for (auto& timedTexture : op.textures()) {
        CompositableHost::TimedTexture* t = textures.AppendElement();
        t->mTexture =
            TextureHost::AsTextureHost(timedTexture.textureParent());
        MOZ_ASSERT(t->mTexture);
        t->mTimeStamp = timedTexture.timeStamp();
        t->mPictureRect = timedTexture.picture();
        t->mFrameID = timedTexture.frameID();
        t->mProducerID = timedTexture.producerID();
        MOZ_ASSERT(ValidatePictureRect(t->mTexture->GetSize(), t->mPictureRect));

        MaybeFence maybeFence = timedTexture.fence();
        if (maybeFence.type() == MaybeFence::TFenceHandle) {
          FenceHandle fence = maybeFence.get_FenceHandle();
          if (fence.IsValid()) {
            t->mTexture->SetAcquireFenceHandle(fence);
          }
        }
      }
      compositable->UseTextureHost(textures);

      if (IsAsync() && compositable->GetLayer()) {
        ScheduleComposition(op);
        
        
        compositable->GetLayer()->SetInvalidRectToVisibleRegion();
      }
      break;
    }
    case CompositableOperation::TOpUseComponentAlphaTextures: {
      const OpUseComponentAlphaTextures& op = aEdit.get_OpUseComponentAlphaTextures();
      CompositableHost* compositable = AsCompositable(op);
      RefPtr<TextureHost> texOnBlack = TextureHost::AsTextureHost(op.textureOnBlackParent());
      RefPtr<TextureHost> texOnWhite = TextureHost::AsTextureHost(op.textureOnWhiteParent());

      MOZ_ASSERT(texOnBlack && texOnWhite);
      compositable->UseComponentAlphaTextures(texOnBlack, texOnWhite);

      if (IsAsync()) {
        ScheduleComposition(op);
      }
      break;
    }
#ifdef MOZ_WIDGET_GONK
    case CompositableOperation::TOpUseOverlaySource: {
      const OpUseOverlaySource& op = aEdit.get_OpUseOverlaySource();
      CompositableHost* compositable = AsCompositable(op);
      MOZ_ASSERT(compositable->GetType() == CompositableType::IMAGE_OVERLAY, "Invalid operation!");
      if (!ValidatePictureRect(op.overlay().size(), op.picture())) {
        return false;
      }
      compositable->UseOverlaySource(op.overlay(), op.picture());
      break;
    }
#endif
    default: {
      MOZ_ASSERT(false, "bad type");
    }
  }

  return true;
}

void
CompositableParentManager::SendPendingAsyncMessages()
{
  if (mPendingAsyncMessage.empty()) {
    return;
  }

  
  
  
  
#if defined(OS_POSIX)
  static const uint32_t kMaxMessageNumber = FileDescriptorSet::MAX_DESCRIPTORS_PER_MESSAGE;
#else
  
  static const uint32_t kMaxMessageNumber = 250;
#endif

  InfallibleTArray<AsyncParentMessageData> messages;
  messages.SetCapacity(mPendingAsyncMessage.size());
  for (size_t i = 0; i < mPendingAsyncMessage.size(); i++) {
    messages.AppendElement(mPendingAsyncMessage[i]);
    
    if (messages.Length() >= kMaxMessageNumber) {
      SendAsyncMessage(messages);
      
      messages.Clear();
    }
  }

  if (messages.Length() > 0) {
    SendAsyncMessage(messages);
  }
  mPendingAsyncMessage.clear();
}

} 
} 

