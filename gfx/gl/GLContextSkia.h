




#include "skia/GrGLInterface.h"

namespace mozilla {
namespace gl {
class GLContext;
}
}

GrGLInterface* CreateGrInterfaceFromGLContext(mozilla::gl::GLContext* context);
