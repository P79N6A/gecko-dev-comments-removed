




#include "WebGL1Context.h"

using namespace mozilla;

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
