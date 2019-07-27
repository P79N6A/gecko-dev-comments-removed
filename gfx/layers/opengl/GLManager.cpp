




#include "GLManager.h"
#include "CompositorOGL.h"              
#include "GLContext.h"                  
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

class GLManagerCompositor : public GLManager
{
public:
  explicit GLManagerCompositor(CompositorOGL* aCompositor)
    : mImpl(aCompositor)
  {}

  virtual GLContext* gl() const MOZ_OVERRIDE
  {
    return mImpl->gl();
  }

  virtual ShaderProgramOGL* GetProgram(GLenum aTarget, gfx::SurfaceFormat aFormat) MOZ_OVERRIDE
  {
    ShaderConfigOGL config = ShaderConfigFromTargetAndFormat(aTarget, aFormat);
    return mImpl->GetShaderProgramFor(config);
  }

  virtual const gfx::Matrix4x4& GetProjMatrix() const MOZ_OVERRIDE
  {
    return mImpl->GetProjMatrix();
  }

  virtual void BindAndDrawQuad(ShaderProgramOGL *aProg,
                               const gfx::Rect& aLayerRect,
                               const gfx::Rect& aTextureRect) MOZ_OVERRIDE
  {
    mImpl->BindAndDrawQuad(aProg, aLayerRect, aTextureRect);
  }

private:
  RefPtr<CompositorOGL> mImpl;
};

 GLManager*
GLManager::CreateGLManager(LayerManagerComposite* aManager)
{
  if (aManager &&
      Compositor::GetBackend() == LayersBackend::LAYERS_OPENGL) {
    return new GLManagerCompositor(static_cast<CompositorOGL*>(
      aManager->GetCompositor()));
  }
  return nullptr;
}

}
}
