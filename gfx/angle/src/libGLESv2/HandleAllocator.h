








#ifndef LIBGLESV2_HANDLEALLOCATOR_H_
#define LIBGLESV2_HANDLEALLOCATOR_H_

#include "angle_gl.h"

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
