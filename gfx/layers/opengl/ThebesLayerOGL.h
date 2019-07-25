




































#ifndef GFX_THEBESLAYEROGL_H
#define GFX_THEBESLAYEROGL_H

#include "Layers.h"
#include "LayerManagerOGL.h"
#include "gfxImageSurface.h"


namespace mozilla {
namespace layers {

class ThebesLayerOGL : public ThebesLayer, 
                         public LayerOGL
{
public:
  ThebesLayerOGL(LayerManagerOGL *aManager);
  virtual ~ThebesLayerOGL();

  
  void SetVisibleRegion(const nsIntRegion& aRegion);

  
  void InvalidateRegion(const nsIntRegion& aRegion);

  gfxContext *BeginDrawing(nsIntRegion* aRegionToDraw);

  void EndDrawing();

  
  LayerType GetType();
  Layer* GetLayer();
  virtual PRBool IsEmpty();
  virtual void RenderLayer(int aPreviousFrameBuffer);

  
  const nsIntRect &GetVisibleRect();
  const nsIntRect &GetInvalidatedRect();

private:
  



  nsIntRect mVisibleRect;
  


  nsIntRect mInvalidatedRect;
  



  nsRefPtr<gfxASurface> mDestinationSurface;

  


  nsRefPtr<gfxContext> mContext;

  


  GLuint mTexture;

};

} 
} 
#endif 
