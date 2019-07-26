




#include "mozilla/RefPtr.h"

#ifdef USE_SKIA_GPU

#include "GLContext.h"
#include "skia/GrGLInterface.h"
#include "skia/GrContext.h"

namespace mozilla {
namespace gl {

class SkiaGLGlue : public GenericAtomicRefCounted
{
public:
  SkiaGLGlue(GLContext* context);
  GLContext* GetGLContext() const { return mGLContext.get(); }
  GrContext* GetGrContext() const { return mGrContext.get(); }

protected:
  virtual ~SkiaGLGlue() {
    




    mGrContext = nullptr;
    mGrGLInterface = nullptr;
    mGLContext = nullptr;
  }

private:
  RefPtr<GLContext> mGLContext;
  SkRefPtr<GrGLInterface> mGrGLInterface;
  SkRefPtr<GrContext> mGrContext;
};

}
}

#else

class GrContext;

namespace mozilla {
namespace gl {

class GLContext;

class SkiaGLGlue : public GenericAtomicRefCounted
{
public:
  SkiaGLGlue(GLContext* context);
  GLContext* GetGLContext() const { return nullptr; }
  GrContext* GetGrContext() const { return nullptr; }
};
}
}

#endif