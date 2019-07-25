




































#ifndef GFX_THEBESLAYEROGL_H
#define GFX_THEBESLAYEROGL_H

#include "Layers.h"
#include "LayerManagerOGL.h"
#include "gfxImageSurface.h"
#include "GLContext.h"


namespace mozilla {
namespace layers {

class ThebesLayerOGL : public ThebesLayer, 
                         public LayerOGL
{
public:
  typedef mozilla::gl::GLContext GLContext;
  ThebesLayerOGL(LayerManagerOGL *aManager);
  virtual ~ThebesLayerOGL();

  
  void SetVisibleRegion(const nsIntRegion& aRegion);

  
  void InvalidateRegion(const nsIntRegion& aRegion);

  
  LayerType GetType();
  Layer* GetLayer();
  virtual PRBool IsEmpty();
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

  
  nsIntRect GetVisibleRect() { return mVisibleRegion.GetBounds(); }
  const nsIntRect &GetInvalidatedRect();

private:
  PRBool EnsureSurface();
  


  nsIntRect mInvalidatedRect;

  


  GLuint mTexture;
  nsRefPtr<GLContext> mOffscreenSurfaceAsGLContext;
  nsRefPtr<gfxASurface> mOffScreenSurface;
  gfxASurface::gfxImageFormat mOffscreenFormat;
  gfxIntSize mOffscreenSize;
};

} 
} 
#endif 
