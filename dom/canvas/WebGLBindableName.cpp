




#include "WebGLBindableName.h"
#include "GLConsts.h"
#include "mozilla/Assertions.h"

using namespace mozilla;

WebGLBindableName::WebGLBindableName()
    : mGLName(LOCAL_GL_NONE)
    , mTarget(LOCAL_GL_NONE)
{ }

void
WebGLBindableName::BindTo(GLenum target)
{
    MOZ_ASSERT(target != LOCAL_GL_NONE, "Can't bind to GL_NONE.");
    MOZ_ASSERT(mTarget == LOCAL_GL_NONE || mTarget == target, "Rebinding is illegal.");

    bool targetChanged = (target != mTarget);
    mTarget = target;
    if (targetChanged)
        OnTargetChanged();
}
