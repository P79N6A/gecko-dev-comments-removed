






#ifndef mozilla_layers_ShadowLayersParent_h
#define mozilla_layers_ShadowLayersParent_h

#include "mozilla/layers/PLayersParent.h"
#include "ShadowLayers.h"
#include "ShadowLayersManager.h"

namespace mozilla {

namespace layout {
class RenderFrameParent;
}

namespace layers {

class Layer;
class ShadowLayerManager;

class ShadowLayersParent : public PLayersParent,
                           public ISurfaceDeAllocator
{
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;
  typedef InfallibleTArray<Edit> EditArray;
  typedef InfallibleTArray<EditReply> EditReplyArray;

public:
  ShadowLayersParent(ShadowLayerManager* aManager,
                     ShadowLayersManager* aLayersManager,
                     uint64_t aId);
  ~ShadowLayersParent();

  void Destroy();

  ShadowLayerManager* layer_manager() const { return mLayerManager; }

  uint64_t GetId() const { return mId; }
  ContainerLayer* GetRoot() const { return mRoot; }

  virtual void DestroySharedSurface(gfxSharedImageSurface* aSurface);
  virtual void DestroySharedSurface(SurfaceDescriptor* aSurface);

protected:
  virtual bool RecvUpdate(const EditArray& cset,
                          const TargetConfig& targetConfig,
                          const bool& isFirstPaint,
                          EditReplyArray* reply) MOZ_OVERRIDE;

  virtual bool RecvDrawToSurface(const SurfaceDescriptor& surfaceIn,
                                 SurfaceDescriptor* surfaceOut) MOZ_OVERRIDE;

  virtual bool RecvUpdateNoSwap(const EditArray& cset,
                                const TargetConfig& targetConfig,
                                const bool& isFirstPaint) MOZ_OVERRIDE;

  virtual PGrallocBufferParent*
  AllocPGrallocBuffer(const gfxIntSize& aSize, const gfxContentType& aContent,
                      MaybeMagicGrallocBufferHandle* aOutHandle) MOZ_OVERRIDE;
  virtual bool
  DeallocPGrallocBuffer(PGrallocBufferParent* actor) MOZ_OVERRIDE;

  virtual PLayerParent* AllocPLayer() MOZ_OVERRIDE;
  virtual bool DeallocPLayer(PLayerParent* actor) MOZ_OVERRIDE;

private:
  nsRefPtr<ShadowLayerManager> mLayerManager;
  ShadowLayersManager* mShadowLayersManager;
  
  
  nsRefPtr<ContainerLayer> mRoot;
  
  
  
  
  uint64_t mId;
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mDestroyed;
};

} 
} 

#endif 
