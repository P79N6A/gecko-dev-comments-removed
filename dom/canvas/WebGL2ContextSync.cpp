




#include "WebGL2Context.h"
#include "GLContext.h"

using namespace mozilla;
using namespace mozilla::dom;




already_AddRefed<WebGLSync>
WebGL2Context::FenceSync(GLenum condition, GLbitfield flags)
{
    MOZ_CRASH("Not Implemented.");
    return nullptr;
}

bool
WebGL2Context::IsSync(WebGLSync* sync)
{
    MOZ_CRASH("Not Implemented.");
    return false;
}

void
WebGL2Context::DeleteSync(WebGLSync* sync)
{
    MOZ_CRASH("Not Implemented.");
}

GLenum
WebGL2Context::ClientWaitSync(WebGLSync* sync, GLbitfield flags, GLuint64 timeout)
{
    MOZ_CRASH("Not Implemented.");
    return LOCAL_GL_FALSE;
}

void
WebGL2Context::WaitSync(WebGLSync* sync, GLbitfield flags, GLuint64 timeout)
{
    MOZ_CRASH("Not Implemented.");
}

void
WebGL2Context::GetSyncParameter(JSContext*, WebGLSync* sync, GLenum pname, JS::MutableHandleValue retval)
{
    MOZ_CRASH("Not Implemented.");
}
