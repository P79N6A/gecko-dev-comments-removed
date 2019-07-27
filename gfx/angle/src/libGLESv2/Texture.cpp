









#include "libGLESv2/Texture.h"
#include "libGLESv2/main.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/Image.h"
#include "libGLESv2/renderer/RenderTarget.h"
#include "libGLESv2/renderer/d3d/TextureStorage.h"

#include "libEGL/Surface.h"

#include "common/mathutil.h"
#include "common/utilities.h"

namespace gl
{

bool IsMipmapFiltered(const gl::SamplerState &samplerState)
{
    switch (samplerState.minFilter)
    {
      case GL_NEAREST:
      case GL_LINEAR:
        return false;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
        return true;
      default: UNREACHABLE();
        return false;
    }
}

bool IsPointSampled(const gl::SamplerState &samplerState)
{
    return (samplerState.magFilter == GL_NEAREST && (samplerState.minFilter == GL_NEAREST || samplerState.minFilter == GL_NEAREST_MIPMAP_NEAREST));
}

Texture::Texture(rx::TextureImpl *impl, GLuint id, GLenum target)
    : RefCountObject(id),
      mTexture(impl),
      mUsage(GL_NONE),
      mImmutable(false),
      mTarget(target)
{
}

Texture::~Texture()
{
    SafeDelete(mTexture);
}

GLenum Texture::getTarget() const
{
    return mTarget;
}

void Texture::setUsage(GLenum usage)
{
    mUsage = usage;
    getImplementation()->setUsage(usage);
}

void Texture::getSamplerStateWithNativeOffset(SamplerState *sampler)
{
    *sampler = mSamplerState;

    
    rx::TextureStorageInterface *texture = getNativeTexture();
    int topLevel = texture ? texture->getTopLevel() : 0;
    sampler->baseLevel = topLevel + mSamplerState.baseLevel;
}

GLenum Texture::getUsage() const
{
    return mUsage;
}

GLint Texture::getBaseLevelWidth() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getWidth() : 0);
}

GLint Texture::getBaseLevelHeight() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getHeight() : 0);
}

GLint Texture::getBaseLevelDepth() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getDepth() : 0);
}




GLenum Texture::getBaseLevelInternalFormat() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getInternalFormat() : GL_NONE);
}

rx::TextureStorageInterface *Texture::getNativeTexture()
{
    return getImplementation()->getNativeTexture();
}

void Texture::generateMipmaps()
{
    getImplementation()->generateMipmaps();
}

void Texture::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    getImplementation()->copySubImage(target, level, xoffset, yoffset, zoffset, x, y, width, height, source);
}

unsigned int Texture::getTextureSerial()
{
    rx::TextureStorageInterface *texture = getNativeTexture();
    return texture ? texture->getTextureSerial() : 0;
}

bool Texture::isImmutable() const
{
    return mImmutable;
}

int Texture::immutableLevelCount()
{
    return (mImmutable ? getNativeTexture()->getStorageInstance()->getLevelCount() : 0);
}

int Texture::mipLevels() const
{
    return log2(std::max(std::max(getBaseLevelWidth(), getBaseLevelHeight()), getBaseLevelDepth())) + 1;
}

const rx::Image *Texture::getBaseLevelImage() const
{
    return (getImplementation()->getLayerCount(0) > 0 ? getImplementation()->getImage(0, 0) : NULL);
}

Texture2D::Texture2D(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_2D)
{
    mSurface = NULL;
}

Texture2D::~Texture2D()
{
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;
    }
}

GLsizei Texture2D::getWidth(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getWidth();
    else
        return 0;
}

GLsizei Texture2D::getHeight(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getHeight();
    else
        return 0;
}

GLenum Texture2D::getInternalFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getInternalFormat();
    else
        return GL_NONE;
}

GLenum Texture2D::getActualFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getActualFormat();
    else
        return GL_NONE;
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    releaseTexImage();

    mTexture->setImage(GL_TEXTURE_2D, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void Texture2D::bindTexImage(egl::Surface *surface)
{
    releaseTexImage();

    mTexture->bindTexImage(surface);

    mSurface = surface;
    mSurface->setBoundTexture(this);
}

void Texture2D::releaseTexImage()
{
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;

        mTexture->releaseTexImage();
    }
}

void Texture2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    releaseTexImage();

    mTexture->setCompressedImage(GL_TEXTURE_2D, level, format, width, height, 1, imageSize, pixels);
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(GL_TEXTURE_2D, level, xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels);
}

void Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(GL_TEXTURE_2D, level, xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels);
}

void Texture2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    releaseTexImage();

    mTexture->copyImage(GL_TEXTURE_2D, level, format, x, y, width, height, source);
}

void Texture2D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    mImmutable = true;

    mTexture->storage(GL_TEXTURE_2D, levels, internalformat, width, height, 1);
}


bool Texture2D::isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    if (!textureCaps.get(getInternalFormat(0)).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    bool npotSupport = extensions.textureNPOT;

    if (!npotSupport)
    {
        if ((samplerState.wrapS != GL_CLAMP_TO_EDGE && !gl::isPow2(width)) ||
            (samplerState.wrapT != GL_CLAMP_TO_EDGE && !gl::isPow2(height)))
        {
            return false;
        }
    }

    if (IsMipmapFiltered(samplerState))
    {
        if (!npotSupport)
        {
            if (!gl::isPow2(width) || !gl::isPow2(height))
            {
                return false;
            }
        }

        if (!isMipmapComplete())
        {
            return false;
        }
    }

    
    
    
    
    
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(getInternalFormat(0));
    if (formatInfo.depthBits > 0 && clientVersion > 2)
    {
        if (samplerState.compareMode == GL_NONE)
        {
            if ((samplerState.minFilter != GL_NEAREST && samplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST) ||
                samplerState.magFilter != GL_NEAREST)
            {
                return false;
            }
        }
    }

    return true;
}

bool Texture2D::isCompressed(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).compressed;
}

bool Texture2D::isDepth(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).depthBits > 0;
}

void Texture2D::generateMipmaps()
{
    releaseTexImage();

    mTexture->generateMipmaps();
}

unsigned int Texture2D::getRenderTargetSerial(GLint level)
{
    return mTexture->getRenderTargetSerial(level, 0);
}

rx::RenderTarget *Texture2D::getRenderTarget(GLint level)
{
    return mTexture->getRenderTarget(level, 0);
}


bool Texture2D::isMipmapComplete() const
{
    int levelCount = mipLevels();

    for (int level = 0; level < levelCount; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2D::isLevelComplete(int level) const
{
    if (isImmutable())
    {
        return true;
    }

    const rx::Image *baseImage = getBaseLevelImage();

    GLsizei width = baseImage->getWidth();
    GLsizei height = baseImage->getHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    
    if (level == 0)
    {
        return true;
    }

    ASSERT(level >= 1 && level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getImage(level, 0) != NULL);
    rx::Image *image = mTexture->getImage(level, 0);

    if (image->getInternalFormat() != baseImage->getInternalFormat())
    {
        return false;
    }

    if (image->getWidth() != std::max(1, width >> level))
    {
        return false;
    }

    if (image->getHeight() != std::max(1, height >> level))
    {
        return false;
    }

    return true;
}

TextureCubeMap::TextureCubeMap(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_CUBE_MAP)
{
}

TextureCubeMap::~TextureCubeMap()
{
}

GLsizei TextureCubeMap::getWidth(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getWidth();
    else
        return 0;
}

GLsizei TextureCubeMap::getHeight(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getHeight();
    else
        return 0;
}

GLenum TextureCubeMap::getInternalFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getInternalFormat();
    else
        return GL_NONE;
}

GLenum TextureCubeMap::getActualFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getActualFormat();
    else
        return GL_NONE;
}

void TextureCubeMap::setImagePosX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    mTexture->setCompressedImage(target, level, format, width, height, 1, imageSize, pixels);
}

void TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(target, level, xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels);
}

void TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(target, level, xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels);
}


bool TextureCubeMap::isCubeComplete() const
{
    int    baseWidth  = getBaseLevelWidth();
    int    baseHeight = getBaseLevelHeight();
    GLenum baseFormat = getBaseLevelInternalFormat();

    if (baseWidth <= 0 || baseWidth != baseHeight)
    {
        return false;
    }

    for (int faceIndex = 1; faceIndex < 6; faceIndex++)
    {
        const rx::Image *faceBaseImage = mTexture->getImage(0, faceIndex);

        if (faceBaseImage->getWidth()          != baseWidth  ||
            faceBaseImage->getHeight()         != baseHeight ||
            faceBaseImage->getInternalFormat() != baseFormat )
        {
            return false;
        }
    }

    return true;
}

bool TextureCubeMap::isCompressed(GLenum target, GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(target, level)).compressed;
}

bool TextureCubeMap::isDepth(GLenum target, GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(target, level)).depthBits > 0;
}

void TextureCubeMap::copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    mTexture->copyImage(target, level, format, x, y, width, height, source);
}

void TextureCubeMap::storage(GLsizei levels, GLenum internalformat, GLsizei size)
{
    mImmutable = true;

    mTexture->storage(GL_TEXTURE_CUBE_MAP, levels, internalformat, size, size, 1);
}


bool TextureCubeMap::isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const
{
    int size = getBaseLevelWidth();

    bool mipmapping = IsMipmapFiltered(samplerState);

    if (!textureCaps.get(getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0)).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    if (!gl::isPow2(size) && !extensions.textureNPOT)
    {
        if (samplerState.wrapS != GL_CLAMP_TO_EDGE || samplerState.wrapT != GL_CLAMP_TO_EDGE || mipmapping)
        {
            return false;
        }
    }

    if (!mipmapping)
    {
        if (!isCubeComplete())
        {
            return false;
        }
    }
    else
    {
        if (!isMipmapComplete())   
        {
            return false;
        }
    }

    return true;
}

unsigned int TextureCubeMap::getRenderTargetSerial(GLenum target, GLint level)
{
    return mTexture->getRenderTargetSerial(level, targetToLayerIndex(target));
}

int TextureCubeMap::targetToLayerIndex(GLenum target)
{
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_X - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 1);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 2);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 3);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 4);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

    return target - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
}

GLenum TextureCubeMap::layerIndexToTarget(GLint layer)
{
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_X - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 1);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 2);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 3);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 4);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

    return GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer;
}

rx::RenderTarget *TextureCubeMap::getRenderTarget(GLenum target, GLint level)
{
    return mTexture->getRenderTarget(level, targetToLayerIndex(target));
}

bool TextureCubeMap::isMipmapComplete() const
{
    if (isImmutable())
    {
        return true;
    }

    if (!isCubeComplete())
    {
        return false;
    }

    int levelCount = mipLevels();

    for (int face = 0; face < 6; face++)
    {
        for (int level = 1; level < levelCount; level++)
        {
            if (!isFaceLevelComplete(face, level))
            {
                return false;
            }
        }
    }

    return true;
}

bool TextureCubeMap::isFaceLevelComplete(int faceIndex, int level) const
{
    ASSERT(level >= 0 && faceIndex < 6 && level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getImage(level, faceIndex) != NULL);

    if (isImmutable())
    {
        return true;
    }

    int baseSize = getBaseLevelWidth();

    if (baseSize <= 0)
    {
        return false;
    }

    
    
    
    if (level == 0)
    {
        return true;
    }

    
    const rx::Image *faceLevelImage = mTexture->getImage(level, faceIndex);

    if (faceLevelImage->getInternalFormat() != getBaseLevelInternalFormat())
    {
        return false;
    }

    if (faceLevelImage->getWidth() != std::max(1, baseSize >> level))
    {
        return false;
    }

    return true;
}


Texture3D::Texture3D(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_3D)
{
}

Texture3D::~Texture3D()
{
}

GLsizei Texture3D::getWidth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getWidth() : 0;
}

GLsizei Texture3D::getHeight(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getHeight() : 0;
}

GLsizei Texture3D::getDepth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getDepth() : 0;
}

GLenum Texture3D::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getInternalFormat() : GL_NONE;
}

GLenum Texture3D::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getActualFormat() : GL_NONE;
}

bool Texture3D::isCompressed(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).compressed;
}

bool Texture3D::isDepth(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).depthBits > 0;
}

void Texture3D::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_3D, level, width, height, depth, internalFormat, format, type, unpack, pixels);
}

void Texture3D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    mTexture->setCompressedImage(GL_TEXTURE_3D, level, format, width, height, depth, imageSize, pixels);
}

void Texture3D::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels);
}

void Texture3D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, pixels);
}

void Texture3D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    mImmutable = true;

    mTexture->storage(GL_TEXTURE_3D, levels, internalformat, width, height, depth);
}

bool Texture3D::isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (!textureCaps.get(getInternalFormat(0)).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    if (IsMipmapFiltered(samplerState) && !isMipmapComplete())
    {
        return false;
    }

    return true;
}

unsigned int Texture3D::getRenderTargetSerial(GLint level, GLint layer)
{
    return mTexture->getRenderTargetSerial(level, layer);
}


rx::RenderTarget *Texture3D::getRenderTarget(GLint level, GLint layer)
{
    return mTexture->getRenderTarget(level, layer);
}

bool Texture3D::isMipmapComplete() const
{
    int levelCount = mipLevels();

    for (int level = 0; level < levelCount; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture3D::isLevelComplete(int level) const
{
    ASSERT(level >= 0 && level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getImage(level, 0) != NULL);

    if (isImmutable())
    {
        return true;
    }

    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (level == 0)
    {
        return true;
    }

    rx::Image *levelImage = mTexture->getImage(level, 0);

    if (levelImage->getInternalFormat() != getBaseLevelInternalFormat())
    {
        return false;
    }

    if (levelImage->getWidth() != std::max(1, width >> level))
    {
        return false;
    }

    if (levelImage->getHeight() != std::max(1, height >> level))
    {
        return false;
    }

    if (levelImage->getDepth() != std::max(1, depth >> level))
    {
        return false;
    }

    return true;
}

Texture2DArray::Texture2DArray(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_2D_ARRAY)
{
}

Texture2DArray::~Texture2DArray()
{
}

GLsizei Texture2DArray::getWidth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getWidth() : 0;
}

GLsizei Texture2DArray::getHeight(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getHeight() : 0;
}

GLsizei Texture2DArray::getLayers(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getLayerCount(level) : 0;
}

GLenum Texture2DArray::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getInternalFormat() : GL_NONE;
}

GLenum Texture2DArray::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getActualFormat() : GL_NONE;
}

bool Texture2DArray::isCompressed(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).compressed;
}

bool Texture2DArray::isDepth(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).depthBits > 0;
}

void Texture2DArray::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_2D_ARRAY, level, width, height, depth, internalFormat, format, type, unpack, pixels);
}

void Texture2DArray::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    mTexture->setCompressedImage(GL_TEXTURE_2D_ARRAY, level, format, width, height, depth, imageSize, pixels);
}

void Texture2DArray::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels);
}

void Texture2DArray::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, pixels);
}

void Texture2DArray::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    mImmutable = true;

    mTexture->storage(GL_TEXTURE_2D_ARRAY, levels, internalformat, width, height, depth);
}

bool Texture2DArray::isSamplerComplete(const SamplerState &samplerState, const TextureCapsMap &textureCaps, const Extensions &extensions, int clientVersion) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getLayers(0);

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (!textureCaps.get(getBaseLevelInternalFormat()).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    if (IsMipmapFiltered(samplerState) && !isMipmapComplete())
    {
        return false;
    }

    return true;
}

unsigned int Texture2DArray::getRenderTargetSerial(GLint level, GLint layer)
{
    return mTexture->getRenderTargetSerial(level, layer);
}

rx::RenderTarget *Texture2DArray::getRenderTarget(GLint level, GLint layer)
{
    return mTexture->getRenderTarget(level, layer);
}

bool Texture2DArray::isMipmapComplete() const
{
    int levelCount = mipLevels();

    for (int level = 1; level < levelCount; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2DArray::isLevelComplete(int level) const
{
    ASSERT(level >= 0 && level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (isImmutable())
    {
        return true;
    }

    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei layers = getLayers(0);

    if (width <= 0 || height <= 0 || layers <= 0)
    {
        return false;
    }

    if (level == 0)
    {
        return true;
    }

    if (getInternalFormat(level) != getInternalFormat(0))
    {
        return false;
    }

    if (getWidth(level) != std::max(1, width >> level))
    {
        return false;
    }

    if (getHeight(level) != std::max(1, height >> level))
    {
        return false;
    }

    if (getLayers(level) != layers)
    {
        return false;
    }

    return true;
}

}
