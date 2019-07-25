




































#ifndef GFX_MACIOSURFACEIMAGEOGL_H
#define GFX_MACIOSURFACEIMAGEOGL_H
#ifdef XP_MACOSX

#include "nsCoreAnimationSupport.h"

namespace mozilla {
namespace layers {

class THEBES_API MacIOSurfaceImageOGL : public MacIOSurfaceImage
{
  typedef mozilla::gl::GLContext GLContext;

public:
  MacIOSurfaceImageOGL(LayerManagerOGL *aManager);
  virtual ~MacIOSurfaceImageOGL();

  void SetUpdateCallback(UpdateSurfaceCallback aCallback, void* aPluginInstanceOwner);
  void SetDestroyCallback(DestroyCallback aCallback);
  void Update(ImageContainer* aContainer);

  void SetData(const Data &aData);

  GLTexture mTexture;
  gfxIntSize mSize;
  nsAutoPtr<nsIOSurface> mIOSurface;
  void* mPluginInstanceOwner;
  UpdateSurfaceCallback mUpdateCallback;
  DestroyCallback mDestroyCallback;
};

} 
} 
#endif 
#endif 
