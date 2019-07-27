







#include "libGLESv2/Query.h"
#include "libGLESv2/renderer/QueryImpl.h"

namespace gl
{
Query::Query(rx::QueryImpl *impl, GLuint id)
    : RefCountObject(id),
      mQuery(impl)
{
}

Query::~Query()
{
    delete mQuery;
}

void Query::begin()
{
    
    
    
    
    mStarted = mQuery->begin();
}

void Query::end()
{
    mQuery->end();
}

GLuint Query::getResult()
{
    return mQuery->getResult();
}

GLboolean Query::isResultAvailable()
{
    return mQuery->isResultAvailable();
}

GLenum Query::getType() const
{
    return mQuery->getType();
}

bool Query::isStarted() const
{
    return mStarted;
}

}
