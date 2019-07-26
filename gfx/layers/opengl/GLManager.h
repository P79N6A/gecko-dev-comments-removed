




#ifndef MOZILLA_GFX_GLMANAGER_H
#define MOZILLA_GFX_GLMANAGER_H

#include "mozilla/gfx/Types.h"          
#include "LayerManagerOGLProgram.h"

namespace mozilla {
namespace gl {
class GLContext;
}

namespace layers {

class LayerManager;






class GLManager
{
public:
  static GLManager* CreateGLManager(LayerManager* aManager);

  virtual ~GLManager() {}

  virtual gl::GLContext* gl() const = 0;
  virtual ShaderProgramOGL* GetProgram(GLenum aTarget, gfx::SurfaceFormat aFormat) = 0;
  virtual const gfx3DMatrix& GetProjMatrix() const = 0;
  virtual void BindAndDrawQuad(ShaderProgramOGL *aProg) = 0;
};

}
}
#endif
