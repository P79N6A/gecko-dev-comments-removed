




#ifndef MOZILLA_GFX_PersistentBUFFERPROVIDER_H
#define MOZILLA_GFX_PersistentBUFFERPROVIDER_H

#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/gfx/Types.h"

namespace mozilla {
namespace layers {

class CopyableCanvasLayer;








class PersistentBufferProvider : public RefCounted<PersistentBufferProvider>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(PersistentBufferProvider)

  virtual ~PersistentBufferProvider() { }

  virtual LayersBackend GetType() { return LayersBackend::LAYERS_NONE; }

  






  virtual gfx::DrawTarget* GetDT(const gfx::IntRect& aPersistedRect) = 0;
  




  virtual bool ReturnAndUseDT(gfx::DrawTarget* aDT) = 0;

  virtual already_AddRefed<gfx::SourceSurface> GetSnapshot() = 0;
protected:
};

class PersistentBufferProviderBasic : public PersistentBufferProvider
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(PersistentBufferProviderBasic)

  PersistentBufferProviderBasic(LayerManager* aManager, gfx::IntSize aSize,
                                gfx::SurfaceFormat aFormat, gfx::BackendType aBackend);

  bool IsValid() { return !!mDrawTarget; }
  virtual LayersBackend GetType() { return LayersBackend::LAYERS_BASIC; }
  gfx::DrawTarget* GetDT(const gfx::IntRect& aPersistedRect) { return mDrawTarget; }
  bool ReturnAndUseDT(gfx::DrawTarget* aDT) { MOZ_ASSERT(mDrawTarget == aDT); return true; }
  virtual already_AddRefed<gfx::SourceSurface> GetSnapshot() { return mDrawTarget->Snapshot(); }
private:
  RefPtr<gfx::DrawTarget> mDrawTarget;
};

}
}
#endif
