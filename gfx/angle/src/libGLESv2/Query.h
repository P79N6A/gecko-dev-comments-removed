







#ifndef LIBGLESV2_QUERY_H_
#define LIBGLESV2_QUERY_H_

#include "angle_gl.h"

#include "common/angleutils.h"
#include "common/RefCountObject.h"

namespace rx
{
class Renderer;
class QueryImpl;
}

namespace gl
{

class Query : public RefCountObject
{
  public:
    Query(rx::Renderer *renderer, GLenum type, GLuint id);
    virtual ~Query();

    void begin();
    void end();

    GLuint getResult();
    GLboolean isResultAvailable();

    GLenum getType() const;
    bool isStarted() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Query);

    rx::QueryImpl *mQuery;
};

}

#endif   
