




#include "mozilla/RefPtr.h"
#include "skia/GrGLInterface.h"
#include "skia/GrContext.h"

namespace mozilla {
namespace gl {

class GLContext;

class SkiaGLGlue : public GenericAtomicRefCounted
{
public:
    SkiaGLGlue(GLContext* context);
    GLContext* GetGLContext() const { return mGLContext.get(); }
    GrContext* GetGrContext() const { return mGrContext.get(); }
private:
    





    RefPtr<GLContext> mGLContext;
    SkRefPtr<GrGLInterface> mGrGLInterface;
    SkRefPtr<GrContext> mGrContext;
};

}
}
