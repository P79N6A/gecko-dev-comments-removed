




#ifndef WEBGLTEXTURE_H_
#define WEBGLTEXTURE_H_

#include "WebGLBindableName.h"
#include "WebGLFramebufferAttachable.h"
#include "WebGLObjectModel.h"
#include "WebGLStrongTypes.h"

#include "nsWrapperCache.h"

#include "mozilla/CheckedInt.h"
#include "mozilla/LinkedList.h"
#include <algorithm>

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
    explicit WebGLTexture(WebGLContext* aContext);

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
            : mWebGLFormat(LOCAL_GL_NONE)
            , mWebGLType(LOCAL_GL_NONE)
            , mImageDataStatus(WebGLImageDataStatus::NoImageData)
        {}

        ImageInfo(GLsizei width,
                  GLsizei height,
                  TexInternalFormat webGLFormat,
                  TexType webGLType,
                  WebGLImageDataStatus status)
            : WebGLRectangleObject(width, height)
            , mWebGLFormat(webGLFormat)
            , mWebGLType(webGLType)
            , mImageDataStatus(status)
        {
            
            MOZ_ASSERT(status != WebGLImageDataStatus::NoImageData);
        }

        bool operator==(const ImageInfo& a) const {
            return mImageDataStatus == a.mImageDataStatus &&
                   mWidth == a.mWidth &&
                   mHeight == a.mHeight &&
                   mWebGLFormat == a.mWebGLFormat &&
                   mWebGLType == a.mWebGLType;
        }
        bool operator!=(const ImageInfo& a) const {
            return !(*this == a);
        }
        bool IsSquare() const {
            return mWidth == mHeight;
        }
        bool IsPositive() const {
            return mWidth > 0 && mHeight > 0;
        }
        bool IsPowerOfTwo() const {
            return is_pot_assuming_nonnegative(mWidth) &&
                   is_pot_assuming_nonnegative(mHeight); 
        }
        bool HasUninitializedImageData() const {
            return mImageDataStatus == WebGLImageDataStatus::UninitializedImageData;
        }
        int64_t MemoryUsage() const;
        



        TexInternalFormat WebGLFormat() const { return mWebGLFormat; }
        



        TexType WebGLType() const { return mWebGLType; }

    protected:
        TexInternalFormat mWebGLFormat; 
        TexType mWebGLType;   
        WebGLImageDataStatus mImageDataStatus;

        friend class WebGLTexture;
    };

private:
    static size_t FaceForTarget(TexImageTarget texImageTarget) {
        if (texImageTarget == LOCAL_GL_TEXTURE_2D)
            return 0;

        return texImageTarget.get() - LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    }

    ImageInfo& ImageInfoAtFace(size_t face, GLint level) {
        MOZ_ASSERT(face < mFacesCount, "wrong face index, must be 0 for TEXTURE_2D and at most 5 for cube maps");

        
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

    int64_t MemoryUsage() const;

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

    void DoDeferredImageInitialization(TexImageTarget imageTarget, GLint level);

protected:

    GLenum mMinFilter, mMagFilter, mWrapS, mWrapT;

    size_t mFacesCount, mMaxLevelWithCustomImages;
    nsTArray<ImageInfo> mImageInfos;

    bool mHaveGeneratedMipmap;
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

    bool DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(TexImageTarget texImageTarget) const;

public:

    void Bind(TexTarget aTexTarget);

    void SetImageInfo(TexImageTarget aTarget, GLint aLevel,
                      GLsizei aWidth, GLsizei aHeight,
                      TexInternalFormat aFormat, TexType aType, WebGLImageDataStatus aStatus);

    void SetMinFilter(GLenum aMinFilter) {
        mMinFilter = aMinFilter;
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
    }
    void SetMagFilter(GLenum aMagFilter) {
        mMagFilter = aMagFilter;
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
    }
    void SetWrapS(GLenum aWrapS) {
        mWrapS = aWrapS;
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
    }
    void SetWrapT(GLenum aWrapT) {
        mWrapT = aWrapT;
        SetFakeBlackStatus(WebGLTextureFakeBlackStatus::Unknown);
    }
    GLenum MinFilter() const { return mMinFilter; }

    bool DoesMinFilterRequireMipmap() const {
        return !(mMinFilter == LOCAL_GL_NEAREST || mMinFilter == LOCAL_GL_LINEAR);
    }

    void SetGeneratedMipmap();

    void SetCustomMipmap();

    bool IsFirstImagePowerOfTwo() const {
        return ImageInfoBase().IsPowerOfTwo();
    }

    bool AreAllLevel0ImageInfosEqual() const;

    bool IsMipmapTexture2DComplete() const;

    bool IsCubeComplete() const;

    bool IsMipmapCubeComplete() const;

    void SetFakeBlackStatus(WebGLTextureFakeBlackStatus x);

    
    
    WebGLTextureFakeBlackStatus ResolvedFakeBlackStatus();
};

} 

#endif
