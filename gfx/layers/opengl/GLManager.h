




#ifndef MOZILLA_GFX_GLMANAGER_H
#define MOZILLA_GFX_GLMANAGER_H

#include "LayerManagerOGL.h"

namespace mozilla {
namespace gl {
class GLContext;
}
namespace layers {






class GLManager
{
public:
  static GLManager* CreateGLManager(LayerManager* aManager);

  virtual ~GLManager() {}

  virtual gl::GLContext* gl() const = 0;
  virtual ShaderProgramOGL* GetProgram(gl::ShaderProgramType aType) = 0;
  virtual void BindAndDrawQuad(ShaderProgramOGL *aProg) = 0;

};

}
}
#endif
