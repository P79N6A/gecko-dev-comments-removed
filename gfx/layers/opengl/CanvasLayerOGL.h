




#ifndef GFX_CANVASLAYEROGL_H
#define GFX_CANVASLAYEROGL_H

#include "GLContextTypes.h"             
#include "GLDefs.h"                     
#include "LayerManagerOGL.h"            
#include "Layers.h"                     
#include "gfxTypes.h"
#include "gfxPoint.h"                   
#include "mozilla/Preferences.h"        
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"             
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "opengl/LayerManagerOGLProgram.h"  
#if defined(GL_PROVIDER_GLX)
#include "GLXLibrary.h"
#include "mozilla/X11Util.h"
#endif

struct nsIntPoint;
class gfxASurface;
class gfxImageSurface;

namespace mozilla {
namespace layers {

class CanvasLayerOGL :
  public CanvasLayer,
  public LayerOGL
{
public:
  CanvasLayerOGL(LayerManagerOGL *aManager);
  ~CanvasLayerOGL();

  
  virtual void Initialize(const Data& aData);

  
  virtual void Destroy();
  virtual Layer* GetLayer() { return this; }
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources();

protected:
  void UpdateSurface();

  nsRefPtr<gfxASurface> mCanvasSurface;
  nsRefPtr<GLContext> mGLContext;
  ShaderProgramType mLayerProgram;
  RefPtr<gfx::DrawTarget> mDrawTarget;

  GLuint mTexture;
  GLenum mTextureTarget;

  bool mDelayedUpdates;
  bool mIsGLAlphaPremult;
  bool mNeedsYFlip;
  bool mForceReadback;
  GLuint mUploadTexture;
#if defined(GL_PROVIDER_GLX)
  GLXPixmap mPixmap;
#endif

  nsRefPtr<gfxImageSurface> mCachedTempSurface;
  gfxIntSize mCachedSize;
  gfxImageFormat mCachedFormat;

  gfxImageSurface* GetTempSurface(const gfxIntSize& aSize,
                                  const gfxImageFormat aFormat);

  void DiscardTempSurface() {
    mCachedTempSurface = nullptr;
  }
};

} 
} 
#endif 
