






#include "CompositableTransactionParent.h"
#include "CompositableHost.h"           
#include "CompositorParent.h"           
#include "GLContext.h"                  
#include "Layers.h"                     
#include "RenderTrace.h"                
#include "TiledLayerBuffer.h"           
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/ContentHost.h"  
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/layers/TextureHostOGL.h"  
#include "mozilla/layers/ThebesLayerComposite.h"
#include "mozilla/mozalloc.h"           
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

bool
CompositableParentManager::ReceiveCompositableUpdate(const CompositableOperation& aEdit,
                                                     EditReplyVector& replyv)
{
  switch (aEdit.type()) {
    case CompositableOperation::TOpCreatedIncrementalTexture: {
      MOZ_LAYERS_LOG(("[ParentSide] Created texture"));
      const OpCreatedIncrementalTexture& op = aEdit.get_OpCreatedIncrementalTexture();
      CompositableHost* compositable = AsCompositable(op);

      bool success =
        compositable->CreatedIncrementalTexture(this,
                                                op.textureInfo(),
                                                op.bufferRect());
      if (!success) {
        return false;
      }
      break;
    }
    case CompositableOperation::TOpPaintTextureRegion: {
      MOZ_LAYERS_LOG(("[ParentSide] Paint ThebesLayer"));

      const OpPaintTextureRegion& op = aEdit.get_OpPaintTextureRegion();
      CompositableHost* compositable = AsCompositable(op);
      Layer* layer = compositable->GetLayer();
      if (!layer || layer->GetType() != Layer::TYPE_THEBES) {
        return false;
      }
      ThebesLayerComposite* thebes = static_cast<ThebesLayerComposite*>(layer);

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
      
      ReturnTextureDataIfNecessary(compositable, replyv, op.compositableParent());
      break;
    }
    case CompositableOperation::TOpPaintTextureIncremental: {
      MOZ_LAYERS_LOG(("[ParentSide] Paint ThebesLayer"));

      const OpPaintTextureIncremental& op = aEdit.get_OpPaintTextureIncremental();

      CompositableHost* compositable = AsCompositable(op);

      SurfaceDescriptor desc = op.image();

      compositable->UpdateIncremental(op.textureId(),
                                      desc,
                                      op.updatedRegion(),
                                      op.bufferRect(),
                                      op.bufferRotation());
      break;
    }
    case CompositableOperation::TOpUpdatePictureRect: {
      const OpUpdatePictureRect& op = aEdit.get_OpUpdatePictureRect();
      CompositableHost* compositable = AsCompositable(op);
      MOZ_ASSERT(compositable);
      compositable->SetPictureRect(op.picture());
      break;
    }
    case CompositableOperation::TOpUseTiledLayerBuffer: {
      MOZ_LAYERS_LOG(("[ParentSide] Paint TiledLayerBuffer"));
      const OpUseTiledLayerBuffer& op = aEdit.get_OpUseTiledLayerBuffer();
      CompositableHost* compositable = AsCompositable(op);

      TiledLayerComposer* tileComposer = compositable->AsTiledLayerComposer();
      NS_ASSERTION(tileComposer, "compositable is not a tile composer");

      const SurfaceDescriptorTiles& tileDesc = op.tileLayerDescriptor();
      tileComposer->UseTiledLayerBuffer(this, tileDesc);
      break;
    }
    case CompositableOperation::TOpRemoveTexture: {
      const OpRemoveTexture& op = aEdit.get_OpRemoveTexture();
      CompositableHost* compositable = AsCompositable(op);
      RefPtr<TextureHost> tex = TextureHost::AsTextureHost(op.textureParent());

      MOZ_ASSERT(tex.get());
      compositable->RemoveTextureHost(tex);
      
      ReturnTextureDataIfNecessary(compositable, replyv, op.compositableParent());
      break;
    }
    case CompositableOperation::TOpUseTexture: {
      const OpUseTexture& op = aEdit.get_OpUseTexture();
      CompositableHost* compositable = AsCompositable(op);
      RefPtr<TextureHost> tex = TextureHost::AsTextureHost(op.textureParent());

      MOZ_ASSERT(tex.get());
      compositable->UseTextureHost(tex);

      if (IsAsync()) {
        ScheduleComposition(op);
        
        
        if (compositable->GetLayer()) {
          compositable->GetLayer()->SetInvalidRectToVisibleRegion();
        }
      }
      
      ReturnTextureDataIfNecessary(compositable, replyv, op.compositableParent());
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
      
      ReturnTextureDataIfNecessary(compositable, replyv, op.compositableParent());
      break;
    }
    case CompositableOperation::TOpUpdateTexture: {
      const OpUpdateTexture& op = aEdit.get_OpUpdateTexture();
      RefPtr<TextureHost> texture = TextureHost::AsTextureHost(op.textureParent());
      MOZ_ASSERT(texture);

      texture->Updated(op.region().type() == MaybeRegion::TnsIntRegion
                       ? &op.region().get_nsIntRegion()
                       : nullptr); 
      break;
    }

    default: {
      MOZ_ASSERT(false, "bad type");
    }
  }

  return true;
}

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
void
CompositableParentManager::ReturnTextureDataIfNecessary(CompositableHost* aCompositable,
                                                        EditReplyVector& replyv,
                                                        PCompositableParent* aParent)
{
  if (!aCompositable || !aCompositable->GetCompositableBackendSpecificData()) {
    return;
  }

  const std::vector< RefPtr<TextureHost> > textureList =
        aCompositable->GetCompositableBackendSpecificData()->GetPendingReleaseFenceTextureList();
  
  for (size_t i = 0; i < textureList.size(); i++) {
    
    
    if (mPrevFenceHandles.size() >= 4) {
      break;
    }
    TextureHostOGL* hostOGL = textureList[i]->AsHostOGL();
    PTextureParent* actor = textureList[i]->GetIPDLActor();
    if (!hostOGL || !actor) {
      continue;
    }
    android::sp<android::Fence> fence = hostOGL->GetAndResetReleaseFence();
    if (fence.get() && fence->isValid()) {
      FenceHandle handle = FenceHandle(fence);
      replyv.push_back(ReturnReleaseFence(aParent, nullptr, actor, nullptr, handle));
      
      mPrevFenceHandles.push_back(handle);
    }
  }
  aCompositable->GetCompositableBackendSpecificData()->ClearPendingReleaseFenceTextureList();
}
#else
void
CompositableParentManager::ReturnTextureDataIfNecessary(CompositableHost* aCompositable,
                                                       EditReplyVector& replyv,
                                                       PCompositableParent* aParent)
{
  if (!aCompositable || !aCompositable->GetCompositableBackendSpecificData()) {
    return;
  }
  aCompositable->GetCompositableBackendSpecificData()->ClearPendingReleaseFenceTextureList();
}
#endif

void
CompositableParentManager::ClearPrevFenceHandles()
{
  mPrevFenceHandles.clear();
}

} 
} 

