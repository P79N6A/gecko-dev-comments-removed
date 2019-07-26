




#ifndef GLTEXTUREIMAGE_H_
#define GLTEXTUREIMAGE_H_

#include "nsAutoPtr.h"
#include "nsRegion.h"
#include "nsTArray.h"
#include "gfxASurface.h"
#include "GLContextTypes.h"
#include "gfxPattern.h"

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
        NeedsYFlip       = 0x2,
        ForceSingleTile  = 0x4
    };

    typedef gfxASurface::gfxContentType ContentType;

    static already_AddRefed<TextureImage> Create(
                       GLContext* gl,
                       const nsIntSize& aSize,
                       TextureImage::ContentType aContentType,
                       GLenum aWrapMode,
                       TextureImage::Flags aFlags = TextureImage::NoFlags);

    virtual ~TextureImage() {}

    




















    virtual gfxASurface* BeginUpdate(nsIntRegion& aRegion) = 0;
    






    virtual void GetUpdateRegion(nsIntRegion& aForRegion) {
    }
    






    virtual void EndUpdate() = 0;

    



    virtual void BeginTileIteration() {
    }

    virtual bool NextTile() {
        return false;
    }

    
    
    
    typedef bool (* TileIterationCallback)(TextureImage* aImage,
                                           int aTileNumber,
                                           void* aCallbackData);

    
    virtual void SetIterationCallback(TileIterationCallback aCallback,
                                      void* aCallbackData) {
    }

    virtual nsIntRect GetTileRect() {
        return nsIntRect(nsIntPoint(0,0), mSize);
    }

    virtual GLuint GetTextureID() = 0;

    virtual uint32_t GetTileCount() {
        return 1;
    }

    







    virtual void Resize(const nsIntSize& aSize) {
        mSize = aSize;
        nsIntRegion r(nsIntRect(0, 0, aSize.width, aSize.height));
        BeginUpdate(r);
        EndUpdate();
    }

    



    virtual void MarkValid() {}

    




    virtual bool DirectUpdate(gfxASurface *aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom = nsIntPoint(0,0)) = 0;

    virtual void BindTexture(GLenum aTextureUnit) = 0;
    virtual void ReleaseTexture() {}

    void BindTextureAndApplyFilter(GLenum aTextureUnit) {
        BindTexture(aTextureUnit);
        ApplyFilter();
    }

    class ScopedBindTexture
    {
    public:
        ScopedBindTexture(TextureImage *aTexture, GLenum aTextureUnit);

        ~ScopedBindTexture()
        {
            if (mTexture) {
                mTexture->ReleaseTexture();
            }
        }

    protected:
        TextureImage *mTexture;
    };

    class ScopedBindTextureAndApplyFilter
        : public ScopedBindTexture
    {
    public:
        ScopedBindTextureAndApplyFilter(TextureImage *aTexture, GLenum aTextureUnit) :
          ScopedBindTexture(aTexture, aTextureUnit)
        {
            if (mTexture) {
                mTexture->ApplyFilter();
            }
        }
    };

    




    virtual ShaderProgramType GetShaderProgramType()
    {
         return mShaderType;
    }

    

    



    virtual already_AddRefed<gfxASurface> GetBackingSurface()
    { return NULL; }

    const nsIntSize& GetSize() const { return mSize; }
    ContentType GetContentType() const { return mContentType; }
    virtual bool InUpdate() const = 0;
    GLenum GetWrapMode() const { return mWrapMode; }

    void SetFilter(gfxPattern::GraphicsFilter aFilter) { mFilter = aFilter; }

    



    virtual void ApplyFilter() = 0;

protected:
    friend class GLContext;

    





    TextureImage(const nsIntSize& aSize,
                 GLenum aWrapMode, ContentType aContentType,
                 Flags aFlags = NoFlags)
        : mSize(aSize)
        , mWrapMode(aWrapMode)
        , mContentType(aContentType)
        , mFilter(gfxPattern::FILTER_GOOD)
        , mFlags(aFlags)
    {}

    virtual nsIntRect GetSrcTileRect() {
        return nsIntRect(nsIntPoint(0,0), mSize);
    }

    nsIntSize mSize;
    GLenum mWrapMode;
    ContentType mContentType;
    ShaderProgramType mShaderType;
    gfxPattern::GraphicsFilter mFilter;
    Flags mFlags;
};










class BasicTextureImage
    : public TextureImage
{
public:
    typedef gfxASurface::gfxImageFormat ImageFormat;
    virtual ~BasicTextureImage();

    BasicTextureImage(GLuint aTexture,
                      const nsIntSize& aSize,
                      GLenum aWrapMode,
                      ContentType aContentType,
                      GLContext* aContext,
                      TextureImage::Flags aFlags = TextureImage::NoFlags)
        : TextureImage(aSize, aWrapMode, aContentType, aFlags)
        , mTexture(aTexture)
        , mTextureState(Created)
        , mGLContext(aContext)
        , mUpdateOffset(0, 0)
    {}

    virtual void BindTexture(GLenum aTextureUnit);

    virtual gfxASurface* BeginUpdate(nsIntRegion& aRegion);
    virtual void GetUpdateRegion(nsIntRegion& aForRegion);
    virtual void EndUpdate();
    virtual bool DirectUpdate(gfxASurface* aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom = nsIntPoint(0,0));
    virtual GLuint GetTextureID() { return mTexture; }
    
    virtual already_AddRefed<gfxASurface>
      GetSurfaceForUpdate(const gfxIntSize& aSize, ImageFormat aFmt);

    virtual void MarkValid() { mTextureState = Valid; }

    
    
    
    virtual bool FinishedSurfaceUpdate();

    
    virtual void FinishedSurfaceUpload();

    virtual bool InUpdate() const { return !!mUpdateSurface; }

    virtual void Resize(const nsIntSize& aSize);

    virtual void ApplyFilter();
protected:

    GLuint mTexture;
    TextureState mTextureState;
    GLContext* mGLContext;
    nsRefPtr<gfxASurface> mUpdateSurface;
    nsIntRegion mUpdateRegion;

    
    nsIntPoint mUpdateOffset;
};






class TiledTextureImage
    : public TextureImage
{
public:
    TiledTextureImage(GLContext* aGL, nsIntSize aSize,
        TextureImage::ContentType, TextureImage::Flags aFlags = TextureImage::NoFlags);
    ~TiledTextureImage();
    void DumpDiv();
    virtual gfxASurface* BeginUpdate(nsIntRegion& aRegion);
    virtual void GetUpdateRegion(nsIntRegion& aForRegion);
    virtual void EndUpdate();
    virtual void Resize(const nsIntSize& aSize);
    virtual uint32_t GetTileCount();
    virtual void BeginTileIteration();
    virtual bool NextTile();
    virtual void SetIterationCallback(TileIterationCallback aCallback,
                                      void* aCallbackData);
    virtual nsIntRect GetTileRect();
    virtual GLuint GetTextureID() {
        return mImages[mCurrentImage]->GetTextureID();
    }
    virtual bool DirectUpdate(gfxASurface* aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom = nsIntPoint(0,0));
    virtual bool InUpdate() const { return mInUpdate; }
    virtual void BindTexture(GLenum);
    virtual void ApplyFilter();

protected:
    virtual nsIntRect GetSrcTileRect();

    unsigned int mCurrentImage;
    TileIterationCallback mIterationCallback;
    void* mIterationCallbackData;
    nsTArray< nsRefPtr<TextureImage> > mImages;
    bool mInUpdate;
    nsIntSize mSize;
    unsigned int mTileSize;
    unsigned int mRows, mColumns;
    GLContext* mGL;
    
    nsRefPtr<gfxASurface> mUpdateSurface;
    
    nsIntRegion mUpdateRegion;
    TextureState mTextureState;
};

} 
} 

#endif 
