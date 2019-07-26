




#include "GLTextureImage.h"
#include "GLContext.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "gfx2DGlue.h"

namespace mozilla {
namespace gl {

already_AddRefed<TextureImage>
TextureImage::Create(GLContext* gl,
                     const nsIntSize& size,
                     TextureImage::ContentType contentType,
                     GLenum wrapMode,
                     TextureImage::Flags flags)
{
    return gl->CreateTextureImage(size, contentType, wrapMode, flags);
}


already_AddRefed<TextureImage>
TextureImage::Create(GLContext* gl,
                     const gfx::IntSize& size,
                     TextureImage::ContentType contentType,
                     GLenum wrapMode,
                     TextureImage::Flags flags)
{
    return Create(gl, ThebesIntSize(size), contentType, wrapMode, flags);
}

bool
TextureImage::UpdateFromDataSource(gfx::DataSourceSurface *aSurface,
                                   const nsIntRegion* aDestRegion,
                                   const gfx::IntPoint* aSrcPoint)
{
    nsIntRegion destRegion = aDestRegion ? *aDestRegion
                                         : nsIntRect(0, 0,
                                                     aSurface->GetSize().width,
                                                     aSurface->GetSize().height);
    nsIntPoint thebesSrcPoint = aSrcPoint ? nsIntPoint(aSrcPoint->x, aSrcPoint->y)
                                          : nsIntPoint(0, 0);
    RefPtr<gfxASurface> thebesSurf
        = new gfxImageSurface(aSurface->GetData(),
                              ThebesIntSize(aSurface->GetSize()),
                              aSurface->Stride(),
                              SurfaceFormatToImageFormat(aSurface->GetFormat()));
    return DirectUpdate(thebesSurf, destRegion, thebesSrcPoint);
}

gfx::IntRect TextureImage::GetTileRect() {
    return gfx::IntRect(gfx::IntPoint(0,0), ToIntSize(mSize));
}

gfx::IntRect TextureImage::GetSrcTileRect() {
    return GetTileRect();
}

BasicTextureImage::~BasicTextureImage()
{
    GLContext *ctx = mGLContext;
    if (ctx->IsDestroyed() || !ctx->IsOwningThreadCurrent()) {
        ctx = ctx->GetSharedContext();
    }

    
    
    
    
    if (ctx && !ctx->IsDestroyed()) {
        mGLContext->MakeCurrent();
        mGLContext->fDeleteTextures(1, &mTexture);
    }
}

gfxASurface*
BasicTextureImage::BeginUpdate(nsIntRegion& aRegion)
{
    NS_ASSERTION(!mUpdateSurface, "BeginUpdate() without EndUpdate()?");

    
    if (mGLContext->CanUploadSubTextures()) {
        GetUpdateRegion(aRegion);
    } else {
        aRegion = nsIntRect(nsIntPoint(0, 0), mSize);
    }

    mUpdateRegion = aRegion;

    nsIntRect rgnSize = mUpdateRegion.GetBounds();
    if (!nsIntRect(nsIntPoint(0, 0), mSize).Contains(rgnSize)) {
        NS_ERROR("update outside of image");
        return nullptr;
    }

    ImageFormat format =
        (GetContentType() == GFX_CONTENT_COLOR) ?
        gfxImageFormatRGB24 : gfxImageFormatARGB32;
    mUpdateSurface =
        GetSurfaceForUpdate(gfxIntSize(rgnSize.width, rgnSize.height), format);

    if (!mUpdateSurface || mUpdateSurface->CairoStatus()) {
        mUpdateSurface = nullptr;
        return nullptr;
    }

    mUpdateSurface->SetDeviceOffset(gfxPoint(-rgnSize.x, -rgnSize.y));

    return mUpdateSurface;
}

void
BasicTextureImage::GetUpdateRegion(nsIntRegion& aForRegion)
{
  
  
  
  if (mTextureState != Valid)
      aForRegion = nsIntRect(nsIntPoint(0, 0), mSize);
}

void
BasicTextureImage::EndUpdate()
{
    NS_ASSERTION(!!mUpdateSurface, "EndUpdate() without BeginUpdate()?");

    

    
    
    mUpdateSurface->SetDeviceOffset(gfxPoint(0, 0));

    bool relative = FinishedSurfaceUpdate();

    mTextureFormat =
        mGLContext->UploadSurfaceToTexture(mUpdateSurface,
                                           mUpdateRegion,
                                           mTexture,
                                           mTextureState == Created,
                                           mUpdateOffset,
                                           relative);
    FinishedSurfaceUpload();

    mUpdateSurface = nullptr;
    mTextureState = Valid;
}

void
BasicTextureImage::BindTexture(GLenum aTextureUnit)
{
    mGLContext->fActiveTexture(aTextureUnit);
    mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
    mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);
}

void
BasicTextureImage::ApplyFilter()
{
  mGLContext->ApplyFilterToBoundTexture(mFilter);
}


already_AddRefed<gfxASurface>
BasicTextureImage::GetSurfaceForUpdate(const gfxIntSize& aSize, ImageFormat aFmt)
{
    return gfxPlatform::GetPlatform()->
        CreateOffscreenSurface(aSize, gfxASurface::ContentFromFormat(aFmt));
}

bool
BasicTextureImage::FinishedSurfaceUpdate()
{
    return false;
}

void
BasicTextureImage::FinishedSurfaceUpload()
{
}

bool
BasicTextureImage::DirectUpdate(gfxASurface* aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom )
{
    nsIntRect bounds = aRegion.GetBounds();
    nsIntRegion region;
    if (mTextureState != Valid) {
        bounds = nsIntRect(0, 0, mSize.width, mSize.height);
        region = nsIntRegion(bounds);
    } else {
        region = aRegion;
    }

    mTextureFormat =
        mGLContext->UploadSurfaceToTexture(aSurf,
                                           region,
                                           mTexture,
                                           mTextureState == Created,
                                           bounds.TopLeft() + aFrom,
                                           false);
    mTextureState = Valid;
    return true;
}

void
BasicTextureImage::Resize(const nsIntSize& aSize)
{
    NS_ASSERTION(!mUpdateSurface, "Resize() while in update?");

    mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

    mGLContext->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                            0,
                            LOCAL_GL_RGBA,
                            aSize.width,
                            aSize.height,
                            0,
                            LOCAL_GL_RGBA,
                            LOCAL_GL_UNSIGNED_BYTE,
                            nullptr);

    mTextureState = Allocated;
    mSize = aSize;
}


void TextureImage::Resize(const gfx::IntSize& aSize) {
  Resize(ThebesIntSize(aSize));
}

gfx::IntSize TextureImage::GetSize() const {
  return ToIntSize(mSize);
}

TextureImage::TextureImage(const gfx::IntSize& aSize,
             GLenum aWrapMode, ContentType aContentType,
             Flags aFlags)
    : mSize(ThebesIntSize(aSize))
    , mWrapMode(aWrapMode)
    , mContentType(aContentType)
    , mFilter(gfxPattern::FILTER_GOOD)
    , mFlags(aFlags)
{}

BasicTextureImage::BasicTextureImage(GLuint aTexture,
                                     const nsIntSize& aSize,
                                     GLenum aWrapMode,
                                     ContentType aContentType,
                                     GLContext* aContext,
                                     TextureImage::Flags aFlags ,
                                     TextureImage::ImageFormat aImageFormat )
    : TextureImage(aSize, aWrapMode, aContentType, aFlags, aImageFormat)
    , mTexture(aTexture)
    , mTextureState(Created)
    , mGLContext(aContext)
    , mUpdateOffset(0, 0)
{
}

BasicTextureImage::BasicTextureImage(GLuint aTexture,
                  const gfx::IntSize& aSize,
                  GLenum aWrapMode,
                  ContentType aContentType,
                  GLContext* aContext,
                  TextureImage::Flags aFlags,
                  TextureImage::ImageFormat aImageFormat)
  : TextureImage(ThebesIntSize(aSize), aWrapMode, aContentType, aFlags, aImageFormat)
  , mTexture(aTexture)
  , mTextureState(Created)
  , mGLContext(aContext)
  , mUpdateOffset(0, 0)
{}

already_AddRefed<TextureImage>
CreateBasicTextureImage(GLContext* aGL,
                        const gfx::IntSize& aSize,
                        TextureImage::ContentType aContentType,
                        GLenum aWrapMode,
                        TextureImage::Flags aFlags)
{
  return CreateBasicTextureImage(aGL, ThebesIntSize(aSize), aContentType, aWrapMode, aFlags);
}

TiledTextureImage::TiledTextureImage(GLContext* aGL,
                                     nsIntSize aSize,
                                     TextureImage::ContentType aContentType,
                                     TextureImage::Flags aFlags,
                                     TextureImage::ImageFormat aImageFormat)
    : TextureImage(aSize, LOCAL_GL_CLAMP_TO_EDGE, aContentType, aFlags)
    , mCurrentImage(0)
    , mIterationCallback(nullptr)
    , mInUpdate(false)
    , mRows(0)
    , mColumns(0)
    , mGL(aGL)
    , mTextureState(Created)
    , mImageFormat(aImageFormat)
{
    if (!(aFlags & TextureImage::DisallowBigImage) && mGL->WantsSmallTiles()) {
      mTileSize = 256;
    } else {
      mGL->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_SIZE, (GLint*) &mTileSize);
    }
    if (aSize.width != 0 && aSize.height != 0) {
        Resize(aSize);
    }
}

TiledTextureImage::~TiledTextureImage()
{
}

bool
TiledTextureImage::DirectUpdate(gfxASurface* aSurf, const nsIntRegion& aRegion, const nsIntPoint& aFrom )
{
    if (mSize.width == 0 || mSize.height == 0) {
        return true;
    }

    nsIntRegion region;

    if (mTextureState != Valid) {
        nsIntRect bounds = nsIntRect(0, 0, mSize.width, mSize.height);
        region = nsIntRegion(bounds);
    } else {
        region = aRegion;
    }

    bool result = true;
    int oldCurrentImage = mCurrentImage;
    BeginTileIteration();
    do {
        nsIntRect tileRect = ThebesIntRect(GetSrcTileRect());
        int xPos = tileRect.x;
        int yPos = tileRect.y;

        nsIntRegion tileRegion;
        tileRegion.And(region, tileRect); 

        if (tileRegion.IsEmpty())
            continue;

        if (mGL->CanUploadSubTextures()) {
          tileRegion.MoveBy(-xPos, -yPos); 
        } else {
          
          tileRect.x = tileRect.y = 0;
          tileRegion = nsIntRegion(tileRect);
        }

        result &= mImages[mCurrentImage]->
          DirectUpdate(aSurf, tileRegion, aFrom + nsIntPoint(xPos, yPos));

        if (mCurrentImage == mImages.Length() - 1) {
            
            
            NextTile();
            break;
        }
        
        
        
    } while (NextTile() || (mTextureState != Valid));
    mCurrentImage = oldCurrentImage;

    mTextureFormat = mImages[0]->GetTextureFormat();
    mTextureState = Valid;
    return result;
}

void
TiledTextureImage::GetUpdateRegion(nsIntRegion& aForRegion)
{
    if (mTextureState != Valid) {
        
        
        
        aForRegion = nsIntRect(nsIntPoint(0, 0), mSize);
        return;
    }

    nsIntRegion newRegion;

    
    
    for (unsigned i = 0; i < mImages.Length(); i++) {
        int xPos = (i % mColumns) * mTileSize;
        int yPos = (i / mColumns) * mTileSize;
        nsIntRect imageRect = nsIntRect(nsIntPoint(xPos,yPos),
                                        ThebesIntSize(mImages[i]->GetSize()));

        if (aForRegion.Intersects(imageRect)) {
            
            nsIntRegion subRegion;
            subRegion.And(aForRegion, imageRect);
            
            subRegion.MoveBy(-xPos, -yPos);
            
            mImages[i]->GetUpdateRegion(subRegion);
            
            subRegion.MoveBy(xPos, yPos);
            
            newRegion.Or(newRegion, subRegion);
        }
    }

    aForRegion = newRegion;
}

gfxASurface*
TiledTextureImage::BeginUpdate(nsIntRegion& aRegion)
{
    NS_ASSERTION(!mInUpdate, "nested update");
    mInUpdate = true;

    
    
    
    if (mTextureState != Valid)
    {
        
        
        
        aRegion = nsIntRect(nsIntPoint(0, 0), mSize);
    }

    nsIntRect bounds = aRegion.GetBounds();

    for (unsigned i = 0; i < mImages.Length(); i++) {
        int xPos = (i % mColumns) * mTileSize;
        int yPos = (i / mColumns) * mTileSize;
        nsIntRegion imageRegion =
          nsIntRegion(nsIntRect(nsIntPoint(xPos,yPos),
                                ThebesIntSize(mImages[i]->GetSize())));

        
        if (imageRegion.Contains(aRegion)) {
            
            aRegion.MoveBy(-xPos, -yPos);
            
            nsRefPtr<gfxASurface> surface = mImages[i]->BeginUpdate(aRegion);
            
            aRegion.MoveBy(xPos, yPos);
            
            gfxPoint offset = surface->GetDeviceOffset();
            surface->SetDeviceOffset(gfxPoint(offset.x - xPos,
                                              offset.y - yPos));
            
            mUpdateSurface = nullptr;
            
            mCurrentImage = i;
            return surface.get();
        }
    }

    
    
    GetUpdateRegion(aRegion);
    mUpdateRegion = aRegion;
    bounds = aRegion.GetBounds();

    
    gfxImageFormat format =
        (GetContentType() == GFX_CONTENT_COLOR) ?
        gfxImageFormatRGB24 : gfxImageFormatARGB32;
    mUpdateSurface = gfxPlatform::GetPlatform()->
        CreateOffscreenSurface(gfxIntSize(bounds.width, bounds.height), gfxASurface::ContentFromFormat(format));
    mUpdateSurface->SetDeviceOffset(gfxPoint(-bounds.x, -bounds.y));

    return mUpdateSurface;
}

void
TiledTextureImage::EndUpdate()
{
    NS_ASSERTION(mInUpdate, "EndUpdate not in update");
    if (!mUpdateSurface) { 
        mImages[mCurrentImage]->EndUpdate();
        mInUpdate = false;
        mTextureState = Valid;
        mTextureFormat = mImages[mCurrentImage]->GetTextureFormat();
        return;
    }

    
    for (unsigned i = 0; i < mImages.Length(); i++) {
        int xPos = (i % mColumns) * mTileSize;
        int yPos = (i / mColumns) * mTileSize;
        nsIntRect imageRect = nsIntRect(nsIntPoint(xPos,yPos),
                                        ThebesIntSize(mImages[i]->GetSize()));

        nsIntRegion subregion;
        subregion.And(mUpdateRegion, imageRect);
        if (subregion.IsEmpty())
            continue;
        subregion.MoveBy(-xPos, -yPos); 
        
        gfxASurface* surf = mImages[i]->BeginUpdate(subregion);
        nsRefPtr<gfxContext> ctx = new gfxContext(surf);
        gfxUtils::ClipToRegion(ctx, subregion);
        ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
        ctx->SetSource(mUpdateSurface, gfxPoint(-xPos, -yPos));
        ctx->Paint();
        mImages[i]->EndUpdate();
    }

    mUpdateSurface = nullptr;
    mInUpdate = false;
    mTextureFormat = mImages[0]->GetTextureFormat();
    mTextureState = Valid;
}

void TiledTextureImage::BeginTileIteration()
{
    mCurrentImage = 0;
}

bool TiledTextureImage::NextTile()
{
    bool continueIteration = true;

    if (mIterationCallback)
        continueIteration = mIterationCallback(this, mCurrentImage,
                                               mIterationCallbackData);

    if (mCurrentImage + 1 < mImages.Length()) {
        mCurrentImage++;
        return continueIteration;
    }
    return false;
}

void TiledTextureImage::SetIterationCallback(TileIterationCallback aCallback,
                                             void* aCallbackData)
{
    mIterationCallback = aCallback;
    mIterationCallbackData = aCallbackData;
}

gfx::IntRect TiledTextureImage::GetTileRect()
{
    if (!GetTileCount()) {
        return gfx::IntRect();
    }
    gfx::IntRect rect = mImages[mCurrentImage]->GetTileRect();
    unsigned int xPos = (mCurrentImage % mColumns) * mTileSize;
    unsigned int yPos = (mCurrentImage / mColumns) * mTileSize;
    rect.MoveBy(xPos, yPos);
    return rect;
}

gfx::IntRect TiledTextureImage::GetSrcTileRect()
{
    gfx::IntRect rect = GetTileRect();
    unsigned int srcY = mFlags & NeedsYFlip
                        ? mSize.height - rect.height - rect.y
                        : rect.y;
    return gfx::IntRect(rect.x, srcY, rect.width, rect.height);
}

void
TiledTextureImage::BindTexture(GLenum aTextureUnit)
{
    if (!GetTileCount()) {
        return;
    }
    mImages[mCurrentImage]->BindTexture(aTextureUnit);
}

void
TiledTextureImage::ApplyFilter()
{
   mGL->ApplyFilterToBoundTexture(mFilter);
}








void TiledTextureImage::Resize(const nsIntSize& aSize)
{
    if (mSize == aSize && mTextureState != Created) {
        return;
    }

    
    unsigned int columns = (aSize.width  + mTileSize - 1) / mTileSize;
    unsigned int rows = (aSize.height + mTileSize - 1) / mTileSize;

    
    int row;
    unsigned int i = 0;
    for (row = 0; row < (int)rows; row++) {
        
        
        if (row >= (int)mRows)
            mColumns = 0;

        
        
        
        
        
        if ((row == (int)mRows - 1) && (aSize.height != mSize.height))
            mColumns = 0;

        int col;
        for (col = 0; col < (int)columns; col++) {
            nsIntSize size( 
                    (col+1) * mTileSize > (unsigned int)aSize.width  ? aSize.width  % mTileSize : mTileSize,
                    (row+1) * mTileSize > (unsigned int)aSize.height ? aSize.height % mTileSize : mTileSize);

            bool replace = false;

            
            if (col < (int)mColumns) {
                
                
                if (mSize.width != aSize.width) {
                    if (col == (int)mColumns - 1) {
                        
                        
                        replace = true;
                    } else if (col == (int)columns - 1) {
                        
                    } else {
                        
                        
                        i++;
                        continue;
                    }
                } else {
                    
                    i++;
                    continue;
                }
            }

            
            nsRefPtr<TextureImage> teximg =
                    mGL->TileGenFunc(size, mContentType, mFlags, mImageFormat);
            if (replace)
                mImages.ReplaceElementAt(i, teximg.forget());
            else
                mImages.InsertElementAt(i, teximg.forget());
            i++;
        }

        
        if (row < (int)mRows) {
            for (col = (int)mColumns - col; col > 0; col--) {
                mImages.RemoveElementAt(i);
            }
        }
    }

    
    unsigned int length = mImages.Length();
    for (; i < length; i++)
      mImages.RemoveElementAt(mImages.Length()-1);

    
    mRows = rows;
    mColumns = columns;
    mSize = aSize;
    mTextureState = Allocated;
    mCurrentImage = 0;
}

uint32_t TiledTextureImage::GetTileCount()
{
    return mImages.Length();
}

TextureImage::ScopedBindTexture::ScopedBindTexture(TextureImage* aTexture,
                                                   GLenum aTextureUnit)
    : mTexture(aTexture)
{
    if (mTexture) {
        MOZ_ASSERT(aTextureUnit >= LOCAL_GL_TEXTURE0);
        mTexture->BindTexture(aTextureUnit);
    }
}

already_AddRefed<TextureImage>
CreateBasicTextureImage(GLContext* aGL,
                        const nsIntSize& aSize,
                        TextureImage::ContentType aContentType,
                        GLenum aWrapMode,
                        TextureImage::Flags aFlags,
                        TextureImage::ImageFormat aImageFormat)
{
    bool useNearestFilter = aFlags & TextureImage::UseNearestFilter;
    aGL->MakeCurrent();

    GLuint texture = 0;
    aGL->fGenTextures(1, &texture);

    ScopedBindTexture bind(aGL, texture);

    GLint texfilter = useNearestFilter ? LOCAL_GL_NEAREST : LOCAL_GL_LINEAR;
    aGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, texfilter);
    aGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, texfilter);
    aGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, aWrapMode);
    aGL->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, aWrapMode);

    nsRefPtr<BasicTextureImage> texImage =
        new BasicTextureImage(texture, aSize, aWrapMode, aContentType,
                              aGL, aFlags, aImageFormat);
    return texImage.forget();
}

} 
} 
