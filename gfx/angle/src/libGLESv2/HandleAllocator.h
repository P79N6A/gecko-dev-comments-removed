








#ifndef LIBGLESV2_HANDLEALLOCATOR_H_
#define LIBGLESV2_HANDLEALLOCATOR_H_

#define GL_APICALL
#include <GLES2/gl2.h>

#include <vector>

#include "common/angleutils.h"

namespace gl
{

class HandleAllocator
{
  public:
    HandleAllocator();
    virtual ~HandleAllocator();

    void setBaseHandle(GLuint value);

    GLuint allocate();
    void release(GLuint handle);

  private:
    DISALLOW_COPY_AND_ASSIGN(HandleAllocator);

    GLuint mBaseValue;
    GLuint mNextValue;
    typedef std::vector<GLuint> HandleList;
    HandleList mFreeValues;
};

}

#endif   
