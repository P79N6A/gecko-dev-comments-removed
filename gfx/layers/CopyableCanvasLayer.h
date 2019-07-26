




#ifndef GFX_COPYABLECANVASLAYER_H
#define GFX_COPYABLECANVASLAYER_H

#include <stdint.h>                     
#include "GLContextTypes.h"             
#include "Layers.h"                     
#include "gfxASurface.h"                
#include "gfxContext.h"                 
#include "gfxTypes.h"
#include "gfxPlatform.h"                
#include "mozilla/Assertions.h"         
#include "mozilla/Preferences.h"        
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"             
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"            

namespace mozilla {

namespace gfx {
class SurfaceStream;
class SharedSurface;
class SurfaceFactory;
}

namespace layers {

class CanvasClientWebGL;





class CopyableCanvasLayer : public CanvasLayer
{
public:
  CopyableCanvasLayer(LayerManager* aLayerManager, void *aImplData);
  virtual ~CopyableCanvasLayer();

  virtual void Initialize(const Data& aData);

  virtual bool IsDataValid(const Data& aData);

protected:
  void PaintWithOpacity(gfx::DrawTarget* aTarget,
                        float aOpacity,
                        gfx::SourceSurface* aMaskSurface,
                        gfx::CompositionOp aOperator = gfx::CompositionOp::OP_OVER);

  void UpdateTarget(gfx::DrawTarget* aDestTarget = nullptr,
                    gfx::SourceSurface* aMaskSurface = nullptr);

  RefPtr<gfx::SourceSurface> mSurface;
  nsRefPtr<gfxASurface> mDeprecatedSurface;
  nsRefPtr<mozilla::gl::GLContext> mGLContext;
  mozilla::RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;

  RefPtr<gfx::SurfaceStream> mStream;

  uint32_t mCanvasFramebuffer;

  bool mIsGLAlphaPremult;
  bool mNeedsYFlip;

  RefPtr<gfx::DataSourceSurface> mCachedTempSurface;
  nsRefPtr<gfxImageSurface> mDeprecatedCachedTempSurface;
  gfx::IntSize mCachedSize;
  gfx::SurfaceFormat mCachedFormat;
  gfxImageFormat mDeprecatedCachedFormat;

  gfx::DataSourceSurface* GetTempSurface(const gfx::IntSize& aSize,
                                         const gfx::SurfaceFormat aFormat);

  void DiscardTempSurface();

  
protected:
  void DeprecatedPaintWithOpacity(gfxContext* aContext,
                                  float aOpacity,
                                  Layer* aMaskLayer,
                                  gfxContext::GraphicsOperator aOperator = gfxContext::OPERATOR_OVER);

  void DeprecatedUpdateSurface(gfxASurface* aDestSurface = nullptr,
                               Layer* aMaskLayer = nullptr);

  gfxImageSurface* DeprecatedGetTempSurface(const gfx::IntSize& aSize,
                                            const gfxImageFormat aFormat);

};

}
}

#endif
