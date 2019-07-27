







#ifndef LIBGLESV2_RENDERER_QUERYIMPL_H_
#define LIBGLESV2_RENDERER_QUERYIMPL_H_

#include "common/angleutils.h"

namespace rx
{

class QueryImpl
{
  public:
    explicit QueryImpl(GLenum type) : mType(type), mStatus(GL_FALSE), mResult(0) { }
    virtual ~QueryImpl() { }

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual GLuint getResult() = 0;
    virtual GLboolean isResultAvailable() = 0;
    virtual bool isStarted() const = 0;

    GLenum getType() const { return mType; }

  protected:
    GLuint mResult;
    GLboolean mStatus;

  private:
    DISALLOW_COPY_AND_ASSIGN(QueryImpl);

    GLenum mType;
};

}

#endif 
