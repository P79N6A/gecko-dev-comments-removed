




#ifndef MOZILLA_GFX_GLMANAGER_H
#define MOZILLA_GFX_GLMANAGER_H

#include "mozilla/gfx/Types.h"          
#include "OGLShaderProgram.h"

namespace mozilla {
namespace gl {
class GLContext;
}

namespace layers {

class LayerManagerComposite;






class GLManager
{
public:
  static GLManager* CreateGLManager(LayerManagerComposite* aManager);

  virtual ~GLManager() {}

  virtual gl::GLContext* gl() const = 0;
  virtual ShaderProgramOGL* GetProgram(GLenum aTarget, gfx::SurfaceFormat aFormat) = 0;
  virtual const gfx::Matrix4x4& GetProjMatrix() const = 0;
  virtual void BindAndDrawQuad(ShaderProgramOGL *aProg, const gfx::Rect& aRect) = 0;
};

}
}
#endif
