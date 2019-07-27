




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

  virtual GLContext* gl() const override
  {
    return mImpl->gl();
  }

  virtual void ActivateProgram(ShaderProgramOGL *aProg) override
  {
    mImpl->ActivateProgram(aProg);
  }

  virtual ShaderProgramOGL* GetProgram(GLenum aTarget, gfx::SurfaceFormat aFormat) override
  {
    ShaderConfigOGL config = ShaderConfigFromTargetAndFormat(aTarget, aFormat);
    return mImpl->GetShaderProgramFor(config);
  }

  virtual const gfx::Matrix4x4& GetProjMatrix() const override
  {
    return mImpl->GetProjMatrix();
  }

  virtual void BindAndDrawQuad(ShaderProgramOGL *aProg,
                               const gfx::Rect& aLayerRect,
                               const gfx::Rect& aTextureRect) override
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
