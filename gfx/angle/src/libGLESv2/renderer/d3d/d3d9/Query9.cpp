#include "precompiled.h"









#include "libGLESv2/renderer/d3d/d3d9/Query9.h"
#include "libGLESv2/main.h"
#include "libGLESv2/renderer/d3d/d3d9/renderer9_utils.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"

namespace rx
{

Query9::Query9(rx::Renderer9 *renderer, GLenum type) : QueryImpl(type)
{
    mRenderer = renderer;
    mQuery = NULL;
}

Query9::~Query9()
{
    SafeRelease(mQuery);
}

void Query9::begin()
{
    if (mQuery == NULL)
    {
        if (FAILED(mRenderer->getDevice()->CreateQuery(D3DQUERYTYPE_OCCLUSION, &mQuery)))
        {
            return gl::error(GL_OUT_OF_MEMORY);
        }
    }

    HRESULT result = mQuery->Issue(D3DISSUE_BEGIN);
    UNUSED_ASSERTION_VARIABLE(result);
    ASSERT(SUCCEEDED(result));
}

void Query9::end()
{
    ASSERT(mQuery);

    HRESULT result = mQuery->Issue(D3DISSUE_END);
    UNUSED_ASSERTION_VARIABLE(result);
    ASSERT(SUCCEEDED(result));

    mStatus = GL_FALSE;
    mResult = GL_FALSE;
}

GLuint Query9::getResult()
{
    if (mQuery != NULL)
    {
        while (!testQuery())
        {
            Sleep(0);
            
            
            
            if (mRenderer->testDeviceLost(true))
            {
                return gl::error(GL_OUT_OF_MEMORY, 0);
            }
        }
    }

    return mResult;
}

GLboolean Query9::isResultAvailable()
{
    if (mQuery != NULL)
    {
        testQuery();
    }

    return mStatus;
}

GLboolean Query9::testQuery()
{
    if (mQuery != NULL && mStatus != GL_TRUE)
    {
        DWORD numPixels = 0;

        HRESULT hres = mQuery->GetData(&numPixels, sizeof(DWORD), D3DGETDATA_FLUSH);
        if (hres == S_OK)
        {
            mStatus =  GL_TRUE;

            switch (getType())
            {
              case GL_ANY_SAMPLES_PASSED_EXT:
              case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
                mResult = (numPixels > 0) ? GL_TRUE : GL_FALSE;
                break;
              default:
                ASSERT(false);
            }
        }
        else if (d3d9::isDeviceLostError(hres))
        {
            mRenderer->notifyDeviceLost();
            return gl::error(GL_OUT_OF_MEMORY, GL_TRUE);
        }

        return mStatus;
    }

    return GL_TRUE; 
}

bool Query9::isStarted() const
{
    return (mQuery != NULL);
}

}
