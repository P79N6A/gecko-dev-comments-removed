







#ifndef LIBGLESV2_RENDERER_TRANSFORMFEEDBACKIMPL_H_
#define LIBGLESV2_RENDERER_TRANSFORMFEEDBACKIMPL_H_

#include "common/angleutils.h"
#include "libGLESv2/TransformFeedback.h"

namespace rx
{

class TransformFeedbackImpl
{
  public:
    virtual ~TransformFeedbackImpl() { }

    virtual void begin(GLenum primitiveMode) = 0;
    virtual void end() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
};

}

#endif 
