







#ifndef LIBGLESV2_RENDERER_QUERY11_H_
#define LIBGLESV2_RENDERER_QUERY11_H_

#include "libGLESv2/renderer/QueryImpl.h"

namespace rx
{
class Renderer11;

class Query11 : public QueryImpl
{
  public:
    Query11(rx::Renderer11 *renderer, GLenum type);
    virtual ~Query11();

    virtual gl::Error begin();
    virtual gl::Error end();
    virtual gl::Error getResult(GLuint *params);
    virtual gl::Error isResultAvailable(GLuint *available);

  private:
    DISALLOW_COPY_AND_ASSIGN(Query11);

    gl::Error testQuery();

    GLuint mResult;

    bool mQueryFinished;

    rx::Renderer11 *mRenderer;
    ID3D11Query *mQuery;
};

}

#endif 
