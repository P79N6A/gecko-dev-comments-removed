




#ifndef MOZILLA_GFX_COMPOSITOR_H
#define MOZILLA_GFX_COMPOSITOR_H

#include "Units.h"                      
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "nsTraceRefcnt.h"              
#include "nsRegion.h"






















































































class nsIWidget;
struct gfxMatrix;
struct nsIntSize;
class nsIntRegion;

namespace mozilla {
namespace gfx {
class Matrix4x4;
class DrawTarget;
}

namespace layers {

struct Effect;
struct EffectChain;
class Image;
class ISurfaceAllocator;
class NewTextureSource;
class DataTextureSource;
class CompositingRenderTarget;
class PCompositorParent;
class LayerManagerComposite;

enum SurfaceInitMode
{
  INIT_MODE_NONE,
  INIT_MODE_CLEAR
};













































class Compositor : public RefCounted<Compositor>
{
public:
  Compositor(PCompositorParent* aParent = nullptr)
    : mCompositorID(0)
    , mDiagnosticTypes(DIAGNOSTIC_NONE)
    , mParent(aParent)
  {
    MOZ_COUNT_CTOR(Compositor);
  }
  virtual ~Compositor()
  {
    MOZ_COUNT_DTOR(Compositor);
  }

  virtual TemporaryRef<DataTextureSource> CreateDataTextureSource(TextureFlags aFlags = 0) = 0;
  virtual bool Initialize() = 0;
  virtual void Destroy() = 0;

  






  virtual bool SupportsEffect(EffectTypes aEffect) { return true; }

  




  virtual TextureFactoryIdentifier GetTextureFactoryIdentifier() = 0;

  


  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize& aSize) = 0;
  virtual int32_t GetMaxTextureSize() const = 0;

  






  virtual void SetTargetContext(gfx::DrawTarget* aTarget) = 0;

  typedef uint32_t MakeCurrentFlags;
  static const MakeCurrentFlags ForceMakeCurrent = 0x1;
  










  virtual void MakeCurrent(MakeCurrentFlags aFlags = 0) = 0;

  



  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTarget(const gfx::IntRect& aRect, SurfaceInitMode aInit) = 0;

  






  virtual TemporaryRef<CompositingRenderTarget>
  CreateRenderTargetFromSource(const gfx::IntRect& aRect,
                               const CompositingRenderTarget* aSource,
                               const gfx::IntPoint& aSourcePoint) = 0;

  



  virtual void SetRenderTarget(CompositingRenderTarget* aSurface) = 0;

  



  virtual CompositingRenderTarget* GetCurrentRenderTarget() = 0;

  



  virtual void SetDestinationSurfaceSize(const gfx::IntSize& aSize) = 0;

  



  virtual void SetScreenRenderOffset(const ScreenPoint& aOffset) = 0;

  





  virtual void DrawQuad(const gfx::Rect& aRect, const gfx::Rect& aClipRect,
                        const EffectChain& aEffectChain,
                        gfx::Float aOpacity, const gfx::Matrix4x4 &aTransform) = 0;

  


















  virtual void BeginFrame(const nsIntRegion& aInvalidRegion,
                          const gfx::Rect* aClipRectIn,
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

  void SetDiagnosticTypes(DiagnosticTypes aDiagnostics)
  {
    mDiagnosticTypes = aDiagnostics;
  }

  void DrawDiagnostics(DiagnosticFlags aFlags,
                       const gfx::Rect& visibleRect,
                       const gfx::Rect& aClipRect,
                       const gfx::Matrix4x4& transform);

  void DrawDiagnostics(DiagnosticFlags aFlags,
                       const nsIntRegion& visibleRegion,
                       const gfx::Rect& aClipRect,
                       const gfx::Matrix4x4& transform);


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

  



  virtual bool Ready() { return true; }

  
  
  virtual nsIWidget* GetWidget() const { return nullptr; }
  virtual const nsIntSize& GetWidgetSize() = 0;

  
  
  
  virtual void SaveState() {}
  virtual void RestoreState() {}

  



  static void AssertOnCompositorThread();

  







  static LayersBackend GetBackend();

protected:
  void DrawDiagnosticsInternal(DiagnosticFlags aFlags,
                               const gfx::Rect& aVisibleRect,
                               const gfx::Rect& aClipRect,
                               const gfx::Matrix4x4& transform);

  bool ShouldDrawDiagnostics(DiagnosticFlags);

  uint32_t mCompositorID;
  static LayersBackend sBackend;
  DiagnosticTypes mDiagnosticTypes;
  PCompositorParent* mParent;

  




  size_t mPixelsPerFrame;
  size_t mPixelsFilled;
};

} 
} 

#endif 
