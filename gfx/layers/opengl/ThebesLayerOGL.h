




































#ifndef GFX_THEBESLAYEROGL_H
#define GFX_THEBESLAYEROGL_H

#ifdef MOZ_IPC
# include "mozilla/layers/PLayers.h"
# include "mozilla/layers/ShadowLayers.h"
#endif

#include "Layers.h"
#include "LayerManagerOGL.h"
#include "gfxImageSurface.h"
#include "GLContext.h"


namespace mozilla {
namespace layers {

class ThebesLayerBufferOGL;
class BasicBufferOGL;
class ShadowBufferOGL;

class ThebesLayerOGL : public ThebesLayer, 
                       public LayerOGL
{
  typedef ThebesLayerBufferOGL Buffer;

public:
  ThebesLayerOGL(LayerManagerOGL *aManager);
  virtual ~ThebesLayerOGL();

  
  void SetVisibleRegion(const nsIntRegion& aRegion);

  
  void InvalidateRegion(const nsIntRegion& aRegion);

  
  void Destroy();
  Layer* GetLayer();
  virtual PRBool IsEmpty();
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

private:
  friend class BasicBufferOGL;

  PRBool CreateSurface();

  nsRefPtr<Buffer> mBuffer;
};

#ifdef MOZ_IPC
class ShadowThebesLayerOGL : public ShadowThebesLayer,
                             public LayerOGL
{
public:
  ShadowThebesLayerOGL(LayerManagerOGL *aManager);
  virtual ~ShadowThebesLayerOGL();

  
  virtual void SetFrontBuffer(const ThebesBuffer& aNewFront,
                              const nsIntRegion& aValidRegion,
                              float aXResolution, float aYResolution);
  virtual void
  Swap(const ThebesBuffer& aNewFront, const nsIntRegion& aUpdatedRegion,
       ThebesBuffer* aNewBack, nsIntRegion* aNewBackValidRegion,
       float* aNewXResolution, float* aNewYResolution);
  virtual void DestroyFrontBuffer();

  
  void Destroy();
  Layer* GetLayer();
  virtual PRBool IsEmpty();
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

private:
  nsRefPtr<ShadowBufferOGL> mBuffer;


  
  SurfaceDescriptor mDeadweight;


};
#endif  

} 
} 
#endif 
