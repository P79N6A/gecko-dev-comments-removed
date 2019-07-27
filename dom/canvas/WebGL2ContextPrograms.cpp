




#include "WebGL2Context.h"

#include "GLContext.h"
#include "WebGLProgram.h"

namespace mozilla {




GLint
WebGL2Context::GetFragDataLocation(WebGLProgram* prog, const nsAString& name)
{
    if (IsContextLost())
        return -1;

    if (!ValidateObject("getFragDataLocation: program", prog))
        return -1;

    return prog->GetFragDataLocation(name);
}

} 
