




#include "WebGL1Context.h"

namespace mozilla {

bool
WebGL1Context::ValidateAttribPointerType(bool , GLenum type, GLsizei* out_alignment, const char* info)
{
    MOZ_ASSERT(out_alignment);
    if (!out_alignment)
        return false;

    switch (type) {
    case LOCAL_GL_BYTE:
    case LOCAL_GL_UNSIGNED_BYTE:
        *out_alignment = 1;
        return true;

    case LOCAL_GL_SHORT:
    case LOCAL_GL_UNSIGNED_SHORT:
        *out_alignment = 2;
        return true;
        
    case LOCAL_GL_FLOAT:
        *out_alignment = 4;
        return true;
    }

    ErrorInvalidEnumInfo(info, type);
    return false;
}

bool
WebGL1Context::ValidateUniformMatrixTranspose(bool transpose, const char* info)
{
    if (transpose) {
        ErrorInvalidValue("%s: transpose must be FALSE as per the "
                          "OpenGL ES 2.0 spec", info);
        return false;
    }

    return true;
}

} 
