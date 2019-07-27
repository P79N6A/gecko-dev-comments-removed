







#ifndef LIBGLESV2_QUERY_H_
#define LIBGLESV2_QUERY_H_

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

    void begin();
    void end();

    GLuint getResult();
    GLboolean isResultAvailable();

    GLenum getType() const;
    bool isStarted() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Query);

    bool mStarted;

    rx::QueryImpl *mQuery;
};

}

#endif   
