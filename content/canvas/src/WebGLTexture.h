




#ifndef WEBGLTEXTURE_H_
#define WEBGLTEXTURE_H_

#include "WebGLObjectModel.h"

#include "nsWrapperCache.h"

#include "mozilla/LinkedList.h"
#include <algorithm>

namespace mozilla {


inline bool is_pot_assuming_nonnegative(GLsizei x)
{
    return x && (x & (x-1)) == 0;
}



class WebGLTexture MOZ_FINAL
    : public nsWrapperCache
    , public WebGLRefCountedObject<WebGLTexture>
    , public LinkedListElement<WebGLTexture>
    , public WebGLContextBoundObject
{
public:
    WebGLTexture(WebGLContext *context);

    ~WebGLTexture() {
        DeleteOnce();
    }

    void Delete();

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    GLuint GLName() { return mGLName; }
    GLenum Target() const { return mTarget; }

    WebGLContext *GetParentObject() const {
        return Context();
    }

    virtual JSObject* WrapObject(JSContext *cx,
                                 JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLTexture)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLTexture)

protected:

    friend class WebGLContext;
    friend class WebGLFramebuffer;

    bool mHasEverBeenBound;
    GLuint mGLName;

    
    

public:

    class ImageInfo : public WebGLRectangleObject {
    public:
        ImageInfo()
            : mFormat(0)
            , mType(0)
            , mIsDefined(false)
        {}

        ImageInfo(GLsizei width, GLsizei height,
                  GLenum format, GLenum type)
            : WebGLRectangleObject(width, height)
            , mFormat(format)
            , mType(type)
            , mIsDefined(true)
        {}

        bool operator==(const ImageInfo& a) const {
            return mIsDefined == a.mIsDefined &&
                   mWidth     == a.mWidth &&
                   mHeight    == a.mHeight &&
                   mFormat    == a.mFormat &&
                   mType      == a.mType;
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
        int64_t MemoryUsage() const;
        GLenum Format() const { return mFormat; }
        GLenum Type() const { return mType; }
    protected:
        GLenum mFormat, mType;
        bool mIsDefined;

        friend class WebGLTexture;
    };

    ImageInfo& ImageInfoAt(size_t level, size_t face = 0) {
#ifdef DEBUG
        if (face >= mFacesCount)
            NS_ERROR("wrong face index, must be 0 for TEXTURE_2D and at most 5 for cube maps");
#endif
        
        return mImageInfos.ElementAt(level * mFacesCount + face);
    }

    const ImageInfo& ImageInfoAt(size_t level, size_t face) const {
        return const_cast<WebGLTexture*>(this)->ImageInfoAt(level, face);
    }

    bool HasImageInfoAt(size_t level, size_t face) const {
        CheckedUint32 checked_index = CheckedUint32(level) * mFacesCount + face;
        return checked_index.isValid() &&
               checked_index.value() < mImageInfos.Length() &&
               ImageInfoAt(level, face).mIsDefined;
    }

    static size_t FaceForTarget(GLenum target) {
        return target == LOCAL_GL_TEXTURE_2D ? 0 : target - LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    }

    int64_t MemoryUsage() const;

protected:

    GLenum mTarget;
    GLenum mMinFilter, mMagFilter, mWrapS, mWrapT;

    size_t mFacesCount, mMaxLevelWithCustomImages;
    nsTArray<ImageInfo> mImageInfos;

    bool mHaveGeneratedMipmap;
    FakeBlackStatus mFakeBlackStatus;

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

    bool DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(size_t face) const;

public:

    void SetDontKnowIfNeedFakeBlack();

    void Bind(GLenum aTarget);

    void SetImageInfo(GLenum aTarget, GLint aLevel,
                      GLsizei aWidth, GLsizei aHeight,
                      GLenum aFormat, GLenum aType);

    void SetMinFilter(GLenum aMinFilter) {
        mMinFilter = aMinFilter;
        SetDontKnowIfNeedFakeBlack();
    }
    void SetMagFilter(GLenum aMagFilter) {
        mMagFilter = aMagFilter;
        SetDontKnowIfNeedFakeBlack();
    }
    void SetWrapS(GLenum aWrapS) {
        mWrapS = aWrapS;
        SetDontKnowIfNeedFakeBlack();
    }
    void SetWrapT(GLenum aWrapT) {
        mWrapT = aWrapT;
        SetDontKnowIfNeedFakeBlack();
    }
    GLenum MinFilter() const { return mMinFilter; }

    bool DoesMinFilterRequireMipmap() const {
        return !(mMinFilter == LOCAL_GL_NEAREST || mMinFilter == LOCAL_GL_LINEAR);
    }

    void SetGeneratedMipmap();

    void SetCustomMipmap();

    bool IsFirstImagePowerOfTwo() const {
        return ImageInfoAt(0, 0).IsPowerOfTwo();
    }

    bool AreAllLevel0ImageInfosEqual() const;

    bool IsMipmapTexture2DComplete() const;

    bool IsCubeComplete() const;

    bool IsMipmapCubeComplete() const;

    bool NeedFakeBlack();
};

} 

#endif
