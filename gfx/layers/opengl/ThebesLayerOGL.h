




#ifndef GFX_THEBESLAYEROGL_H
#define GFX_THEBESLAYEROGL_H

#include "Layers.h"
#include "LayerManagerOGL.h"
#include "gfxImageSurface.h"
#include "GLContext.h"
#include "base/task.h"


namespace mozilla {
namespace layers {

class ThebesLayerBufferOGL;
class BasicBufferOGL;

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
  virtual bool IsEmpty();
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources();

private:
  friend class BasicBufferOGL;

  bool CreateSurface();

  nsRefPtr<Buffer> mBuffer;
};


} 
} 
#endif 
