




#ifndef MOZILLA_GFX_COMPOSITOR_H
#define MOZILLA_GFX_COMPOSITOR_H

#include "mozilla/gfx/Rect.h"
#include "mozilla/gfx/Matrix.h"
#include "gfxMatrix.h"
#include "Layers.h"
#include "mozilla/layers/TextureHost.h"
#include "mozilla/RefPtr.h"























































































class gfxContext;
class nsIWidget;

namespace mozilla {
namespace gfx {
class DrawTarget;
}

namespace layers {

struct Effect;
struct EffectChain;
class Image;
class ISurfaceAllocator;

enum SurfaceInitMode
{
  INIT_MODE_NONE,
  INIT_MODE_CLEAR,
  INIT_MODE_COPY
};













































class Compositor : public RefCounted<Compositor>
{
public:
  Compositor()
    : mCompositorID(0)
  {
    MOZ_COUNT_CTOR(Compositor);
  }
  virtual ~Compositor()
  {
    MOZ_COUNT_DTOR(Compositor);
  }

  virtual bool Initialize() = 0;
  virtual void Destroy() = 0;

  




  virtual TextureFactoryIdentifier GetTextureFactoryIdentifier() = 0;

  


  virtual bool CanUseCanvasLayerForSize(const gfxIntSize& aSize) = 0;
  virtual int32_t GetMaxTextureSize() const = 0;

  






  virtual void SetTargetContext(gfxContext* aTarget) = 0;

  typedef uint32_t MakeCurrentFlags;
  static const MakeCurrentFlags ForceMakeCurrent = 0x1;
  










  virtual void MakeCurrent(MakeCurrentFlags aFlags = 0) = 0;

  



  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTarget(const gfx::IntRect& aRect, SurfaceInitMode aInit) = 0;

  




  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTargetFromSource(const gfx::IntRect& aRect,
                               const CompositingRenderTarget* aSource) = 0;

  



  virtual void SetRenderTarget(CompositingRenderTarget* aSurface) = 0;

  



  virtual CompositingRenderTarget* GetCurrentRenderTarget() = 0;

  



  virtual void SetDestinationSurfaceSize(const gfx::IntSize& aSize) = 0;

  






  virtual void DrawQuad(const gfx::Rect& aRect, const gfx::Rect& aClipRect,
                        const EffectChain& aEffectChain,
                        gfx::Float aOpacity, const gfx::Matrix4x4 &aTransform,
                        const gfx::Point& aOffset) = 0;

  










  virtual void BeginFrame(const gfx::Rect* aClipRectIn,
                          const gfxMatrix& aTransform,
                          const gfx::Rect& aRenderBounds,
                          gfx::Rect* aClipRectOut = nullptr,
                          gfx::Rect* aRenderBoundsOut = nullptr) = 0;

  


  virtual void EndFrame() = 0;

  




  virtual void EndFrameForExternalComposition(const gfxMatrix& aTransform) = 0;

  


  virtual void AbortFrame() = 0;

  









  virtual void PrepareViewport(const gfx::IntSize& aSize,
                               const gfxMatrix& aWorldTransform) = 0;

  


  virtual bool SupportsPartialTextureUpdate() = 0;

#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const = 0;
#endif 


  






  uint32_t GetCompositorID() const
  {
    return mCompositorID;
  }
  void SetCompositorID(uint32_t aID)
  {
    MOZ_ASSERT(mCompositorID == 0, "The compositor ID must be set only once.");
    mCompositorID = aID;
  }

  




  virtual void NotifyLayersTransaction() = 0;

  




  virtual void Pause() {}
  




  virtual bool Resume() { return true; }

  
  
  virtual nsIWidget* GetWidget() const { return nullptr; }
  virtual const nsIntSize& GetWidgetSize() = 0;

  





  static LayersBackend GetBackend();

protected:
  uint32_t mCompositorID;
  static LayersBackend sBackend;
};

} 
} 

#endif 
