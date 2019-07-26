




#ifndef GFX_COPYABLECANVASLAYER_H
#define GFX_COPYABLECANVASLAYER_H

#include <stdint.h>                     
#include "GLContextTypes.h"             
#include "Layers.h"                     
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

  bool IsGLLayer() { return !!mGLContext; }

protected:
  void UpdateTarget(gfx::DrawTarget* aDestTarget = nullptr);

  RefPtr<gfx::SourceSurface> mSurface;
  nsRefPtr<mozilla::gl::GLContext> mGLContext;
  mozilla::RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;

  RefPtr<gfx::SurfaceStream> mStream;

  uint32_t mCanvasFramebuffer;

  bool mIsGLAlphaPremult;
  bool mNeedsYFlip;

  RefPtr<gfx::DataSourceSurface> mCachedTempSurface;

  gfx::DataSourceSurface* GetTempSurface(const gfx::IntSize& aSize,
                                         const gfx::SurfaceFormat aFormat);

  void DiscardTempSurface();
};

}
}

#endif
