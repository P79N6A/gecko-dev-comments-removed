




#ifndef WEBGLTEXTURE_H_
#define WEBGLTEXTURE_H_

#include "WebGLBindableName.h"
#include "WebGLFramebufferAttachable.h"
#include "WebGLObjectModel.h"
#include "WebGLStrongTypes.h"

#include "nsWrapperCache.h"

#include "mozilla/CheckedInt.h"
#include "mozilla/LinkedList.h"
#include "mozilla/Assertions.h"
#include <algorithm>
#include "nsAlgorithm.h"

namespace mozilla {


inline bool is_pot_assuming_nonnegative(GLsizei x)
{
    return x && (x & (x-1)) == 0;
}



class WebGLTexture MOZ_FINAL
    : public nsWrapperCache
    , public WebGLBindableName<TexTarget>
    , public WebGLRefCountedObject<WebGLTexture>
    , public LinkedListElement<WebGLTexture>
    , public WebGLContextBoundObject
    , public WebGLFramebufferAttachable
{
public:
    explicit WebGLTexture(WebGLContext* aContext, GLuint tex);

    void Delete();

    WebGLContext *GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLTexture)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLTexture)

protected:
    ~WebGLTexture() {
        DeleteOnce();
    }

    friend class WebGLContext;
    friend class WebGLFramebuffer;

    
    

public:

    class ImageInfo
        : public WebGLRectangleObject
    {
    public:
        ImageInfo()
            : mEffectiveInternalFormat(LOCAL_GL_NONE)
            , mDepth(0)
            , mImageDataStatus(WebGLImageDataStatus::NoImageData)
        {}

        ImageInfo(GLsizei width,
                  GLsizei height,
                  GLsizei depth,
                  TexInternalFormat effectiveInternalFormat,
                  WebGLImageDataStatus status)
            : WebGLRectangleObject(width, height)
            , mEffectiveInternalFormat(effectiveInternalFormat)
            , mDepth(depth)
            , mImageDataStatus(status)
        {
            
            MOZ_ASSERT(status != WebGLImageDataStatus::NoImageData);
        }

        bool operator==(const ImageInfo& a) const {
            return mImageDataStatus == a.mImageDataStatus &&
                   mWidth == a.mWidth &&
                   mHeight == a.mHeight &&
                   mDepth == a.mDepth &&
                   mEffectiveInternalFormat == a.mEffectiveInternalFormat;
        }
        bool operator!=(const ImageInfo& a) const {
            return !(*this == a);
        }
        bool IsSquare() const {
            return mWidth == mHeight;
        }
        bool IsPositive() const {
            return mWidth > 0 && mHeight > 0 && mDepth > 0;
        }
        bool IsPowerOfTwo() const {
            return is_pot_assuming_nonnegative(mWidth) &&
                   is_pot_assuming_nonnegative(mHeight); 
        }
        bool HasUninitializedImageData() const {
            return mImageDataStatus == WebGLImageDataStatus::UninitializedImageData;
        }
        size_t MemoryUsage() const;

        TexInternalFormat EffectiveInternalFormat() const { return mEffectiveInternalFormat; }
        GLsizei Depth() const { return mDepth; }

    protected:
        




        TexInternalFormat mEffectiveInternalFormat;

        





        GLsizei mDepth;

        WebGLImageDataStatus mImageDataStatus;

        friend class WebGLTexture;
    };

private:
    static size_t FaceForTarget(TexImageTarget texImageTarget) {
        if (texImageTarget == LOCAL_GL_TEXTURE_2D ||
            texImageTarget == LOCAL_GL_TEXTURE_3D)
        {
            return 0;
        }
        return texImageTarget.get() - LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    }

    ImageInfo& ImageInfoAtFace(size_t face, GLint level) {
        MOZ_ASSERT(face < mFacesCount, "wrong face index, must be 0 for TEXTURE_2D or TEXTURE_3D, and at most 5 for cube maps");

        
        return mImageInfos.ElementAt(level * mFacesCount + face);
    }

    const ImageInfo& ImageInfoAtFace(size_t face, GLint level) const {
        return const_cast<const ImageInfo&>(
            const_cast<WebGLTexture*>(this)->ImageInfoAtFace(face, level)
        );
    }

public:
    ImageInfo& ImageInfoAt(TexImageTarget imageTarget, GLint level) {
        size_t face = FaceForTarget(imageTarget);
        return ImageInfoAtFace(face, level);
    }

    const ImageInfo& ImageInfoAt(TexImageTarget imageTarget, GLint level) const {
        return const_cast<WebGLTexture*>(this)->ImageInfoAt(imageTarget, level);
    }

    bool HasImageInfoAt(TexImageTarget imageTarget, GLint level) const {
        size_t face = FaceForTarget(imageTarget);
        CheckedUint32 checked_index = CheckedUint32(level) * mFacesCount + face;
        return checked_index.isValid() &&
               checked_index.value() < mImageInfos.Length() &&
               ImageInfoAt(imageTarget, level).mImageDataStatus != WebGLImageDataStatus::NoImageData;
    }

    ImageInfo& ImageInfoBase() {
        return ImageInfoAtFace(0, 0);
    }

    const ImageInfo& ImageInfoBase() const {
        return ImageInfoAtFace(0, 0);
    }

    size_t MemoryUsage() const;

    void SetImageDataStatus(TexImageTarget imageTarget, GLint level, WebGLImageDataStatus newStatus) {
        MOZ_ASSERT(HasImageInfoAt(imageTarget, level));
        ImageInfo& imageInfo = ImageInfoAt(imageTarget, level);
        
        MOZ_ASSERT(newStatus != WebGLImageDataStatus::NoImageData ||
                   imageInfo.mImageDataStatus == WebGLImageDataStatus::NoImageData);
        if (imageInfo.mImageDataStatus != newStatus) {
            SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
        }
        imageInfo.mImageDataStatus = newStatus;
    }

    void EnsureNoUninitializedImageData(TexImageTarget imageTarget, GLint level);

protected:

    TexMinFilter mMinFilter;
    TexMagFilter mMagFilter;
    TexWrap mWrapS, mWrapT;

    size_t mFacesCount, mMaxLevelWithCustomImages;
    nsTArray<ImageInfo> mImageInfos;

    bool mHaveGeneratedMipmap; 
    bool mImmutable; 

    size_t mBaseMipmapLevel; 
    size_t mMaxMipmapLevel;  

    WebGLTextureFakeBlackStatus mFakeBlackStatus;

    void EnsureMaxLevelWithCustomImagesAtLeast(size_t aMaxLevelWithCustomImages) {
        mMaxLevelWithCustomImages = std::max(mMaxLevelWithCustomImages, aMaxLevelWithCustomImages);
        mImageInfos.EnsureLengthAtLeast((mMaxLevelWithCustomImages + 1) * mFacesCount);
    }

    bool CheckFloatTextureFilterParams() const {
        
        return (mMagFilter == LOCAL_GL_NEAREST) &&
            (mMinFilter == LOCAL_GL_NEAREST || mMinFilter == LOCAL_GL_NEAREST_MIPMAP_NEAREST);
    }

    bool AreBothWrapModesClampToEdge() const {
        return mWrapS == LOCAL_GL_CLAMP_TO_EDGE && mWrapT == LOCAL_GL_CLAMP_TO_EDGE;
    }

    bool DoesMipmapHaveAllLevelsConsistentlyDefined(TexImageTarget texImageTarget) const;

public:

    void Bind(TexTarget aTexTarget);

    void SetImageInfo(TexImageTarget aTarget, GLint aLevel,
                      GLsizei aWidth, GLsizei aHeight, GLsizei aDepth,
                      TexInternalFormat aFormat, WebGLImageDataStatus aStatus);

    void SetMinFilter(TexMinFilter aMinFilter) {
        mMinFilter = aMinFilter;
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
    }
    void SetMagFilter(TexMagFilter aMagFilter) {
        mMagFilter = aMagFilter;
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
    }
    void SetWrapS(TexWrap aWrapS) {
        mWrapS = aWrapS;
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
    }
    void SetWrapT(TexWrap aWrapT) {
        mWrapT = aWrapT;
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
    }
    TexMinFilter MinFilter() const { return mMinFilter; }

    bool DoesMinFilterRequireMipmap() const {
        return !(mMinFilter == LOCAL_GL_NEAREST || mMinFilter == LOCAL_GL_LINEAR);
    }

    void SetGeneratedMipmap();

    void SetCustomMipmap();

    bool IsFirstImagePowerOfTwo() const {
        return ImageInfoBase().IsPowerOfTwo();
    }

    bool AreAllLevel0ImageInfosEqual() const;

    bool IsMipmapComplete() const;

    bool IsCubeComplete() const;

    bool IsMipmapCubeComplete() const;

    void SetFakeBlackStatus(WebGLTextureFakeBlackStatus x);

    bool IsImmutable() const { return mImmutable; }
    void SetImmutable() { mImmutable = true; }

    void SetBaseMipmapLevel(size_t level) { mBaseMipmapLevel = level; }
    void SetMaxMipmapLevel(size_t level) { mMaxMipmapLevel = level; }

    
    
    size_t EffectiveBaseMipmapLevel() const {
        if (IsImmutable())
            return std::min(mBaseMipmapLevel, mMaxLevelWithCustomImages);
        return mBaseMipmapLevel;
    }
    size_t EffectiveMaxMipmapLevel() const {
        if (IsImmutable())
            return mozilla::clamped(mMaxMipmapLevel, EffectiveBaseMipmapLevel(), mMaxLevelWithCustomImages);
        return std::min(mMaxMipmapLevel, mMaxLevelWithCustomImages);
    }
    bool IsMipmapRangeValid() const;

    size_t MaxLevelWithCustomImages() const { return mMaxLevelWithCustomImages; }

    
    
    WebGLTextureFakeBlackStatus ResolvedFakeBlackStatus();
};

inline TexImageTarget
TexImageTargetForTargetAndFace(TexTarget target, size_t face)
{
    switch (target.get()) {
        case LOCAL_GL_TEXTURE_2D:
        case LOCAL_GL_TEXTURE_3D:
            MOZ_ASSERT(face == 0);
            return target.get();
        case LOCAL_GL_TEXTURE_CUBE_MAP:
            MOZ_ASSERT(face < 6);
            return LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
        default:
            MOZ_CRASH();
    }
}

} 

#endif
