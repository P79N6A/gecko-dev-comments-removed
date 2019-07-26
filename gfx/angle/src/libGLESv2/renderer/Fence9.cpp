#include "precompiled.h"








#include "libGLESv2/renderer/Fence9.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/renderer9_utils.h"
#include "libGLESv2/renderer/Renderer9.h"

namespace rx
{

Fence9::Fence9(rx::Renderer9 *renderer)
{
    mRenderer = renderer;
    mQuery = NULL;
}

Fence9::~Fence9()
{
    if (mQuery)
    {
        mRenderer->freeEventQuery(mQuery);
        mQuery = NULL;
    }
}

GLboolean Fence9::isFence()
{
    
    
    return mQuery != NULL;
}

void Fence9::setFence(GLenum condition)
{
    if (!mQuery)
    {
        mQuery = mRenderer->allocateEventQuery();
        if (!mQuery)
        {
            return gl::error(GL_OUT_OF_MEMORY);
        }
    }

    HRESULT result = mQuery->Issue(D3DISSUE_END);
    ASSERT(SUCCEEDED(result));

    setCondition(condition);
    setStatus(GL_FALSE);
}

GLboolean Fence9::testFence()
{
    if (mQuery == NULL)
    {
        return gl::error(GL_INVALID_OPERATION, GL_TRUE);
    }

    HRESULT result = mQuery->GetData(NULL, 0, D3DGETDATA_FLUSH);

    if (d3d9::isDeviceLostError(result))
    {
        mRenderer->notifyDeviceLost();
        return gl::error(GL_OUT_OF_MEMORY, GL_TRUE);
    }

    ASSERT(result == S_OK || result == S_FALSE);
    setStatus(result == S_OK);
    return getStatus();
}

void Fence9::finishFence()
{
    if (mQuery == NULL)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    while (!testFence())
    {
        Sleep(0);
    }
}

void Fence9::getFenceiv(GLenum pname, GLint *params)
{
    if (mQuery == NULL)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    switch (pname)
    {
        case GL_FENCE_STATUS_NV:
        {
            
            
            
            if (getStatus())
            {
                params[0] = GL_TRUE;
                return;
            }
            
            HRESULT result = mQuery->GetData(NULL, 0, 0);

            if (d3d9::isDeviceLostError(result))
            {
                params[0] = GL_TRUE;
                mRenderer->notifyDeviceLost();
                return gl::error(GL_OUT_OF_MEMORY);
            }

            ASSERT(result == S_OK || result == S_FALSE);
            setStatus(result == S_OK);
            params[0] = getStatus();

            break;
        }
        case GL_FENCE_CONDITION_NV:
            params[0] = getCondition();
            break;
        default:
            return gl::error(GL_INVALID_ENUM);
            break;
    }
}

}
