




#include "WebGL2Context.h"
#include "WebGLActiveInfo.h"
#include "WebGLProgram.h"
#include "WebGLTransformFeedback.h"
#include "GLContext.h"

using namespace mozilla;
using namespace mozilla::dom;




already_AddRefed<WebGLTransformFeedback>
WebGL2Context::CreateTransformFeedback()
{
    if (IsContextLost())
        return nullptr;

    GLuint tf = 0;
    MakeContextCurrent();
    gl->fGenTransformFeedbacks(1, &tf);

    nsRefPtr<WebGLTransformFeedback> globj = new WebGLTransformFeedback(this, tf);
    return globj.forget();
}

void
WebGL2Context::DeleteTransformFeedback(WebGLTransformFeedback* tf)
{
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowDeletedOrNull("deleteTransformFeedback", tf))
        return;

    if (!tf || tf->IsDeleted())
        return;

    if (mBoundTransformFeedback == tf)
        BindTransformFeedback(LOCAL_GL_TRANSFORM_FEEDBACK, tf);

    tf->RequestDelete();
}

bool
WebGL2Context::IsTransformFeedback(WebGLTransformFeedback* tf)
{
    if (IsContextLost())
        return false;

    if (!ValidateObjectAllowDeleted("isTransformFeedback", tf))
        return false;

    if (tf->IsDeleted())
        return false;

    MakeContextCurrent();
    return gl->fIsTransformFeedback(tf->GLName());
}

void
WebGL2Context::BindTransformFeedback(GLenum target, WebGLTransformFeedback* tf)
{
    if (IsContextLost())
        return;

    if (!ValidateObjectAllowDeletedOrNull("bindTransformFeedback", tf))
        return;

    if (target != LOCAL_GL_TRANSFORM_FEEDBACK)
        return ErrorInvalidEnum("bindTransformFeedback: target must be TRANSFORM_FEEDBACK");

    WebGLRefPtr<WebGLTransformFeedback> currentTF = mBoundTransformFeedback;
    if (currentTF && currentTF->mIsActive && !currentTF->mIsPaused) {
        return ErrorInvalidOperation("bindTransformFeedback: Currently bound transform "
                                     "feedback is active and not paused");
    }

    if (tf && tf->IsDeleted())
        return ErrorInvalidOperation("bindTransformFeedback: Attempt to bind deleted id");

    if (tf)
        tf->BindTo(LOCAL_GL_TRANSFORM_FEEDBACK);

    MakeContextCurrent();
    gl->fBindTransformFeedback(target, tf ? tf->GLName() : 0);
    if (tf)
        mBoundTransformFeedback = tf;
    else
        mBoundTransformFeedback = mDefaultTransformFeedback;
}

void
WebGL2Context::BeginTransformFeedback(GLenum primitiveMode)
{
    if (IsContextLost())
        return;

    WebGLTransformFeedback* tf = mBoundTransformFeedback;
    MOZ_ASSERT(tf);
    if (!tf)
        return;

    if (tf->mIsActive)
        return ErrorInvalidOperation("beginTransformFeedback: transform feedback is active");

    const GLenum mode = tf->mMode;
    if (mode != LOCAL_GL_POINTS && mode != LOCAL_GL_LINES && mode != LOCAL_GL_TRIANGLES)
        return ErrorInvalidEnum("beginTransformFeedback: primitive must be one of POINTS, LINES, or TRIANGLES");

    
    
    
    
    

    
    
    
    
    if (!mCurrentProgram)
        return ErrorInvalidOperation("beginTransformFeedback: no program is active");

    MakeContextCurrent();
    gl->fBeginTransformFeedback(primitiveMode);
    tf->mIsActive = true;
    tf->mIsPaused = false;
}

void
WebGL2Context::EndTransformFeedback()
{
    if (IsContextLost())
        return;

    WebGLTransformFeedback* tf = mBoundTransformFeedback;
    MOZ_ASSERT(tf);

    if (!tf)
        return;

    if (!tf->mIsActive)
        return ErrorInvalidOperation("%s: transform feedback in not active",
                                     "endTransformFeedback");

    MakeContextCurrent();
    gl->fEndTransformFeedback();
    tf->mIsActive = false;
    tf->mIsPaused = false;
}

void
WebGL2Context::PauseTransformFeedback()
{
    if (IsContextLost())
        return;

    WebGLTransformFeedback* tf = mBoundTransformFeedback;
    MOZ_ASSERT(tf);
    if (!tf)
        return;

    if (!tf->mIsActive || tf->mIsPaused) {
        return ErrorInvalidOperation("%s: transform feedback is not active or is paused",
                                     "pauseTransformFeedback");
    }

    MakeContextCurrent();
    gl->fPauseTransformFeedback();
    tf->mIsPaused = true;
}

void
WebGL2Context::ResumeTransformFeedback()
{
    if (IsContextLost())
        return;

    WebGLTransformFeedback* tf = mBoundTransformFeedback;
    MOZ_ASSERT(tf);
    if (!tf)
        return;

    if (!tf->mIsActive || !tf->mIsPaused)
        return ErrorInvalidOperation("resumeTransformFeedback: transform feedback is not active or is not paused");

    MakeContextCurrent();
    gl->fResumeTransformFeedback();
    tf->mIsPaused = false;
}

void
WebGL2Context::TransformFeedbackVaryings(WebGLProgram* program,
                                         const dom::Sequence<nsString>& varyings,
                                         GLenum bufferMode)
{
    if (IsContextLost())
        return;

    if (!ValidateObject("transformFeedbackVaryings: program", program))
        return;

    program->TransformFeedbackVaryings(varyings, bufferMode);
}

already_AddRefed<WebGLActiveInfo>
WebGL2Context::GetTransformFeedbackVarying(WebGLProgram* program, GLuint index)
{
    if (IsContextLost())
        return nullptr;

    if (!ValidateObject("getTransformFeedbackVarying: program", program))
        return nullptr;

    return program->GetTransformFeedbackVarying(index);
}
