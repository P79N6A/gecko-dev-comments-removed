




#include "GLTextureImage.h"
#include "GLContext.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "gfx2DGlue.h"
#include "mozilla/gfx/2D.h"
#include "ScopedGLHelpers.h"
#include "GLUploadHelpers.h"

#include "TextureImageEGL.h"
#ifdef XP_MACOSX
#include "TextureImageCGL.h"
#endif

using namespace mozilla::gfx;

namespace mozilla {
namespace gl {

already_AddRefed<TextureImage>
CreateTextureImage(GLContext* gl,
                   const gfx::IntSize& aSize,
                   TextureImage::ContentType aContentType,
                   GLenum aWrapMode,
                   TextureImage::Flags aFlags,
                   TextureImage::ImageFormat aImageFormat)
{
    switch (gl->GetContextType()) {
#ifdef XP_MACOSX
        case GLContextType::CGL:
            return CreateTextureImageCGL(gl, aSize, aContentType, aWrapMode, aFlags, aImageFormat);
#endif
        case GLContextType::EGL:
            return CreateTextureImageEGL(gl, aSize, aContentType, aWrapMode, aFlags, aImageFormat);
        default:
            return CreateBasicTextureImage(gl, aSize, aContentType, aWrapMode, aFlags, aImageFormat);
    }
}


static already_AddRefed<TextureImage>
TileGenFunc(GLContext* gl,
            const nsIntSize& aSize,
            TextureImage::ContentType aContentType,
            TextureImage::Flags aFlags,
            TextureImage::ImageFormat aImageFormat)
{
    switch (gl->GetContextType()) {
#ifdef XP_MACOSX
        case GLContextType::CGL:
            return TileGenFuncCGL(gl, aSize, aContentType, aFlags, aImageFormat);
#endif
        case GLContextType::EGL:
            return TileGenFuncEGL(gl, aSize, aContentType, aFlags, aImageFormat);
        default:
            return nullptr;
    }
}

already_AddRefed<TextureImage>
TextureImage::Create(GLContext* gl,
                     const gfx::IntSize& size,
                     TextureImage::ContentType contentType,
                     GLenum wrapMode,
                     TextureImage::Flags flags)
{
    return CreateTextureImage(gl, size, contentType, wrapMode, flags);
}

bool
TextureImage::UpdateFromDataSource(gfx::DataSourceSurface *aSurface,
                                   const nsIntRegion* aDestRegion,
                                   const gfx::IntPoint* aSrcPoint)
{
    nsIntRegion destRegion = aDestRegion ? *aDestRegion
                                         : IntRect(0, 0,
                                                     aSurface->GetSize().width,
                                                     aSurface->GetSize().height);
    gfx::IntPoint srcPoint = aSrcPoint ? *aSrcPoint
                                       : gfx::IntPoint(0, 0);
    return DirectUpdate(aSurface, destRegion, srcPoint);
}

gfx::IntRect TextureImage::GetTileRect() {
    return gfx::IntRect(gfx::IntPoint(0,0), mSize);
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

    
    
    
    
    if (ctx && ctx->MakeCurrent()) {
        ctx->fDeleteTextures(1, &mTexture);
    }
}

gfx::DrawTarget*
BasicTextureImage::BeginUpdate(nsIntRegion& aRegion)
{
    NS_ASSERTION(!mUpdateDrawTarget, "BeginUpdate() without EndUpdate()?");

    
    if (CanUploadSubTextures(mGLContext)) {
        GetUpdateRegion(aRegion);
    } else {
        aRegion = IntRect(IntPoint(0, 0), mSize);
    }

    mUpdateRegion = aRegion;

    IntRect rgnSize = mUpdateRegion.GetBounds();
    if (!IntRect(IntPoint(0, 0), mSize).Contains(rgnSize)) {
        NS_ERROR("update outside of image");
        return nullptr;
    }

    gfx::SurfaceFormat format =
        (GetContentType() == gfxContentType::COLOR) ?
        gfx::SurfaceFormat::B8G8R8X8 : gfx::SurfaceFormat::B8G8R8A8;
    mUpdateDrawTarget =
        GetDrawTargetForUpdate(gfx::IntSize(rgnSize.width, rgnSize.height), format);

    return mUpdateDrawTarget;
}

void
BasicTextureImage::GetUpdateRegion(nsIntRegion& aForRegion)
{
  
  
  
  if (mTextureState != Valid) {
      aForRegion = IntRect(IntPoint(0, 0), mSize);
  }
}

void
BasicTextureImage::EndUpdate()
{
    NS_ASSERTION(!!mUpdateDrawTarget, "EndUpdate() without BeginUpdate()?");

    

    RefPtr<gfx::SourceSurface> updateSnapshot = mUpdateDrawTarget->Snapshot();
    RefPtr<gfx::DataSourceSurface> updateData = updateSnapshot->GetDataSurface();

    bool relative = FinishedSurfaceUpdate();

    mTextureFormat =
        UploadSurfaceToTexture(mGLContext,
                               updateData,
                               mUpdateRegion,
                               mTexture,
                               mTextureState == Created,
                               mUpdateOffset,
                               relative);
    FinishedSurfaceUpload();

    mUpdateDrawTarget = nullptr;
    mTextureState = Valid;
}

void
BasicTextureImage::BindTexture(GLenum aTextureUnit)
{
    mGLContext->fActiveTexture(aTextureUnit);
    mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
    mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);
}

TemporaryRef<gfx::DrawTarget>
BasicTextureImage::GetDrawTargetForUpdate(const gfx::IntSize& aSize, gfx::SurfaceFormat aFmt)
{
    return gfx::Factory::CreateDrawTarget(gfx::BackendType::CAIRO, aSize, aFmt);
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
BasicTextureImage::DirectUpdate(gfx::DataSourceSurface* aSurf, const nsIntRegion& aRegion, const gfx::IntPoint& aFrom )
{
    IntRect bounds = aRegion.GetBounds();
    nsIntRegion region;
    if (mTextureState != Valid) {
        bounds = IntRect(0, 0, mSize.width, mSize.height);
        region = nsIntRegion(bounds);
    } else {
        region = aRegion;
    }

    mTextureFormat =
        UploadSurfaceToTexture(mGLContext,
                               aSurf,
                               region,
                               mTexture,
                               mTextureState == Created,
                               bounds.TopLeft() + IntPoint(aFrom.x, aFrom.y),
                               false);
    mTextureState = Valid;
    return true;
}

void
BasicTextureImage::Resize(const gfx::IntSize& aSize)
{
    NS_ASSERTION(!mUpdateDrawTarget, "Resize() while in update?");

    mGLContext->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

    
    
    GLenum format;
    GLenum type;
    if (mGLContext->GetPreferredARGB32Format() == LOCAL_GL_BGRA) {
        MOZ_ASSERT(!mGLContext->IsGLES());
        format = LOCAL_GL_BGRA;
        type = LOCAL_GL_UNSIGNED_INT_8_8_8_8_REV;
    } else {
        format = LOCAL_GL_RGBA;
        type = LOCAL_GL_UNSIGNED_BYTE;
    }

    mGLContext->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                            0,
                            LOCAL_GL_RGBA,
                            aSize.width,
                            aSize.height,
                            0,
                            format,
                            type,
                            nullptr);

    mTextureState = Allocated;
    mSize = aSize;
}

gfx::IntSize TextureImage::GetSize() const {
  return mSize;
}

TextureImage::TextureImage(const gfx::IntSize& aSize,
             GLenum aWrapMode, ContentType aContentType,
             Flags aFlags, ImageFormat aImageFormat)
    : mSize(aSize)
    , mWrapMode(aWrapMode)
    , mContentType(aContentType)
    , mFilter(GraphicsFilter::FILTER_GOOD)
    , mFlags(aFlags)
{}

BasicTextureImage::BasicTextureImage(GLuint aTexture,
                  const gfx::IntSize& aSize,
                  GLenum aWrapMode,
                  ContentType aContentType,
                  GLContext* aContext,
                  TextureImage::Flags aFlags,
                  TextureImage::ImageFormat aImageFormat)
  : TextureImage(aSize, aWrapMode, aContentType, aFlags, aImageFormat)
  , mTexture(aTexture)
  , mTextureState(Created)
  , mGLContext(aContext)
  , mUpdateOffset(0, 0)
{}

static bool
WantsSmallTiles(GLContext* gl)
{
    
    
    if (!CanUploadSubTextures(gl))
        return true;

    
    if (gl->WorkAroundDriverBugs() &&
        gl->Renderer() == GLRenderer::SGX540)
        return false;

    
    
    return false;
}

TiledTextureImage::TiledTextureImage(GLContext* aGL,
                                     gfx::IntSize aSize,
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
    if (!(aFlags & TextureImage::DisallowBigImage) && WantsSmallTiles(mGL)) {
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
TiledTextureImage::DirectUpdate(gfx::DataSourceSurface* aSurf, const nsIntRegion& aRegion, const gfx::IntPoint& aFrom )
{
    if (mSize.width == 0 || mSize.height == 0) {
        return true;
    }

    nsIntRegion region;

    if (mTextureState != Valid) {
        IntRect bounds = IntRect(0, 0, mSize.width, mSize.height);
        region = nsIntRegion(bounds);
    } else {
        region = aRegion;
    }

    bool result = true;
    int oldCurrentImage = mCurrentImage;
    BeginBigImageIteration();
    do {
        IntRect tileRect = GetSrcTileRect();
        int xPos = tileRect.x;
        int yPos = tileRect.y;

        nsIntRegion tileRegion;
        tileRegion.And(region, tileRect); 

        if (tileRegion.IsEmpty())
            continue;

        if (CanUploadSubTextures(mGL)) {
          tileRegion.MoveBy(-xPos, -yPos); 
        } else {
          
          tileRect.x = tileRect.y = 0;
          tileRegion = nsIntRegion(tileRect);
        }

        result &= mImages[mCurrentImage]->
          DirectUpdate(aSurf, tileRegion, aFrom + gfx::IntPoint(xPos, yPos));

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
        
        
        
        aForRegion = IntRect(IntPoint(0, 0), mSize);
        return;
    }

    nsIntRegion newRegion;

    
    
    for (unsigned i = 0; i < mImages.Length(); i++) {
        int xPos = (i % mColumns) * mTileSize;
        int yPos = (i / mColumns) * mTileSize;
        IntRect imageRect = IntRect(IntPoint(xPos,yPos),
                                        mImages[i]->GetSize());

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

gfx::DrawTarget*
TiledTextureImage::BeginUpdate(nsIntRegion& aRegion)
{
    NS_ASSERTION(!mInUpdate, "nested update");
    mInUpdate = true;

    
    
    
    if (mTextureState != Valid)
    {
        
        
        
        aRegion = IntRect(IntPoint(0, 0), mSize);
    }

    IntRect bounds = aRegion.GetBounds();

    for (unsigned i = 0; i < mImages.Length(); i++) {
        int xPos = (i % mColumns) * mTileSize;
        int yPos = (i / mColumns) * mTileSize;
        nsIntRegion imageRegion =
          nsIntRegion(IntRect(IntPoint(xPos,yPos),
                                mImages[i]->GetSize()));

        
        if (imageRegion.Contains(aRegion)) {
            
            aRegion.MoveBy(-xPos, -yPos);
            
            RefPtr<gfx::DrawTarget> drawTarget = mImages[i]->BeginUpdate(aRegion);
            
            aRegion.MoveBy(xPos, yPos);
            
            mUpdateDrawTarget = nullptr;
            
            mCurrentImage = i;
            return drawTarget.get();
        }
    }

    
    
    GetUpdateRegion(aRegion);
    mUpdateRegion = aRegion;
    bounds = aRegion.GetBounds();

    
    gfx::SurfaceFormat format =
        (GetContentType() == gfxContentType::COLOR) ?
        gfx::SurfaceFormat::B8G8R8X8: gfx::SurfaceFormat::B8G8R8A8;
    mUpdateDrawTarget = gfx::Factory::CreateDrawTarget(gfx::BackendType::CAIRO,
                                                       bounds.Size(),
                                                       format);

    return mUpdateDrawTarget;;
}

void
TiledTextureImage::EndUpdate()
{
    NS_ASSERTION(mInUpdate, "EndUpdate not in update");
    if (!mUpdateDrawTarget) { 
        mImages[mCurrentImage]->EndUpdate();
        mInUpdate = false;
        mTextureState = Valid;
        mTextureFormat = mImages[mCurrentImage]->GetTextureFormat();
        return;
    }

    RefPtr<gfx::SourceSurface> updateSnapshot = mUpdateDrawTarget->Snapshot();
    RefPtr<gfx::DataSourceSurface> updateData = updateSnapshot->GetDataSurface();

    
    for (unsigned i = 0; i < mImages.Length(); i++) {
        int xPos = (i % mColumns) * mTileSize;
        int yPos = (i / mColumns) * mTileSize;
        IntRect imageRect = IntRect(IntPoint(xPos,yPos), mImages[i]->GetSize());

        nsIntRegion subregion;
        subregion.And(mUpdateRegion, imageRect);
        if (subregion.IsEmpty())
            continue;
        subregion.MoveBy(-xPos, -yPos); 
        
        gfx::DrawTarget* drawTarget = mImages[i]->BeginUpdate(subregion);
        MOZ_ASSERT(drawTarget->GetBackendType() == BackendType::CAIRO,
                   "updateSnapshot should not have been converted to data");
        gfxUtils::ClipToRegion(drawTarget, subregion);
        Size size(updateData->GetSize().width,
                  updateData->GetSize().height);
        drawTarget->DrawSurface(updateData,
                                Rect(Point(-xPos, -yPos), size),
                                Rect(Point(0, 0), size),
                                DrawSurfaceOptions(),
                                DrawOptions(1.0, CompositionOp::OP_SOURCE,
                                            AntialiasMode::NONE));
        drawTarget->PopClip();
        mImages[i]->EndUpdate();
    }

    mUpdateDrawTarget = nullptr;
    mInUpdate = false;
    mTextureFormat = mImages[0]->GetTextureFormat();
    mTextureState = Valid;
}

void TiledTextureImage::BeginBigImageIteration()
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

void TiledTextureImage::SetIterationCallback(BigImageIterationCallback aCallback,
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
    const bool needsYFlip = mFlags & OriginBottomLeft;
    unsigned int srcY = needsYFlip ? mSize.height - rect.height - rect.y
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








void TiledTextureImage::Resize(const gfx::IntSize& aSize)
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
                TileGenFunc(mGL, size, mContentType, mFlags, mImageFormat);
            if (replace)
                mImages.ReplaceElementAt(i, teximg);
            else
                mImages.InsertElementAt(i, teximg);
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

already_AddRefed<TextureImage>
CreateBasicTextureImage(GLContext* aGL,
                        const gfx::IntSize& aSize,
                        TextureImage::ContentType aContentType,
                        GLenum aWrapMode,
                        TextureImage::Flags aFlags,
                        TextureImage::ImageFormat aImageFormat)
{
    bool useNearestFilter = aFlags & TextureImage::UseNearestFilter;
    if (!aGL->MakeCurrent()) {
      return nullptr;
    }

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
