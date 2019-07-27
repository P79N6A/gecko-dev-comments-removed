




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

namespace gl {
class SharedSurface;
}

namespace layers {





class CopyableCanvasLayer : public CanvasLayer
{
public:
  CopyableCanvasLayer(LayerManager* aLayerManager, void *aImplData);

protected:
  virtual ~CopyableCanvasLayer();

public:
  virtual void Initialize(const Data& aData) override;

  virtual bool IsDataValid(const Data& aData) override;

  bool IsGLLayer() { return !!mGLContext; }

protected:
  void UpdateTarget(gfx::DrawTarget* aDestTarget = nullptr);

  RefPtr<gfx::SourceSurface> mSurface;
  nsRefPtr<gl::GLContext> mGLContext;
  GLuint mCanvasFrontbufferTexID;
  mozilla::RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;

  UniquePtr<gl::SharedSurface> mGLFrontbuffer;

  bool mIsAlphaPremultiplied;
  gl::OriginPos mOriginPos;

  RefPtr<gfx::DataSourceSurface> mCachedTempSurface;

  gfx::DataSourceSurface* GetTempSurface(const gfx::IntSize& aSize,
                                         const gfx::SurfaceFormat aFormat);

  void DiscardTempSurface();
};

}
}

#endif
