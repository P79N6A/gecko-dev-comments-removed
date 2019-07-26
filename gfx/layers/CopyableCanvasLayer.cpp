




#include "BasicLayersImpl.h"            
#include "CopyableCanvasLayer.h"
#include "GLContext.h"                  
#include "GLScreenBuffer.h"             
#include "SharedSurface.h"              
#include "SharedSurfaceGL.h"            
#include "SurfaceTypes.h"               
#include "gfxMatrix.h"                  
#include "gfxPattern.h"                 
#include "gfxPlatform.h"                
#include "gfxRect.h"                    
#include "gfxUtils.h"                   
#include "gfx2DGlue.h"                  
#include "mozilla/gfx/BaseSize.h"       
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsSize.h"                     
#include "LayerUtils.h"

using namespace mozilla::gfx;
using namespace mozilla::gl;

namespace mozilla {
namespace layers {

CopyableCanvasLayer::CopyableCanvasLayer(LayerManager* aLayerManager, void *aImplData) :
  CanvasLayer(aLayerManager, aImplData)
  , mStream(nullptr)
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
    mStream = aData.mStream;
    mIsGLAlphaPremult = aData.mIsGLAlphaPremult;
    mNeedsYFlip = true;
    MOZ_ASSERT(mGLContext->IsOffscreen(), "canvas gl context isn't offscreen");

    
    
  } else if (aData.mDrawTarget) {
    mDrawTarget = aData.mDrawTarget;
    mSurface = mDrawTarget->Snapshot();
    mNeedsYFlip = false;
  } else {
    NS_ERROR("CanvasLayer created without mSurface, mDrawTarget or mGLContext?");
  }

  mBounds.SetRect(0, 0, aData.mSize.width, aData.mSize.height);
}

bool
CopyableCanvasLayer::IsDataValid(const Data& aData)
{
  return mGLContext == aData.mGLContext && mStream == aData.mStream;
}

void
CopyableCanvasLayer::UpdateTarget(DrawTarget* aDestTarget)
{
  if (!IsDirty())
    return;
  Painted();

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
    }
    return;
  }

  if (mGLContext) {
    RefPtr<DataSourceSurface> readSurf;
    RefPtr<SourceSurface> resultSurf;

    SharedSurface_GL* sharedSurf = nullptr;
    if (mStream) {
      sharedSurf = SharedSurface_GL::Cast(mStream->SwapConsumer());
    } else {
      sharedSurf = mGLContext->RequestFrame();
    }

    if (!sharedSurf) {
      NS_WARNING("Null frame received.");
      return;
    }

    IntSize readSize(sharedSurf->Size());
    SurfaceFormat format = (GetContentFlags() & CONTENT_OPAQUE)
                            ? SurfaceFormat::B8G8R8X8
                            : SurfaceFormat::B8G8R8A8;

    if (aDestTarget) {
      resultSurf = aDestTarget->Snapshot();
      if (!resultSurf) {
        resultSurf = GetTempSurface(readSize, format);
      }
    } else {
      resultSurf = GetTempSurface(readSize, format);
    }
    MOZ_ASSERT(resultSurf);
    MOZ_ASSERT(sharedSurf->APIType() == APITypeT::OpenGL);
    SharedSurface_GL* surfGL = SharedSurface_GL::Cast(sharedSurf);

    if (surfGL->Type() == SharedSurfaceType::Basic) {
      
      
      SharedSurface_Basic* sharedSurf_Basic = SharedSurface_Basic::Cast(surfGL);
      readSurf = sharedSurf_Basic->GetData();
    } else {
      if (resultSurf->GetSize() != readSize ||
          !(readSurf = resultSurf->GetDataSurface()) ||
          readSurf->GetFormat() != format)
      {
        readSurf = GetTempSurface(readSize, format);
      }

      
      mGLContext->Screen()->Readback(surfGL, readSurf);
    }
    MOZ_ASSERT(readSurf);

    bool needsPremult = surfGL->HasAlpha() && !mIsGLAlphaPremult;
    if (needsPremult) {
      PremultiplySurface(readSurf);
    }

    if (readSurf != resultSurf) {
      RefPtr<DataSourceSurface> resultDataSurface =
        resultSurf->GetDataSurface();
      RefPtr<DrawTarget> dt =
        Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                         resultDataSurface->GetData(),
                                         resultDataSurface->GetSize(),
                                         resultDataSurface->Stride(),
                                         resultDataSurface->GetFormat());
      IntSize readSize = readSurf->GetSize();
      dt->CopySurface(readSurf,
                      IntRect(0, 0, readSize.width, readSize.height),
                      IntPoint(0, 0));
    }

    
    
    if (!aDestTarget) {
      mSurface = resultSurf;
    }
  }
}

DataSourceSurface*
CopyableCanvasLayer::GetTempSurface(const IntSize& aSize,
                                    const SurfaceFormat aFormat)
{
  if (!mCachedTempSurface ||
      aSize.width != mCachedSize.width ||
      aSize.height != mCachedSize.height ||
      aFormat != mCachedFormat)
  {
    mCachedTempSurface = Factory::CreateDataSourceSurface(aSize, aFormat);
    mCachedSize = aSize;
    mCachedFormat = aFormat;
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
