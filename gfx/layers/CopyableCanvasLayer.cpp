




#include "BasicLayersImpl.h"            
#include "CopyableCanvasLayer.h"
#include "GLContext.h"                  
#include "GLScreenBuffer.h"             
#include "SharedSurface.h"              
#include "SharedSurfaceGL.h"            
#include "SurfaceTypes.h"               
#include "gfxImageSurface.h"            
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
    mDeprecatedSurface =
      gfxPlatform::GetPlatform()->CreateThebesSurfaceAliasForDrawTarget_hack(mDrawTarget);
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
CopyableCanvasLayer::UpdateTarget(DrawTarget* aDestTarget,
                                  SourceSurface* aMaskSurface)
{
  if (!IsDirty())
    return;
  Painted();

  if (mDrawTarget) {
    mDrawTarget->Flush();
    mSurface = mDrawTarget->Snapshot();
  }

  if (!mGLContext && aDestTarget) {
    PaintWithOpacity(aDestTarget, 1.0f, aMaskSurface);
    return;
  }

  if (mGLContext) {
    RefPtr<DataSourceSurface> readSurf;
    RefPtr<SourceSurface> resultSurf;

    SharedSurface_GL* sharedSurf = mGLContext->RequestFrame();
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
      Rect r(0, 0, readSize.width, readSize.height);
      DrawOptions opts(1.0f, CompositionOp::OP_SOURCE, AntialiasMode::DEFAULT);
      dt->DrawSurface(readSurf, r, r, DrawSurfaceOptions(), opts);
    }

    
    
    if (!aDestTarget) {
      mSurface = resultSurf;
    }
  }
}

void
CopyableCanvasLayer::DeprecatedUpdateSurface(gfxASurface* aDestSurface,
                                             Layer* aMaskLayer)
{
  if (!IsDirty())
    return;
  Painted();

  if (mDrawTarget) {
    mDrawTarget->Flush();
    mDeprecatedSurface =
      gfxPlatform::GetPlatform()->CreateThebesSurfaceAliasForDrawTarget_hack(mDrawTarget);
  }

  if (!mGLContext && aDestSurface) {
    nsRefPtr<gfxContext> tmpCtx = new gfxContext(aDestSurface);
    tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
    DeprecatedPaintWithOpacity(tmpCtx, 1.0f, aMaskLayer);
    return;
  }

  if (mGLContext) {
    nsRefPtr<gfxImageSurface> readSurf;
    RefPtr<DataSourceSurface> readDSurf;
    nsRefPtr<gfxASurface> resultSurf;

    SharedSurface_GL* sharedSurf = mGLContext->RequestFrame();
    if (!sharedSurf) {
      NS_WARNING("Null frame received.");
      return;
    }

    IntSize readSize(sharedSurf->Size());
    gfxImageFormat format = (GetContentFlags() & CONTENT_OPAQUE)
                            ? gfxImageFormat::RGB24
                            : gfxImageFormat::ARGB32;

    if (aDestSurface) {
      resultSurf = aDestSurface;
    } else {
      resultSurf = DeprecatedGetTempSurface(readSize, format);
    }
    MOZ_ASSERT(resultSurf);
    if (resultSurf->CairoStatus() != 0) {
      MOZ_ASSERT(false, "Bad resultSurf->CairoStatus().");
      return;
    }

    MOZ_ASSERT(sharedSurf->APIType() == APITypeT::OpenGL);
    SharedSurface_GL* surfGL = SharedSurface_GL::Cast(sharedSurf);

    if (surfGL->Type() == SharedSurfaceType::Basic) {
      
      
      SharedSurface_Basic* sharedSurf_Basic = SharedSurface_Basic::Cast(surfGL);
      readDSurf = sharedSurf_Basic->GetData();
      readSurf = new gfxImageSurface(readDSurf->GetData(),
                                     ThebesIntSize(readDSurf->GetSize()),
                                     readDSurf->Stride(),
                                     SurfaceFormatToImageFormat(readDSurf->GetFormat()));
    } else {
      if (ToIntSize(resultSurf->GetSize()) != readSize ||
          !(readSurf = resultSurf->GetAsImageSurface()) ||
          readSurf->Format() != format)
      {
        readSurf = DeprecatedGetTempSurface(readSize, format);
      }

      
      mGLContext->Screen()->DeprecatedReadback(surfGL, readSurf);
    }
    MOZ_ASSERT(readSurf);

    bool needsPremult = surfGL->HasAlpha() && !mIsGLAlphaPremult;
    if (needsPremult) {
      readSurf->Flush();
      gfxUtils::PremultiplyImageSurface(readSurf);
      readSurf->MarkDirty();
    }

    if (readSurf != resultSurf) {
      readSurf->Flush();
      nsRefPtr<gfxContext> ctx = new gfxContext(resultSurf);
      ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
      ctx->SetSource(readSurf);
      ctx->Paint();
    }

    
    
    if (!aDestSurface) {
      mDeprecatedSurface = resultSurf;
    }
  }
}

void
CopyableCanvasLayer::PaintWithOpacity(gfx::DrawTarget* aTarget,
                                      float aOpacity,
                                      SourceSurface* aMaskSurface,
                                      gfx::CompositionOp aOperator)
{
  if (!mSurface) {
    NS_WARNING("No valid surface to draw!");
    return;
  }

  SurfacePattern pat(mSurface, ExtendMode::CLAMP, Matrix(), ToFilter(mFilter));

  Matrix oldTransform;
  if (mNeedsYFlip) {
    oldTransform = aTarget->GetTransform();
    Matrix flipped = oldTransform;
    flipped.Translate(0, mBounds.height);
    flipped.Scale(1.0, -1.0);
    aTarget->SetTransform(flipped);
  }

  DrawOptions options = DrawOptions(aOpacity, CompositionOp::OP_SOURCE);

  if (aOperator != CompositionOp::OP_OVER) {
    options.mCompositionOp = aOperator;
  }

  
  
  Rect rect = Rect(0, 0, mBounds.width, mBounds.height);
  aTarget->FillRect(rect, pat, options);

  if (aMaskSurface) {
    aTarget->MaskSurface(pat, aMaskSurface, Point(0, 0), options);
  }

  if (mNeedsYFlip) {
    aTarget->SetTransform(oldTransform);
  }
}

void
CopyableCanvasLayer::DeprecatedPaintWithOpacity(gfxContext* aContext,
                                                float aOpacity,
                                                Layer* aMaskLayer,
                                                gfxContext::GraphicsOperator aOperator)
{
  if (!mDeprecatedSurface) {
    NS_WARNING("No valid surface to draw!");
    return;
  }

  nsRefPtr<gfxPattern> pat = new gfxPattern(mDeprecatedSurface);

  pat->SetFilter(mFilter);
  pat->SetExtend(gfxPattern::EXTEND_PAD);

  gfxMatrix m;
  if (mNeedsYFlip) {
    m = aContext->CurrentMatrix();
    aContext->Translate(gfxPoint(0.0, mBounds.height));
    aContext->Scale(1.0, -1.0);
  }

  
  
  
  gfxContext::GraphicsOperator savedOp;
  if (GetContentFlags() & CONTENT_OPAQUE) {
    savedOp = aContext->CurrentOperator();
    aContext->SetOperator(gfxContext::OPERATOR_SOURCE);
  }

  AutoSetOperator setOperator(aContext, aOperator);
  aContext->NewPath();
  
  aContext->Rectangle(gfxRect(0, 0, mBounds.width, mBounds.height));
  aContext->SetPattern(pat);

  FillWithMask(aContext, aOpacity, aMaskLayer);
  
  if (GetContentFlags() & CONTENT_OPAQUE) {
    aContext->SetOperator(savedOp);
  }

  if (mNeedsYFlip) {
    aContext->SetMatrix(m);
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

gfxImageSurface*
CopyableCanvasLayer::DeprecatedGetTempSurface(const IntSize& aSize,
                                              const gfxImageFormat aFormat)
{
  if (!mDeprecatedCachedTempSurface ||
      aSize.width != mCachedSize.width ||
      aSize.height != mCachedSize.height ||
      aFormat != mDeprecatedCachedFormat)
  {
    mDeprecatedCachedTempSurface =
      new gfxImageSurface(ThebesIntSize(aSize), aFormat);
    mCachedSize = aSize;
    mDeprecatedCachedFormat = aFormat;
  }

  MOZ_ASSERT(mDeprecatedCachedTempSurface->Stride() ==
             mDeprecatedCachedTempSurface->Width() * 4);
  return mDeprecatedCachedTempSurface;
}

void
CopyableCanvasLayer::DiscardTempSurface()
{
  mCachedTempSurface = nullptr;
  mDeprecatedCachedTempSurface = nullptr;
}

}
}
