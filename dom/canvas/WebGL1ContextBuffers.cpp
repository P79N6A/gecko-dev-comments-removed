




#include "WebGL1Context.h"
#include "WebGLBuffer.h"
#include "GLContext.h"

using namespace mozilla;
using namespace mozilla::dom;





bool
WebGL1Context::ValidateBufferTarget(GLenum target, const char* info)
{
    switch (target) {
    case LOCAL_GL_ARRAY_BUFFER:
    case LOCAL_GL_ELEMENT_ARRAY_BUFFER:
        return true;

    default:
        ErrorInvalidEnumInfo(info, target);
        return false;
    }
}

bool
WebGL1Context::ValidateBufferIndexedTarget(GLenum target, const char* info)
{
    ErrorInvalidEnumInfo(info, target);
    return false;
}
