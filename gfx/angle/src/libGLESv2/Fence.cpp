


















#include "libGLESv2/Fence.h"
#include "libGLESv2/renderer/FenceImpl.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/main.h"

#include "angle_gl.h"

namespace gl
{

FenceNV::FenceNV(rx::Renderer *renderer)
{
    mFence = renderer->createFence();
}

FenceNV::~FenceNV()
{
    delete mFence;
}

GLboolean FenceNV::isFence() const
{
    
    
    return (mFence->isSet() ? GL_TRUE : GL_FALSE);
}

void FenceNV::setFence(GLenum condition)
{
    mFence->set();

    mCondition = condition;
    mStatus = GL_FALSE;
}

GLboolean FenceNV::testFence()
{
    
    bool result = mFence->test(true);

    mStatus = (result ? GL_TRUE : GL_FALSE);
    return mStatus;
}

void FenceNV::finishFence()
{
    ASSERT(mFence->isSet());

    while (!mFence->test(true))
    {
        Sleep(0);
    }
}

GLint FenceNV::getFencei(GLenum pname)
{
    ASSERT(mFence->isSet());

    switch (pname)
    {
      case GL_FENCE_STATUS_NV:
        {
            
            
            
            if (mStatus == GL_TRUE)
            {
                return GL_TRUE;
            }

            mStatus = (mFence->test(false) ? GL_TRUE : GL_FALSE);
            return mStatus;
        }

      case GL_FENCE_CONDITION_NV:
        return mCondition;

      default: UNREACHABLE(); return 0;
    }
}

FenceSync::FenceSync(rx::Renderer *renderer, GLuint id)
    : RefCountObject(id)
{
    mFence = renderer->createFence();

    LARGE_INTEGER counterFreqency = { 0 };
    BOOL success = QueryPerformanceFrequency(&counterFreqency);
    UNUSED_ASSERTION_VARIABLE(success);
    ASSERT(success);

    mCounterFrequency = counterFreqency.QuadPart;
}

FenceSync::~FenceSync()
{
    delete mFence;
}

void FenceSync::set(GLenum condition)
{
    mCondition = condition;
    mFence->set();
}

GLenum FenceSync::clientWait(GLbitfield flags, GLuint64 timeout)
{
    ASSERT(mFence->isSet());

    bool flushCommandBuffer = ((flags & GL_SYNC_FLUSH_COMMANDS_BIT) != 0);

    if (mFence->test(flushCommandBuffer))
    {
        return GL_ALREADY_SIGNALED;
    }

    if (mFence->hasError())
    {
        return GL_WAIT_FAILED;
    }

    if (timeout == 0)
    {
        return GL_TIMEOUT_EXPIRED;
    }

    LARGE_INTEGER currentCounter = { 0 };
    BOOL success = QueryPerformanceCounter(&currentCounter);
    UNUSED_ASSERTION_VARIABLE(success);
    ASSERT(success);

    LONGLONG timeoutInSeconds = static_cast<LONGLONG>(timeout) * static_cast<LONGLONG>(1000000ll);
    LONGLONG endCounter = currentCounter.QuadPart + mCounterFrequency * timeoutInSeconds;

    while (currentCounter.QuadPart < endCounter && !mFence->test(flushCommandBuffer))
    {
        Sleep(0);
        BOOL success = QueryPerformanceCounter(&currentCounter);
        UNUSED_ASSERTION_VARIABLE(success);
        ASSERT(success);
    }

    if (mFence->hasError())
    {
        return GL_WAIT_FAILED;
    }

    if (currentCounter.QuadPart >= endCounter)
    {
        return GL_TIMEOUT_EXPIRED;
    }

    return GL_CONDITION_SATISFIED;
}

void FenceSync::serverWait()
{
    
    
    
}

GLenum FenceSync::getStatus() const
{
    if (mFence->test(false))
    {
        
        
        return GL_SIGNALED;
    }

    return GL_UNSIGNALED;
}

}
