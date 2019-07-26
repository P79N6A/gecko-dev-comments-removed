





#include "TextureGarbageBin.h"
#include "GLContext.h"

using namespace mozilla;
using namespace mozilla::gl;

void
TextureGarbageBin::GLContextTeardown()
{
    EmptyGarbage();

    MutexAutoLock lock(mMutex);
    mGL = nullptr;
}

void
TextureGarbageBin::Trash(GLuint tex)
{
    MutexAutoLock lock(mMutex);
    if (!mGL)
        return;

    mGarbageTextures.push(tex);
}

void
TextureGarbageBin::EmptyGarbage()
{
    MutexAutoLock lock(mMutex);
    if (!mGL)
        return;

    while (!mGarbageTextures.empty()) {
        GLuint tex = mGarbageTextures.top();
        mGarbageTextures.pop();
        mGL->fDeleteTextures(1, &tex);
    }
}
