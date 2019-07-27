







#ifndef LIBGLESV2_RENDERER_QUERYIMPL_H_
#define LIBGLESV2_RENDERER_QUERYIMPL_H_

#include "libGLESv2/Error.h"

#include "common/angleutils.h"

#include <GLES2/gl2.h>

namespace rx
{

class QueryImpl
{
  public:
    explicit QueryImpl(GLenum type) { mType = type; }
    virtual ~QueryImpl() { }

    virtual gl::Error begin() = 0;
    virtual gl::Error end() = 0;
    virtual gl::Error getResult(GLuint *params) = 0;
    virtual gl::Error isResultAvailable(GLuint *available) = 0;

    GLenum getType() const { return mType;  }

  private:
    DISALLOW_COPY_AND_ASSIGN(QueryImpl);

    GLenum mType;
};

}

#endif 
