#include "precompiled.h"








#include "libGLESv2/Query.h"
#include "libGLESv2/renderer/QueryImpl.h"
#include "libGLESv2/renderer/Renderer.h"

namespace gl
{

Query::Query(rx::Renderer *renderer, GLenum type, GLuint id) : RefCountObject(id)
{ 
    mQuery = renderer->createQuery(type);
}

Query::~Query()
{
    delete mQuery;
}

void Query::begin()
{
    mQuery->begin();
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
    return mQuery->isStarted();
}

}
