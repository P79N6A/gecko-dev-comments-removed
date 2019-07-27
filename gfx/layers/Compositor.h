




#ifndef MOZILLA_GFX_COMPOSITOR_H
#define MOZILLA_GFX_COMPOSITOR_H

#include "Units.h"                      
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/FenceUtils.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "nsISupportsImpl.h"            
#include "nsRegion.h"
#include <vector>
#include "mozilla/WidgetUtils.h"






















































































class nsIWidget;
class nsIntRegion;

namespace mozilla {
namespace gfx {
class Matrix;
class Matrix4x4;
class DrawTarget;
}

namespace layers {

struct Effect;
struct EffectChain;
class Image;
class Layer;
class TextureSource;
class DataTextureSource;
class CompositingRenderTarget;
class PCompositorParent;
class LayerManagerComposite;

enum SurfaceInitMode
{
  INIT_MODE_NONE,
  INIT_MODE_CLEAR
};











































class Compositor
{
protected:
  virtual ~Compositor() {}

public:
  NS_INLINE_DECL_REFCOUNTING(Compositor)

  explicit Compositor(PCompositorParent* aParent = nullptr)
    : mCompositorID(0)
    , mDiagnosticTypes(DiagnosticTypes::NO_DIAGNOSTIC)
    , mParent(aParent)
    , mScreenRotation(ROTATION_0)
  {
  }

  virtual TemporaryRef<DataTextureSource> CreateDataTextureSource(TextureFlags aFlags = TextureFlags::NO_FLAGS) = 0;
  virtual bool Initialize() = 0;
  virtual void Destroy() = 0;

  






  virtual bool SupportsEffect(EffectTypes aEffect) { return true; }

  




  virtual TextureFactoryIdentifier GetTextureFactoryIdentifier() = 0;

  


  virtual bool CanUseCanvasLayerForSize(const gfx::IntSize& aSize) = 0;
  virtual int32_t GetMaxTextureSize() const = 0;

  






  void SetTargetContext(gfx::DrawTarget* aTarget, const nsIntRect& aRect)
  {
    mTarget = aTarget;
    mTargetBounds = aRect;
  }
  void ClearTargetContext()
  {
    mTarget = nullptr;
  }

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

  



  virtual CompositingRenderTarget* GetCurrentRenderTarget() const = 0;

  



  virtual void SetDestinationSurfaceSize(const gfx::IntSize& aSize) = 0;

  



  virtual void SetScreenRenderOffset(const ScreenPoint& aOffset) = 0;

  





  virtual void DrawQuad(const gfx::Rect& aRect, const gfx::Rect& aClipRect,
                        const EffectChain& aEffectChain,
                        gfx::Float aOpacity, const gfx::Matrix4x4 &aTransform) = 0;

  


  virtual void ClearRect(const gfx::Rect& aRect) = 0;

  


















  virtual void BeginFrame(const nsIntRegion& aInvalidRegion,
                          const gfx::Rect* aClipRectIn,
                          const gfx::Rect& aRenderBounds,
                          gfx::Rect* aClipRectOut = nullptr,
                          gfx::Rect* aRenderBoundsOut = nullptr) = 0;

  


  virtual void EndFrame() = 0;

  virtual void SetFBAcquireFence(Layer* aLayer) {}

  virtual FenceHandle GetReleaseFence()
  {
    return FenceHandle();
  }

  




  virtual void EndFrameForExternalComposition(const gfx::Matrix& aTransform) = 0;

  







  virtual void PrepareViewport(const gfx::IntSize& aSize) = 0;

  


  virtual bool SupportsPartialTextureUpdate() = 0;

  void SetDiagnosticTypes(DiagnosticTypes aDiagnostics)
  {
    mDiagnosticTypes = aDiagnostics;
  }

  DiagnosticTypes GetDiagnosticTypes() const
  {
    return mDiagnosticTypes;
  }

  void DrawDiagnostics(DiagnosticFlags aFlags,
                       const gfx::Rect& visibleRect,
                       const gfx::Rect& aClipRect,
                       const gfx::Matrix4x4& transform,
                       uint32_t aFlashCounter = DIAGNOSTIC_FLASH_COUNTER_MAX);

  void DrawDiagnostics(DiagnosticFlags aFlags,
                       const nsIntRegion& visibleRegion,
                       const gfx::Rect& aClipRect,
                       const gfx::Matrix4x4& transform,
                       uint32_t aFlashCounter = DIAGNOSTIC_FLASH_COUNTER_MAX);

#ifdef MOZ_DUMP_PAINTING
  virtual const char* Name() const = 0;
#endif 

  virtual LayersBackend GetBackendType() const = 0;

  






  uint32_t GetCompositorID() const
  {
    return mCompositorID;
  }
  void SetCompositorID(uint32_t aID)
  {
    MOZ_ASSERT(mCompositorID == 0, "The compositor ID must be set only once.");
    mCompositorID = aID;
  }

  




  virtual void Pause() {}
  




  virtual bool Resume() { return true; }

  



  virtual bool Ready() { return true; }

  
  
  virtual nsIWidget* GetWidget() const { return nullptr; }

  



  static void AssertOnCompositorThread();

  







  static LayersBackend GetBackend();

  size_t GetFillRatio() {
    float fillRatio = 0;
    if (mPixelsFilled > 0 && mPixelsPerFrame > 0) {
      fillRatio = 100.0f * float(mPixelsFilled) / float(mPixelsPerFrame);
      if (fillRatio > 999.0f) {
        fillRatio = 999.0f;
      }
    }
    return fillRatio;
  }

  ScreenRotation GetScreenRotation() const {
    return mScreenRotation;
  }

  void SetScreenRotation(ScreenRotation aRotation) {
    mScreenRotation = aRotation;
  }

protected:
  void DrawDiagnosticsInternal(DiagnosticFlags aFlags,
                               const gfx::Rect& aVisibleRect,
                               const gfx::Rect& aClipRect,
                               const gfx::Matrix4x4& transform,
                               uint32_t aFlashCounter);

  bool ShouldDrawDiagnostics(DiagnosticFlags);

  


  static void SetBackend(LayersBackend backend);

  uint32_t mCompositorID;
  DiagnosticTypes mDiagnosticTypes;
  PCompositorParent* mParent;

  




  size_t mPixelsPerFrame;
  size_t mPixelsFilled;

  ScreenRotation mScreenRotation;

  virtual gfx::IntSize GetWidgetSize() const = 0;

  RefPtr<gfx::DrawTarget> mTarget;
  nsIntRect mTargetBounds;

private:
  static LayersBackend sBackend;

};


typedef gfx::Rect decomposedRectArrayT[4];
size_t DecomposeIntoNoRepeatRects(const gfx::Rect& aRect,
                                  const gfx::Rect& aTexCoordRect,
                                  decomposedRectArrayT* aLayerRects,
                                  decomposedRectArrayT* aTextureRects);

} 
} 

#endif 
