




#include "WebGL2Context.h"
#include "GLContext.h"

using namespace mozilla;
using namespace mozilla::dom;

bool
WebGL2Context::ValidateBufferTarget(GLenum target, const char* info)
{
    switch (target) {
    case LOCAL_GL_ARRAY_BUFFER:
    case LOCAL_GL_ELEMENT_ARRAY_BUFFER:
    case LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER:
        return true;

    default:
        ErrorInvalidEnumInfo(info, target);
        return false;
    }
}

bool
WebGL2Context::ValidateBufferIndexedTarget(GLenum target, const char* info)
{
    switch (target) {
    case LOCAL_GL_TRANSFORM_FEEDBACK_BUFFER:
    case LOCAL_GL_UNIFORM_BUFFER:
        return true;

    default:
        ErrorInvalidEnumInfo(info, target);
        return false;
    }
}




void
WebGL2Context::CopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset,
                                 GLintptr writeOffset, GLsizeiptr size)
{
    MakeContextCurrent();
    gl->fCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
}

void
WebGL2Context::GetBufferSubData(GLenum target, GLintptr offset, const dom::ArrayBuffer& returnedData)
{
    MOZ_CRASH("Not Implemented.");
}

void
WebGL2Context::GetBufferSubData(GLenum target, GLintptr offset, const dom::ArrayBufferView& returnedData)
{
    MOZ_CRASH("Not Implemented.");
}
