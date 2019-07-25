









#include "libGLESv2/Renderbuffer.h"

#include "libGLESv2/main.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/utilities.h"

namespace gl
{
unsigned int RenderbufferStorage::mCurrentSerial = 1;

RenderbufferInterface::RenderbufferInterface()
{
}




void RenderbufferInterface::addProxyRef(const Renderbuffer *proxy)
{
}

void RenderbufferInterface::releaseProxy(const Renderbuffer *proxy)
{
}

GLuint RenderbufferInterface::getRedSize() const
{
    return dx2es::GetRedSize(getD3DFormat());
}

GLuint RenderbufferInterface::getGreenSize() const
{
    return dx2es::GetGreenSize(getD3DFormat());
}

GLuint RenderbufferInterface::getBlueSize() const
{
    return dx2es::GetBlueSize(getD3DFormat());
}

GLuint RenderbufferInterface::getAlphaSize() const
{
    return dx2es::GetAlphaSize(getD3DFormat());
}

GLuint RenderbufferInterface::getDepthSize() const
{
    return dx2es::GetDepthSize(getD3DFormat());
}

GLuint RenderbufferInterface::getStencilSize() const
{
    return dx2es::GetStencilSize(getD3DFormat());
}



RenderbufferTexture2D::RenderbufferTexture2D(Texture2D *texture, GLenum target) : mTarget(target)
{
    mTexture2D.set(texture);
}

RenderbufferTexture2D::~RenderbufferTexture2D()
{
    mTexture2D.set(NULL);
}



void RenderbufferTexture2D::addProxyRef(const Renderbuffer *proxy)
{
    mTexture2D->addProxyRef(proxy);
}

void RenderbufferTexture2D::releaseProxy(const Renderbuffer *proxy)
{
    mTexture2D->releaseProxy(proxy);
}



IDirect3DSurface9 *RenderbufferTexture2D::getRenderTarget()
{
    return mTexture2D->getRenderTarget(mTarget);
}



IDirect3DSurface9 *RenderbufferTexture2D::getDepthStencil()
{
    return mTexture2D->getDepthStencil(mTarget);
}

GLsizei RenderbufferTexture2D::getWidth() const
{
    return mTexture2D->getWidth(0);
}

GLsizei RenderbufferTexture2D::getHeight() const
{
    return mTexture2D->getHeight(0);
}

GLenum RenderbufferTexture2D::getInternalFormat() const
{
    return mTexture2D->getInternalFormat(0);
}

D3DFORMAT RenderbufferTexture2D::getD3DFormat() const
{
    return mTexture2D->getD3DFormat(0);
}

GLsizei RenderbufferTexture2D::getSamples() const
{
    return 0;
}

unsigned int RenderbufferTexture2D::getSerial() const
{
    return mTexture2D->getRenderTargetSerial(mTarget);
}



RenderbufferTextureCubeMap::RenderbufferTextureCubeMap(TextureCubeMap *texture, GLenum target) : mTarget(target)
{
    mTextureCubeMap.set(texture);
}

RenderbufferTextureCubeMap::~RenderbufferTextureCubeMap()
{
    mTextureCubeMap.set(NULL);
}



void RenderbufferTextureCubeMap::addProxyRef(const Renderbuffer *proxy)
{
    mTextureCubeMap->addProxyRef(proxy);
}

void RenderbufferTextureCubeMap::releaseProxy(const Renderbuffer *proxy)
{
    mTextureCubeMap->releaseProxy(proxy);
}



IDirect3DSurface9 *RenderbufferTextureCubeMap::getRenderTarget()
{
    return mTextureCubeMap->getRenderTarget(mTarget);
}



IDirect3DSurface9 *RenderbufferTextureCubeMap::getDepthStencil()
{
    return NULL;
}

GLsizei RenderbufferTextureCubeMap::getWidth() const
{
    return mTextureCubeMap->getWidth(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
}

GLsizei RenderbufferTextureCubeMap::getHeight() const
{
    return mTextureCubeMap->getHeight(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
}

GLenum RenderbufferTextureCubeMap::getInternalFormat() const
{
    return mTextureCubeMap->getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
}

D3DFORMAT RenderbufferTextureCubeMap::getD3DFormat() const
{
    return mTextureCubeMap->getD3DFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
}

GLsizei RenderbufferTextureCubeMap::getSamples() const
{
    return 0;
}

unsigned int RenderbufferTextureCubeMap::getSerial() const
{
    return mTextureCubeMap->getRenderTargetSerial(mTarget);
}



Renderbuffer::Renderbuffer(GLuint id, RenderbufferInterface *instance) : RefCountObject(id)
{
    ASSERT(instance != NULL);
    mInstance = instance;
}

Renderbuffer::~Renderbuffer()
{
    delete mInstance;
}



void Renderbuffer::addRef() const
{
    mInstance->addProxyRef(this);

    RefCountObject::addRef();
}

void Renderbuffer::release() const
{
    mInstance->releaseProxy(this);

    RefCountObject::release();
}



IDirect3DSurface9 *Renderbuffer::getRenderTarget()
{
    return mInstance->getRenderTarget();
}



IDirect3DSurface9 *Renderbuffer::getDepthStencil()
{
    return mInstance->getDepthStencil();
}

GLsizei Renderbuffer::getWidth() const
{
    return mInstance->getWidth();
}

GLsizei Renderbuffer::getHeight() const
{
    return mInstance->getHeight();
}

GLenum Renderbuffer::getInternalFormat() const
{
    return mInstance->getInternalFormat();
}

D3DFORMAT Renderbuffer::getD3DFormat() const
{
    return mInstance->getD3DFormat();
}

GLuint Renderbuffer::getRedSize() const
{
    return mInstance->getRedSize();
}

GLuint Renderbuffer::getGreenSize() const
{
    return mInstance->getGreenSize();
}

GLuint Renderbuffer::getBlueSize() const
{
    return mInstance->getBlueSize();
}

GLuint Renderbuffer::getAlphaSize() const
{
    return mInstance->getAlphaSize();
}

GLuint Renderbuffer::getDepthSize() const
{
    return mInstance->getDepthSize();
}

GLuint Renderbuffer::getStencilSize() const
{
    return mInstance->getStencilSize();
}

GLsizei Renderbuffer::getSamples() const
{
    return mInstance->getSamples();
}

unsigned int Renderbuffer::getSerial() const
{
    return mInstance->getSerial();
}

void Renderbuffer::setStorage(RenderbufferStorage *newStorage)
{
    ASSERT(newStorage != NULL);

    delete mInstance;
    mInstance = newStorage;
}

RenderbufferStorage::RenderbufferStorage() : mSerial(issueSerial())
{
    mWidth = 0;
    mHeight = 0;
    mInternalFormat = GL_RGBA4;
    mD3DFormat = D3DFMT_A8R8G8B8;
    mSamples = 0;
}

RenderbufferStorage::~RenderbufferStorage()
{
}



IDirect3DSurface9 *RenderbufferStorage::getRenderTarget()
{
    return NULL;
}



IDirect3DSurface9 *RenderbufferStorage::getDepthStencil()
{
    return NULL;
}

GLsizei RenderbufferStorage::getWidth() const
{
    return mWidth;
}

GLsizei RenderbufferStorage::getHeight() const
{
    return mHeight;
}

GLenum RenderbufferStorage::getInternalFormat() const
{
    return mInternalFormat;
}

D3DFORMAT RenderbufferStorage::getD3DFormat() const
{
    return mD3DFormat;
}

GLsizei RenderbufferStorage::getSamples() const
{
    return mSamples;
}

unsigned int RenderbufferStorage::getSerial() const
{
    return mSerial;
}

unsigned int RenderbufferStorage::issueSerial()
{
    return mCurrentSerial++;
}

unsigned int RenderbufferStorage::issueCubeSerials()
{
    unsigned int firstSerial = mCurrentSerial;
    mCurrentSerial += 6;
    return firstSerial;
}

Colorbuffer::Colorbuffer(IDirect3DSurface9 *renderTarget) : mRenderTarget(renderTarget)
{
    if (renderTarget)
    {
        renderTarget->AddRef();

        D3DSURFACE_DESC description;
        renderTarget->GetDesc(&description);

        mWidth = description.Width;
        mHeight = description.Height;
        mInternalFormat = dx2es::ConvertBackBufferFormat(description.Format);
        mD3DFormat = description.Format;
        mSamples = dx2es::GetSamplesFromMultisampleType(description.MultiSampleType);
    }
}

Colorbuffer::Colorbuffer(int width, int height, GLenum format, GLsizei samples) : mRenderTarget(NULL)
{
    IDirect3DDevice9 *device = getDevice();

    D3DFORMAT requestedFormat = es2dx::ConvertRenderbufferFormat(format);
    int supportedSamples = getContext()->getNearestSupportedSamples(requestedFormat, samples);

    if (supportedSamples == -1)
    {
        error(GL_OUT_OF_MEMORY);

        return;
    }

    if (width > 0 && height > 0)
    {
        HRESULT result = device->CreateRenderTarget(width, height, requestedFormat, 
                                                    es2dx::GetMultisampleTypeFromSamples(supportedSamples), 0, FALSE, &mRenderTarget, NULL);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            error(GL_OUT_OF_MEMORY);

            return;
        }

        ASSERT(SUCCEEDED(result));
    }

    mWidth = width;
    mHeight = height;
    mInternalFormat = format;
    mD3DFormat = requestedFormat;
    mSamples = supportedSamples;
}

Colorbuffer::~Colorbuffer()
{
    if (mRenderTarget)
    {
        mRenderTarget->Release();
    }
}



IDirect3DSurface9 *Colorbuffer::getRenderTarget()
{
    if (mRenderTarget)
    {
        mRenderTarget->AddRef();
    }

    return mRenderTarget;
}

DepthStencilbuffer::DepthStencilbuffer(IDirect3DSurface9 *depthStencil) : mDepthStencil(depthStencil)
{
    if (depthStencil)
    {
        depthStencil->AddRef();

        D3DSURFACE_DESC description;
        depthStencil->GetDesc(&description);

        mWidth = description.Width;
        mHeight = description.Height;
        mInternalFormat = dx2es::ConvertDepthStencilFormat(description.Format);
        mSamples = dx2es::GetSamplesFromMultisampleType(description.MultiSampleType); 
        mD3DFormat = description.Format;
    }
}

DepthStencilbuffer::DepthStencilbuffer(int width, int height, GLsizei samples)
{
    IDirect3DDevice9 *device = getDevice();

    mDepthStencil = NULL;
    
    int supportedSamples = getContext()->getNearestSupportedSamples(D3DFMT_D24S8, samples);

    if (supportedSamples == -1)
    {
        error(GL_OUT_OF_MEMORY);

        return;
    }

    if (width > 0 && height > 0)
    {
        HRESULT result = device->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, es2dx::GetMultisampleTypeFromSamples(supportedSamples),
                                                           0, FALSE, &mDepthStencil, 0);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            error(GL_OUT_OF_MEMORY);

            return;
        }

        ASSERT(SUCCEEDED(result));
    }

    mWidth = width;
    mHeight = height;
    mInternalFormat = GL_DEPTH24_STENCIL8_OES;
    mD3DFormat = D3DFMT_D24S8;
    mSamples = supportedSamples;
}

DepthStencilbuffer::~DepthStencilbuffer()
{
    if (mDepthStencil)
    {
        mDepthStencil->Release();
    }
}



IDirect3DSurface9 *DepthStencilbuffer::getDepthStencil()
{
    if (mDepthStencil)
    {
        mDepthStencil->AddRef();
    }

    return mDepthStencil;
}

Depthbuffer::Depthbuffer(IDirect3DSurface9 *depthStencil) : DepthStencilbuffer(depthStencil)
{
    if (depthStencil)
    {
        mInternalFormat = GL_DEPTH_COMPONENT16;   
                                                  
                                                  
    }
}

Depthbuffer::Depthbuffer(int width, int height, GLsizei samples) : DepthStencilbuffer(width, height, samples)
{
    if (mDepthStencil)
    {
        mInternalFormat = GL_DEPTH_COMPONENT16;   
                                                  
                                                  
    }
}

Depthbuffer::~Depthbuffer()
{
}

Stencilbuffer::Stencilbuffer(IDirect3DSurface9 *depthStencil) : DepthStencilbuffer(depthStencil)
{
    if (depthStencil)
    {
        mInternalFormat = GL_STENCIL_INDEX8;   
                                               
                                               
    }
}

Stencilbuffer::Stencilbuffer(int width, int height, GLsizei samples) : DepthStencilbuffer(width, height, samples)
{
    if (mDepthStencil)
    {
        mInternalFormat = GL_STENCIL_INDEX8;   
                                               
                                               
    }
}

Stencilbuffer::~Stencilbuffer()
{
}

}
