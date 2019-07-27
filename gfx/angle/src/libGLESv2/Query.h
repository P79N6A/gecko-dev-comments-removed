







#ifndef LIBGLESV2_QUERY_H_
#define LIBGLESV2_QUERY_H_

#include "libGLESv2/Error.h"
#include "common/angleutils.h"
#include "common/RefCountObject.h"

#include "angle_gl.h"

namespace rx
{
class QueryImpl;
}

namespace gl
{

class Query : public RefCountObject
{
  public:
    Query(rx::QueryImpl *impl, GLuint id);
    virtual ~Query();

    Error begin();
    Error end();

    Error getResult(GLuint *params);
    Error isResultAvailable(GLuint *available);

    GLenum getType() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Query);

    rx::QueryImpl *mQuery;
};

}

#endif   
