







#include "libGLESv2/renderer/d3d/d3d11/Fence11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/main.h"

#include "common/utilities.h"

namespace rx
{





template<class FenceClass>
gl::Error FenceSetHelper(FenceClass *fence)
{
    if (!fence->mQuery)
    {
        D3D11_QUERY_DESC queryDesc;
        queryDesc.Query = D3D11_QUERY_EVENT;
        queryDesc.MiscFlags = 0;

        HRESULT result = fence->mRenderer->getDevice()->CreateQuery(&queryDesc, &fence->mQuery);
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create event query, result: 0x%X.", result);
        }
    }

    fence->mRenderer->getDeviceContext()->End(fence->mQuery);
    return gl::Error(GL_NO_ERROR);
}

template <class FenceClass>
gl::Error FenceTestHelper(FenceClass *fence, bool flushCommandBuffer, GLboolean *outFinished)
{
    ASSERT(fence->mQuery);

    UINT getDataFlags = (flushCommandBuffer ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);
    HRESULT result = fence->mRenderer->getDeviceContext()->GetData(fence->mQuery, NULL, 0, getDataFlags);

    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to get query data, result: 0x%X.", result);
    }
    else if (fence->mRenderer->isDeviceLost())
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Device was lost while querying result of an event query.");
    }

    ASSERT(result == S_OK || result == S_FALSE);
    *outFinished = ((result == S_OK) ? GL_TRUE : GL_FALSE);
    return gl::Error(GL_NO_ERROR);
}





FenceNV11::FenceNV11(Renderer11 *renderer)
    : FenceNVImpl(),
      mRenderer(renderer),
      mQuery(NULL)
{
}

FenceNV11::~FenceNV11()
{
    SafeRelease(mQuery);
}

gl::Error FenceNV11::set()
{
    return FenceSetHelper(this);
}

gl::Error FenceNV11::test(bool flushCommandBuffer, GLboolean *outFinished)
{
    return FenceTestHelper(this, flushCommandBuffer, outFinished);
}

gl::Error FenceNV11::finishFence(GLboolean *outFinished)
{
    ASSERT(outFinished);

    while (*outFinished != GL_TRUE)
    {
        gl::Error error = test(true, outFinished);
        if (error.isError())
        {
            return error;
        }

        Sleep(0);
    }

    return gl::Error(GL_NO_ERROR);
}
















FenceSync11::FenceSync11(Renderer11 *renderer)
    : FenceSyncImpl(),
      mRenderer(renderer),
      mQuery(NULL)
{
    LARGE_INTEGER counterFreqency = { 0 };
    BOOL success = QueryPerformanceFrequency(&counterFreqency);
    UNUSED_ASSERTION_VARIABLE(success);
    ASSERT(success);

    mCounterFrequency = counterFreqency.QuadPart;
}

FenceSync11::~FenceSync11()
{
    SafeRelease(mQuery);
}

gl::Error FenceSync11::set()
{
    return FenceSetHelper(this);
}

gl::Error FenceSync11::clientWait(GLbitfield flags, GLuint64 timeout, GLenum *outResult)
{
    ASSERT(outResult);

    bool flushCommandBuffer = ((flags & GL_SYNC_FLUSH_COMMANDS_BIT) != 0);

    GLboolean result = GL_FALSE;
    gl::Error error = FenceTestHelper(this, flushCommandBuffer, &result);
    if (error.isError())
    {
        *outResult = GL_WAIT_FAILED;
        return error;
    }

    if (result == GL_TRUE)
    {
        *outResult = GL_ALREADY_SIGNALED;
        return gl::Error(GL_NO_ERROR);
    }

    if (timeout == 0)
    {
        *outResult = GL_TIMEOUT_EXPIRED;
        return gl::Error(GL_NO_ERROR);
    }

    LARGE_INTEGER currentCounter = { 0 };
    BOOL success = QueryPerformanceCounter(&currentCounter);
    UNUSED_ASSERTION_VARIABLE(success);
    ASSERT(success);

    LONGLONG timeoutInSeconds = static_cast<LONGLONG>(timeout) * static_cast<LONGLONG>(1000000ll);
    LONGLONG endCounter = currentCounter.QuadPart + mCounterFrequency * timeoutInSeconds;

    while (currentCounter.QuadPart < endCounter && !result)
    {
        Sleep(0);
        BOOL success = QueryPerformanceCounter(&currentCounter);
        UNUSED_ASSERTION_VARIABLE(success);
        ASSERT(success);

        error = FenceTestHelper(this, flushCommandBuffer, &result);
        if (error.isError())
        {
            *outResult = GL_WAIT_FAILED;
            return error;
        }
    }

    if (currentCounter.QuadPart >= endCounter)
    {
        *outResult = GL_TIMEOUT_EXPIRED;
    }
    else
    {
        *outResult = GL_CONDITION_SATISFIED;
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error FenceSync11::serverWait(GLbitfield flags, GLuint64 timeout)
{
    
    
    
    return gl::Error(GL_NO_ERROR);
}

gl::Error FenceSync11::getStatus(GLint *outResult)
{
    GLboolean result = GL_FALSE;
    gl::Error error = FenceTestHelper(this, false, &result);
    if (error.isError())
    {
        
        
        *outResult = GL_SIGNALED;

        return error;
    }

    *outResult = (result ? GL_SIGNALED : GL_UNSIGNALED);
    return gl::Error(GL_NO_ERROR);
}

} 
