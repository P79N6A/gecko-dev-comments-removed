







#ifndef LIBGLESV2_RENDERER_QUERYIMPL_H_
#define LIBGLESV2_RENDERER_QUERYIMPL_H_

#include "common/angleutils.h"

#include <GLES2/gl2.h>

namespace rx
{

class QueryImpl
{
  public:
    explicit QueryImpl(GLenum type) { mType = type; }
    virtual ~QueryImpl() { }

    virtual bool begin() = 0;
    virtual void end() = 0;
    virtual GLuint getResult() = 0;
    virtual GLboolean isResultAvailable() = 0;

    GLenum getType() const { return mType;  }

  private:
    DISALLOW_COPY_AND_ASSIGN(QueryImpl);

    GLenum mType;
};

}

#endif 
