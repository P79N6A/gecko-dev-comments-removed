




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
  GLManagerCompositor(CompositorOGL* aCompositor)
    : mImpl(aCompositor)
  {}

  virtual GLContext* gl() const MOZ_OVERRIDE
  {
    return mImpl->gl();
  }

  virtual ShaderProgramOGL* GetProgram(ShaderProgramType aType) MOZ_OVERRIDE
  {
    return mImpl->GetProgram(aType);
  }

  virtual void BindAndDrawQuad(ShaderProgramOGL *aProg) MOZ_OVERRIDE
  {
    mImpl->BindAndDrawQuad(aProg);
  }

private:
  RefPtr<CompositorOGL> mImpl;
};

 GLManager*
GLManager::CreateGLManager(LayerManagerComposite* aManager)
{
  if (aManager &&
      Compositor::GetBackend() == LAYERS_OPENGL) {
    return new GLManagerCompositor(static_cast<CompositorOGL*>(
      aManager->GetCompositor()));
  }
  return nullptr;
}

}
}
