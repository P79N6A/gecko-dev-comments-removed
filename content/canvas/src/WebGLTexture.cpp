




#include "WebGLContext.h"
#include "WebGLTexture.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include <algorithm>
#include "nsContentUtils.h"

using namespace mozilla;

JSObject*
WebGLTexture::WrapObject(JSContext *cx, JSObject *scope) {
    return dom::WebGLTextureBinding::Wrap(cx, scope, this);
}

WebGLTexture::WebGLTexture(WebGLContext *context)
    : WebGLContextBoundObject(context)
    , mHasEverBeenBound(false)
    , mTarget(0)
    , mMinFilter(LOCAL_GL_NEAREST_MIPMAP_LINEAR)
    , mMagFilter(LOCAL_GL_LINEAR)
    , mWrapS(LOCAL_GL_REPEAT)
    , mWrapT(LOCAL_GL_REPEAT)
    , mFacesCount(0)
    , mMaxLevelWithCustomImages(0)
    , mHaveGeneratedMipmap(false)
    , mFakeBlackStatus(DoNotNeedFakeBlack)
{
    SetIsDOMBinding();
    mContext->MakeContextCurrent();
    mContext->gl->fGenTextures(1, &mGLName);
    mContext->mTextures.insertBack(this);
}

void
WebGLTexture::Delete() {
    mImageInfos.Clear();
    mContext->MakeContextCurrent();
    mContext->gl->fDeleteTextures(1, &mGLName);
    LinkedListElement<WebGLTexture>::removeFrom(mContext->mTextures);
}

int64_t
WebGLTexture::ImageInfo::MemoryUsage() const {
    if (!mIsDefined)
        return 0;
    int64_t texelSizeInBits = WebGLContext::GetBitsPerTexel(mFormat, mType);
    return int64_t(mWidth) * int64_t(mHeight) * texelSizeInBits / 8;
}

int64_t
WebGLTexture::MemoryUsage() const {
    if (IsDeleted())
        return 0;
    int64_t result = 0;
    for(size_t face = 0; face < mFacesCount; face++) {
        if (mHaveGeneratedMipmap) {
            
            
            
            result += ImageInfoAt(0, face).MemoryUsage() * 4 / 3;
        } else {
            for(size_t level = 0; level <= mMaxLevelWithCustomImages; level++)
                result += ImageInfoAt(level, face).MemoryUsage();
        }
    }
    return result;
}

bool
WebGLTexture::DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(size_t face) const {
    if (mHaveGeneratedMipmap)
        return true;

    ImageInfo expected = ImageInfoAt(0, face);

    
    
    for (size_t level = 0; level <= mMaxLevelWithCustomImages; ++level) {
        const ImageInfo& actual = ImageInfoAt(level, face);
        if (actual != expected)
            return false;
        expected.mWidth = std::max(1, expected.mWidth >> 1);
        expected.mHeight = std::max(1, expected.mHeight >> 1);

        
        
        if (actual.mWidth == 1 && actual.mHeight == 1)
            return true;
    }

    
    return false;
}

void
WebGLTexture::SetDontKnowIfNeedFakeBlack() {
    mFakeBlackStatus = DontKnowIfNeedFakeBlack;
    mContext->SetDontKnowIfNeedFakeBlack();
}

void
WebGLTexture::Bind(WebGLenum aTarget) {
    
    

    bool firstTimeThisTextureIsBound = !mHasEverBeenBound;

    if (!firstTimeThisTextureIsBound && aTarget != mTarget) {
        mContext->ErrorInvalidOperation("bindTexture: this texture has already been bound to a different target");
        
        
        return;
    }

    mTarget = aTarget;

    mContext->gl->fBindTexture(mTarget, mGLName);

    if (firstTimeThisTextureIsBound) {
        mFacesCount = (mTarget == LOCAL_GL_TEXTURE_2D) ? 1 : 6;
        EnsureMaxLevelWithCustomImagesAtLeast(0);
        SetDontKnowIfNeedFakeBlack();

        
        
        
        if (mTarget == LOCAL_GL_TEXTURE_CUBE_MAP && !mContext->gl->IsGLES2())
            mContext->gl->fTexParameteri(mTarget, LOCAL_GL_TEXTURE_WRAP_R, LOCAL_GL_CLAMP_TO_EDGE);
    }

    mHasEverBeenBound = true;
}

void
WebGLTexture::SetImageInfo(WebGLenum aTarget, WebGLint aLevel,
                  WebGLsizei aWidth, WebGLsizei aHeight,
                  WebGLenum aFormat, WebGLenum aType)
{
    if ( (aTarget == LOCAL_GL_TEXTURE_2D) != (mTarget == LOCAL_GL_TEXTURE_2D) )
        return;

    size_t face = FaceForTarget(aTarget);

    EnsureMaxLevelWithCustomImagesAtLeast(aLevel);

    ImageInfoAt(aLevel, face) = ImageInfo(aWidth, aHeight, aFormat, aType);

    if (aLevel > 0)
        SetCustomMipmap();

    SetDontKnowIfNeedFakeBlack();
}

void
WebGLTexture::SetGeneratedMipmap() {
    if (!mHaveGeneratedMipmap) {
        mHaveGeneratedMipmap = true;
        SetDontKnowIfNeedFakeBlack();
    }
}

void
WebGLTexture::SetCustomMipmap() {
    if (mHaveGeneratedMipmap) {
        
        

        
        
        ImageInfo imageInfo = ImageInfoAt(0, 0);
        NS_ASSERTION(imageInfo.IsPowerOfTwo(), "this texture is NPOT, so how could GenerateMipmap() ever accept it?");

        WebGLsizei size = std::max(imageInfo.mWidth, imageInfo.mHeight);

        
        size_t maxLevel = 0;
        for (WebGLsizei n = size; n > 1; n >>= 1)
            ++maxLevel;

        EnsureMaxLevelWithCustomImagesAtLeast(maxLevel);

        for (size_t level = 1; level <= maxLevel; ++level) {
            
            imageInfo.mWidth >>= 1;
            imageInfo.mHeight >>= 1;
            for(size_t face = 0; face < mFacesCount; ++face)
                ImageInfoAt(level, face) = imageInfo;
        }
    }
    mHaveGeneratedMipmap = false;
}

bool
WebGLTexture::AreAllLevel0ImageInfosEqual() const {
    for (size_t face = 1; face < mFacesCount; ++face) {
        if (ImageInfoAt(0, face) != ImageInfoAt(0, 0))
            return false;
    }
    return true;
}

bool
WebGLTexture::IsMipmapTexture2DComplete() const {
    if (mTarget != LOCAL_GL_TEXTURE_2D)
        return false;
    if (!ImageInfoAt(0, 0).IsPositive())
        return false;
    if (mHaveGeneratedMipmap)
        return true;
    return DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(0);
}

bool
WebGLTexture::IsCubeComplete() const {
    if (mTarget != LOCAL_GL_TEXTURE_CUBE_MAP)
        return false;
    const ImageInfo &first = ImageInfoAt(0, 0);
    if (!first.IsPositive() || !first.IsSquare())
        return false;
    return AreAllLevel0ImageInfosEqual();
}

bool
WebGLTexture::IsMipmapCubeComplete() const {
    if (!IsCubeComplete()) 
        return false;
    for (size_t face = 0; face < mFacesCount; ++face) {
        if (!DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(face))
            return false;
    }
    return true;
}

bool
WebGLTexture::NeedFakeBlack() {
    
    if (mFakeBlackStatus == DoNotNeedFakeBlack)
        return false;

    if (mFakeBlackStatus == DontKnowIfNeedFakeBlack) {
        
        

        for (size_t face = 0; face < mFacesCount; ++face) {
            if (!ImageInfoAt(0, face).mIsDefined) {
                
                
                
                
                mFakeBlackStatus = DoNeedFakeBlack;
                return true;
            }
        }

        const char *msg_rendering_as_black
            = "A texture is going to be rendered as if it were black, as per the OpenGL ES 2.0.24 spec section 3.8.2, "
              "because it";

        if (mTarget == LOCAL_GL_TEXTURE_2D)
        {
            if (DoesMinFilterRequireMipmap())
            {
                if (!IsMipmapTexture2DComplete()) {
                    mContext->GenerateWarning
                        ("%s is a 2D texture, with a minification filter requiring a mipmap, "
                         "and is not mipmap complete (as defined in section 3.7.10).", msg_rendering_as_black);
                    mFakeBlackStatus = DoNeedFakeBlack;
                } else if (!ImageInfoAt(0).IsPowerOfTwo()) {
                    mContext->GenerateWarning
                        ("%s is a 2D texture, with a minification filter requiring a mipmap, "
                         "and either its width or height is not a power of two.", msg_rendering_as_black);
                    mFakeBlackStatus = DoNeedFakeBlack;
                }
            }
            else 
            {
                if (!ImageInfoAt(0).IsPositive()) {
                    mContext->GenerateWarning
                        ("%s is a 2D texture and its width or height is equal to zero.",
                         msg_rendering_as_black);
                    mFakeBlackStatus = DoNeedFakeBlack;
                } else if (!AreBothWrapModesClampToEdge() && !ImageInfoAt(0).IsPowerOfTwo()) {
                    mContext->GenerateWarning
                        ("%s is a 2D texture, with a minification filter not requiring a mipmap, "
                         "with its width or height not a power of two, and with a wrap mode "
                         "different from CLAMP_TO_EDGE.", msg_rendering_as_black);
                    mFakeBlackStatus = DoNeedFakeBlack;
                }
            }
        }
        else 
        {
            bool areAllLevel0ImagesPOT = true;
            for (size_t face = 0; face < mFacesCount; ++face)
                areAllLevel0ImagesPOT &= ImageInfoAt(0, face).IsPowerOfTwo();

            if (DoesMinFilterRequireMipmap())
            {
                if (!IsMipmapCubeComplete()) {
                    mContext->GenerateWarning("%s is a cube map texture, with a minification filter requiring a mipmap, "
                               "and is not mipmap cube complete (as defined in section 3.7.10).",
                               msg_rendering_as_black);
                    mFakeBlackStatus = DoNeedFakeBlack;
                } else if (!areAllLevel0ImagesPOT) {
                    mContext->GenerateWarning("%s is a cube map texture, with a minification filter requiring a mipmap, "
                               "and either the width or the height of some level 0 image is not a power of two.",
                               msg_rendering_as_black);
                    mFakeBlackStatus = DoNeedFakeBlack;
                }
            }
            else 
            {
                if (!IsCubeComplete()) {
                    mContext->GenerateWarning("%s is a cube map texture, with a minification filter not requiring a mipmap, "
                               "and is not cube complete (as defined in section 3.7.10).",
                               msg_rendering_as_black);
                    mFakeBlackStatus = DoNeedFakeBlack;
                } else if (!AreBothWrapModesClampToEdge() && !areAllLevel0ImagesPOT) {
                    mContext->GenerateWarning("%s is a cube map texture, with a minification filter not requiring a mipmap, "
                               "with some level 0 image having width or height not a power of two, and with a wrap mode "
                               "different from CLAMP_TO_EDGE.", msg_rendering_as_black);
                    mFakeBlackStatus = DoNeedFakeBlack;
                }
            }
        }

        
        
        if (mFakeBlackStatus == DontKnowIfNeedFakeBlack)
            mFakeBlackStatus = DoNotNeedFakeBlack;
    }

    return mFakeBlackStatus == DoNeedFakeBlack;
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WebGLTexture)

NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLTexture)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLTexture)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLTexture)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END
