





#include "libGLESv2/TransformFeedback.h"

namespace gl
{

TransformFeedback::TransformFeedback(GLuint id)
    : RefCountObject(id),
      mStarted(GL_FALSE),
      mPrimitiveMode(GL_NONE),
      mPaused(GL_FALSE)
{
}

TransformFeedback::~TransformFeedback()
{
}

void TransformFeedback::start(GLenum primitiveMode)
{
    mStarted = GL_TRUE;
    mPrimitiveMode = primitiveMode;
    mPaused = GL_FALSE;
}

void TransformFeedback::stop()
{
    mStarted = GL_FALSE;
    mPrimitiveMode = GL_NONE;
    mPaused = GL_FALSE;
}

GLboolean TransformFeedback::isStarted() const
{
    return mStarted;
}

GLenum TransformFeedback::getDrawMode() const
{
    return mPrimitiveMode;
}

void TransformFeedback::pause()
{
    mPaused = GL_TRUE;
}

void TransformFeedback::resume()
{
    mPaused = GL_FALSE;
}

GLboolean TransformFeedback::isPaused() const
{
    return mPaused;
}

}
