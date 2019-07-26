




#ifndef VBOARENA_H_
#define VBOARENA_H_

#include "GLTypes.h"
#include <vector>

namespace mozilla {
namespace gl {

class GLContext;

class VBOArena {
public:
    
    GLuint Allocate(GLContext *aGLContext);

    
    void Reset();

    
    void Flush(GLContext *aGLContext);
private:
    std::vector<GLuint> mAllocatedVBOs;
    std::vector<GLuint> mAvailableVBOs;
};

}
}

#endif
