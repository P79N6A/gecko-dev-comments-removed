




#ifndef GLTEXTUREIMAGE_H_
#define GLTEXTUREIMAGE_H_

#include "nsAutoPtr.h"
#include "nsRegion.h"
#include "nsTArray.h"
#include "gfxTypes.h"
#include "GLContextTypes.h"
#include "GraphicsFilter.h"
#include "mozilla/gfx/Rect.h"
#include "mozilla/RefPtr.h"

class gfxASurface;

namespace mozilla {
namespace gfx {
class DataSourceSurface;
class DrawTarget;
}
}

namespace mozilla {
namespace gl {
class GLContext;

















class TextureImage
{
    NS_INLINE_DECL_REFCOUNTING(TextureImage)
public:
    enum TextureState
    {
      Created, 
      Allocated,  
      Valid  
    };

    enum Flags {
        NoFlags          = 0x0,
        UseNearestFilter = 0x1,
        OriginBottomLeft = 0x2,
        DisallowBigImage = 0x4
    };

    typedef gfxContentType ContentType;
    typedef gfxImageFormat ImageFormat;

    static already_AddRefed<TextureImage> Create(
                       GLContext* gl,
                       const gfx::IntSize& aSize,
                       TextureImage::ContentType aContentType,
                       GLenum aWrapMode,
                       TextureImage::Flags aFlags = TextureImage::NoFlags);

    




















    virtual gfx::DrawTarget* BeginUpdate(nsIntRegion& aRegion) = 0;
    






    virtual void GetUpdateRegion(nsIntRegion& aForRegion) {
    }
    






    virtual void EndUpdate() = 0;

    



    virtual void BeginBigImageIteration() {
    }

    virtual bool NextTile() {
        return false;
    }

    
    
    
    typedef bool (* BigImageIterationCallback)(TextureImage* aImage,
                                           int aTileNumber,
                                           void* aCallbackData);

    
    virtual void SetIterationCallback(BigImageIterationCallback aCallback,
                                      void* aCallbackData) {
    }

    virtual gfx::IntRect GetTileRect();

    virtual GLuint GetTextureID() = 0;

    virtual uint32_t GetTileCount() {
        return 1;
    }

    







    virtual void Resize(const gfx::IntSize& aSize) {
        mSize = aSize;
        nsIntRegion r(nsIntRect(0, 0, aSize.width, aSize.height));
        BeginUpdate(r);
        EndUpdate();
    }

    



    virtual void MarkValid() {}

    




    virtual bool DirectUpdate(gfx::DataSourceSurface* aSurf, const nsIntRegion& aRegion, const gfx::IntPoint& aFrom = gfx::IntPoint(0,0)) = 0;
    bool UpdateFromDataSource(gfx::DataSourceSurface *aSurf,
                              const nsIntRegion* aDstRegion = nullptr,
                              const gfx::IntPoint* aSrcOffset = nullptr);

    virtual void BindTexture(GLenum aTextureUnit) = 0;

    



    virtual gfx::SurfaceFormat GetTextureFormat() {
        return mTextureFormat;
    }

    

    



    virtual already_AddRefed<gfxASurface> GetBackingSurface()
    { return nullptr; }


    gfx::IntSize GetSize() const;
    ContentType GetContentType() const { return mContentType; }
    ImageFormat GetImageFormat() const { return mImageFormat; }
    virtual bool InUpdate() const = 0;
    GLenum GetWrapMode() const { return mWrapMode; }

    void SetFilter(GraphicsFilter aFilter) { mFilter = aFilter; }

protected:
    friend class GLContext;

    





    TextureImage(const gfx::IntSize& aSize,
                 GLenum aWrapMode, ContentType aContentType,
                 Flags aFlags = NoFlags,
                 ImageFormat aImageFormat = gfxImageFormat::Unknown);

    
    virtual ~TextureImage() {}

    virtual gfx::IntRect GetSrcTileRect();

    gfx::IntSize mSize;
    GLenum mWrapMode;
    ContentType mContentType;
    ImageFormat mImageFormat;
    gfx::SurfaceFormat mTextureFormat;
    GraphicsFilter mFilter;
    Flags mFlags;
};










class BasicTextureImage
    : public TextureImage
{
public:
    virtual ~BasicTextureImage();

    BasicTextureImage(GLuint aTexture,
                      const gfx::IntSize& aSize,
                      GLenum aWrapMode,
                      ContentType aContentType,
                      GLContext* aContext,
                      TextureImage::Flags aFlags = TextureImage::NoFlags,
                      TextureImage::ImageFormat aImageFormat = gfxImageFormat::Unknown);

    virtual void BindTexture(GLenum aTextureUnit);

    virtual gfx::DrawTarget* BeginUpdate(nsIntRegion& aRegion);
    virtual void GetUpdateRegion(nsIntRegion& aForRegion);
    virtual void EndUpdate();
    virtual bool DirectUpdate(gfx::DataSourceSurface* aSurf, const nsIntRegion& aRegion, const gfx::IntPoint& aFrom = gfx::IntPoint(0,0));
    virtual GLuint GetTextureID() { return mTexture; }
    virtual TemporaryRef<gfx::DrawTarget>
      GetDrawTargetForUpdate(const gfx::IntSize& aSize, gfx::SurfaceFormat aFmt);

    virtual void MarkValid() { mTextureState = Valid; }

    
    
    
    virtual bool FinishedSurfaceUpdate();

    
    virtual void FinishedSurfaceUpload();

    virtual bool InUpdate() const { return !!mUpdateDrawTarget; }

    virtual void Resize(const gfx::IntSize& aSize);

protected:
    GLuint mTexture;
    TextureState mTextureState;
    nsRefPtr<GLContext> mGLContext;
    RefPtr<gfx::DrawTarget> mUpdateDrawTarget;
    nsIntRegion mUpdateRegion;

    
    nsIntPoint mUpdateOffset;
};






class TiledTextureImage final
    : public TextureImage
{
public:
    TiledTextureImage(GLContext* aGL,
                      gfx::IntSize aSize,
                      TextureImage::ContentType,
                      TextureImage::Flags aFlags = TextureImage::NoFlags,
                      TextureImage::ImageFormat aImageFormat = gfxImageFormat::Unknown);
    ~TiledTextureImage();
    void DumpDiv();
    virtual gfx::DrawTarget* BeginUpdate(nsIntRegion& aRegion);
    virtual void GetUpdateRegion(nsIntRegion& aForRegion);
    virtual void EndUpdate();
    virtual void Resize(const gfx::IntSize& aSize);
    virtual uint32_t GetTileCount();
    virtual void BeginBigImageIteration();
    virtual bool NextTile();
    virtual void SetIterationCallback(BigImageIterationCallback aCallback,
                                      void* aCallbackData);
    virtual gfx::IntRect GetTileRect();
    virtual GLuint GetTextureID() {
        return mImages[mCurrentImage]->GetTextureID();
    }
    virtual bool DirectUpdate(gfx::DataSourceSurface* aSurf, const nsIntRegion& aRegion, const gfx::IntPoint& aFrom = gfx::IntPoint(0,0));
    virtual bool InUpdate() const { return mInUpdate; }
    virtual void BindTexture(GLenum);

protected:
    virtual gfx::IntRect GetSrcTileRect();

    unsigned int mCurrentImage;
    BigImageIterationCallback mIterationCallback;
    void* mIterationCallbackData;
    nsTArray< nsRefPtr<TextureImage> > mImages;
    bool mInUpdate;
    unsigned int mTileSize;
    unsigned int mRows, mColumns;
    GLContext* mGL;
    
    RefPtr<gfx::DrawTarget> mUpdateDrawTarget;
    
    nsIntRegion mUpdateRegion;
    TextureState mTextureState;
    TextureImage::ImageFormat mImageFormat;
};






already_AddRefed<TextureImage>
CreateBasicTextureImage(GLContext* aGL,
                        const gfx::IntSize& aSize,
                        TextureImage::ContentType aContentType,
                        GLenum aWrapMode,
                        TextureImage::Flags aFlags,
                        TextureImage::ImageFormat aImageFormat = gfxImageFormat::Unknown);


















already_AddRefed<TextureImage>
CreateTextureImage(GLContext* gl,
                   const gfx::IntSize& aSize,
                   TextureImage::ContentType aContentType,
                   GLenum aWrapMode,
                   TextureImage::Flags aFlags = TextureImage::NoFlags,
                   TextureImage::ImageFormat aImageFormat = gfxImageFormat::Unknown);

} 
} 

#endif
