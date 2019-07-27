




#include "BasicLayersImpl.h"            
#include "CopyableCanvasLayer.h"
#include "GLContext.h"                  
#include "GLScreenBuffer.h"             
#include "SharedSurface.h"              
#include "SharedSurfaceGL.h"              
#include "gfxPattern.h"                 
#include "gfxPlatform.h"                
#include "gfxRect.h"                    
#include "gfxUtils.h"                   
#include "gfx2DGlue.h"                  
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/gfx/Tools.h"
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsSize.h"                     
#include "gfxUtils.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;
using namespace mozilla::gl;

CopyableCanvasLayer::CopyableCanvasLayer(LayerManager* aLayerManager, void *aImplData) :
  CanvasLayer(aLayerManager, aImplData)
  , mGLFrontbuffer(nullptr)
  , mIsAlphaPremultiplied(true)
  , mOriginPos(gl::OriginPos::TopLeft)
{
  MOZ_COUNT_CTOR(CopyableCanvasLayer);
}

CopyableCanvasLayer::~CopyableCanvasLayer()
{
  MOZ_COUNT_DTOR(CopyableCanvasLayer);
}

void
CopyableCanvasLayer::Initialize(const Data& aData)
{
  NS_ASSERTION(mSurface == nullptr, "BasicCanvasLayer::Initialize called twice!");

  if (aData.mGLContext) {
    mGLContext = aData.mGLContext;
    mIsAlphaPremultiplied = aData.mIsGLAlphaPremult;
    mOriginPos = gl::OriginPos::BottomLeft;

    MOZ_ASSERT(mGLContext->IsOffscreen(), "canvas gl context isn't offscreen");

    if (aData.mFrontbufferGLTex) {
      gfx::IntSize size(aData.mSize.width, aData.mSize.height);
      mGLFrontbuffer = SharedSurface_GLTexture::Create(aData.mGLContext,
                                                       nullptr,
                                                       aData.mGLContext->GetGLFormats(),
                                                       size, aData.mHasAlpha,
                                                       aData.mFrontbufferGLTex);
    }
  } else if (aData.mDrawTarget) {
    mDrawTarget = aData.mDrawTarget;
    mSurface = mDrawTarget->Snapshot();
  } else {
    MOZ_CRASH("CanvasLayer created without mSurface, mDrawTarget or mGLContext?");
  }

  mBounds.SetRect(0, 0, aData.mSize.width, aData.mSize.height);
}

bool
CopyableCanvasLayer::IsDataValid(const Data& aData)
{
  return mGLContext == aData.mGLContext;
}

void
CopyableCanvasLayer::UpdateTarget(DrawTarget* aDestTarget)
{
  if (mDrawTarget) {
    mDrawTarget->Flush();
    mSurface = mDrawTarget->Snapshot();
  }

  if (!mGLContext && aDestTarget) {
    NS_ASSERTION(mSurface, "Must have surface to draw!");
    if (mSurface) {
      aDestTarget->CopySurface(mSurface,
                               IntRect(0, 0, mBounds.width, mBounds.height),
                               IntPoint(0, 0));
      mSurface = nullptr;
    }
    return;
  }

  if (mDrawTarget) {
    return;
  }

  MOZ_ASSERT(mGLContext);

  SharedSurface* frontbuffer = nullptr;
  if (mGLFrontbuffer) {
    frontbuffer = mGLFrontbuffer.get();
  } else {
    GLScreenBuffer* screen = mGLContext->Screen();
    ShSurfHandle* front = screen->Front();
    if (front) {
      frontbuffer = front->Surf();
    }
  }

  if (!frontbuffer) {
    NS_WARNING("Null frame received.");
    return;
  }

  IntSize readSize(frontbuffer->mSize);
  SurfaceFormat format = (GetContentFlags() & CONTENT_OPAQUE)
                          ? SurfaceFormat::B8G8R8X8
                          : SurfaceFormat::B8G8R8A8;
  bool needsPremult = frontbuffer->mHasAlpha && !mIsAlphaPremultiplied;

  
  if (aDestTarget) {
    uint8_t* destData;
    IntSize destSize;
    int32_t destStride;
    SurfaceFormat destFormat;
    if (aDestTarget->LockBits(&destData, &destSize, &destStride, &destFormat)) {
      if (destSize == readSize && destFormat == format) {
        RefPtr<DataSourceSurface> data =
          Factory::CreateWrappingDataSourceSurface(destData, destStride, destSize, destFormat);
        mGLContext->Readback(frontbuffer, data);
        if (needsPremult) {
            gfxUtils::PremultiplyDataSurface(data, data);
        }
        aDestTarget->ReleaseBits(destData);
        return;
      }
      aDestTarget->ReleaseBits(destData);
    }
  }

  RefPtr<DataSourceSurface> resultSurf = GetTempSurface(readSize, format);
  
  
  if (NS_WARN_IF(!resultSurf)) {
    return;
  }

  
  mGLContext->Readback(frontbuffer, resultSurf);
  if (needsPremult) {
    gfxUtils::PremultiplyDataSurface(resultSurf, resultSurf);
  }
  MOZ_ASSERT(resultSurf);

  if (aDestTarget) {
    aDestTarget->CopySurface(resultSurf,
                             IntRect(0, 0, readSize.width, readSize.height),
                             IntPoint(0, 0));
  } else {
    
    
    mSurface = resultSurf;
  }
}

DataSourceSurface*
CopyableCanvasLayer::GetTempSurface(const IntSize& aSize,
                                    const SurfaceFormat aFormat)
{
  if (!mCachedTempSurface ||
      aSize != mCachedTempSurface->GetSize() ||
      aFormat != mCachedTempSurface->GetFormat())
  {
    
    uint32_t stride = GetAlignedStride<8>(aSize.width * BytesPerPixel(aFormat));
    mCachedTempSurface = Factory::CreateDataSourceSurfaceWithStride(aSize, aFormat, stride);
  }

  return mCachedTempSurface;
}

void
CopyableCanvasLayer::DiscardTempSurface()
{
  mCachedTempSurface = nullptr;
}

}
}
