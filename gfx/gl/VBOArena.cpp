




#include "VBOArena.h"
#include "GLContext.h"

using namespace mozilla::gl;

GLuint VBOArena::Allocate(GLContext *aGLContext)
{
    if (!mAvailableVBOs.size()) {
        GLuint vbo;
        aGLContext->fGenBuffers(1, &vbo);
        mAllocatedVBOs.push_back(vbo);
        return vbo;
    }
    GLuint vbo = mAvailableVBOs.back();
    mAvailableVBOs.pop_back();
    return vbo;
}

void VBOArena::Reset()
{
    mAvailableVBOs = mAllocatedVBOs;
}

void VBOArena::Flush(GLContext *aGLContext)
{
    if (mAvailableVBOs.size()) {
#ifdef DEBUG
        printf_stderr("VBOArena::Flush for %u VBOs\n", mAvailableVBOs.size());
#endif
        aGLContext->fDeleteBuffers(mAvailableVBOs.size(), &mAvailableVBOs[0]);
    }
}
