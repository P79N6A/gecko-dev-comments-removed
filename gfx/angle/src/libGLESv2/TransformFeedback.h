





#ifndef LIBGLESV2_TRANSFORM_FEEDBACK_H_
#define LIBGLESV2_TRANSFORM_FEEDBACK_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"

#include "angle_gl.h"

namespace gl
{

class TransformFeedback : public RefCountObject
{
  public:
    explicit TransformFeedback(GLuint id);
    virtual ~TransformFeedback();

    void start(GLenum primitiveMode);
    void stop();
    GLboolean isStarted() const;

    GLenum getDrawMode() const;

    void pause();
    void resume();
    GLboolean isPaused() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TransformFeedback);

    GLboolean mStarted;
    GLenum mPrimitiveMode;
    GLboolean mPaused;
};

}

#endif 
