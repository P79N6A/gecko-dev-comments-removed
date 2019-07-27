




#include "WebGL2Context.h"
#include "GLContext.h"

using namespace mozilla;
using namespace mozilla::dom;




void
WebGL2Context::CopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset,
                                 GLintptr writeOffset, GLsizeiptr size)
{
    MOZ_CRASH("Not Implemented.");
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
