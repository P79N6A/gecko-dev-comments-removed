




#include "WebGLObjectModel.h"

#include "WebGLContext.h"

namespace mozilla {

WebGLContextBoundObject::WebGLContextBoundObject(WebGLContext* webgl)
    : mContext(webgl)
    , mContextGeneration(webgl->Generation())
{
}

bool
WebGLContextBoundObject::IsCompatibleWithContext(WebGLContext* other)
{
    return (mContext == other &&
            mContextGeneration == other->Generation());
}

} 
