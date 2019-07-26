




#include "GLTextureImage.h"
#include "GLContext.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"

using namespace mozilla::gl;

already_AddRefed<TextureImage>
TextureImage::Create(GLContext* gl,
                     const nsIntSize& size,
                     TextureImage::ContentType contentType,
                     GLenum wrapMode,
                     TextureImage::Flags flags)
{
    return gl->CreateTextureImage(size, contentType, wrapMode, flags);
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
        return NULL;
    }

    ImageFormat format =
        (GetContentType() == gfxASurface::CONTENT_COLOR) ?
        gfxASurface::ImageFormatRGB24 : gfxASurface::ImageFormatARGB32;
    mUpdateSurface =
        GetSurfaceForUpdate(gfxIntSize(rgnSize.width, rgnSize.height), format);

    if (!mUpdateSurface || mUpdateSurface->CairoStatus()) {
        mUpdateSurface = NULL;
        return NULL;
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

    mShaderType =
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

    mShaderType =
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
                            NULL);

    mTextureState = Allocated;
    mSize = aSize;
}

TiledTextureImage::TiledTextureImage(GLContext* aGL,
                                     nsIntSize aSize,
                                     TextureImage::ContentType aContentType,
                                     TextureImage::Flags aFlags)
    : TextureImage(aSize, LOCAL_GL_CLAMP_TO_EDGE, aContentType, aFlags)
    , mCurrentImage(0)
    , mIterationCallback(nullptr)
    , mInUpdate(false)
    , mRows(0)
    , mColumns(0)
    , mGL(aGL)
    , mTextureState(Created)
{
    if (!(aFlags & TextureImage::ForceSingleTile) && mGL->WantsSmallTiles()) {
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
        nsIntRect tileRect = GetSrcTileRect();
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

    mShaderType = mImages[0]->GetShaderProgramType();
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
        nsIntRect imageRect = nsIntRect(nsIntRect(nsIntPoint(xPos,yPos), mImages[i]->GetSize()));

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
        nsIntRegion imageRegion = nsIntRegion(nsIntRect(nsIntPoint(xPos,yPos), mImages[i]->GetSize()));

        
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

    
    gfxASurface::gfxImageFormat format =
        (GetContentType() == gfxASurface::CONTENT_COLOR) ?
        gfxASurface::ImageFormatRGB24 : gfxASurface::ImageFormatARGB32;
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
        mShaderType = mImages[mCurrentImage]->GetShaderProgramType();
        return;
    }

    
    for (unsigned i = 0; i < mImages.Length(); i++) {
        int xPos = (i % mColumns) * mTileSize;
        int yPos = (i / mColumns) * mTileSize;
        nsIntRect imageRect = nsIntRect(nsIntPoint(xPos,yPos), mImages[i]->GetSize());

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
    mShaderType = mImages[0]->GetShaderProgramType();
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

nsIntRect TiledTextureImage::GetTileRect()
{
    if (!GetTileCount()) {
        return nsIntRect();
    }
    nsIntRect rect = mImages[mCurrentImage]->GetTileRect();
    unsigned int xPos = (mCurrentImage % mColumns) * mTileSize;
    unsigned int yPos = (mCurrentImage / mColumns) * mTileSize;
    rect.MoveBy(xPos, yPos);
    return rect;
}

nsIntRect TiledTextureImage::GetSrcTileRect()
{
    nsIntRect rect = GetTileRect();
    unsigned int srcY = mFlags & NeedsYFlip
                        ? mSize.height - rect.height - rect.y
                        : rect.y;
    return nsIntRect(rect.x, srcY, rect.width, rect.height);
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
                    mGL->TileGenFunc(size, mContentType, mFlags);
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
