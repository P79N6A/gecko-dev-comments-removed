




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
#include "mozilla/gfx/BaseSize.h"       
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsSize.h"                     

using namespace mozilla::gfx;
using namespace mozilla::gl;

namespace mozilla {
namespace layers {

CopyableCanvasLayer::CopyableCanvasLayer(LayerManager* aLayerManager, void *aImplData) :
  CanvasLayer(aLayerManager, aImplData)
{
  MOZ_COUNT_CTOR(CopyableCanvasLayer);
  mForceReadback = Preferences::GetBool("webgl.force-layers-readback", false);
}

CopyableCanvasLayer::~CopyableCanvasLayer()
{
  MOZ_COUNT_DTOR(CopyableCanvasLayer);
}

void
CopyableCanvasLayer::Initialize(const Data& aData)
{
  NS_ASSERTION(mSurface == nullptr, "BasicCanvasLayer::Initialize called twice!");

  if (aData.mSurface) {
    mSurface = aData.mSurface;
    NS_ASSERTION(!aData.mGLContext, "CanvasLayer can't have both surface and GLContext");
    mNeedsYFlip = false;
  } else if (aData.mGLContext) {
    mGLContext = aData.mGLContext;
    mIsGLAlphaPremult = aData.mIsGLAlphaPremult;
    mNeedsYFlip = true;
    MOZ_ASSERT(mGLContext->IsOffscreen(), "canvas gl context isn't offscreen");

    
    
  } else if (aData.mDrawTarget) {
    mDrawTarget = aData.mDrawTarget;
    mSurface =
      gfxPlatform::GetPlatform()->CreateThebesSurfaceAliasForDrawTarget_hack(mDrawTarget);
    mNeedsYFlip = false;
  } else {
    NS_ERROR("CanvasLayer created without mSurface, mDrawTarget or mGLContext?");
  }

  mBounds.SetRect(0, 0, aData.mSize.width, aData.mSize.height);
}

void
CopyableCanvasLayer::UpdateSurface(gfxASurface* aDestSurface, Layer* aMaskLayer)
{
  if (!IsDirty())
    return;
  Painted();

  if (mDrawTarget) {
    mDrawTarget->Flush();
    mSurface =
      gfxPlatform::GetPlatform()->CreateThebesSurfaceAliasForDrawTarget_hack(mDrawTarget);
  }

  if (!mGLContext && aDestSurface) {
    nsRefPtr<gfxContext> tmpCtx = new gfxContext(aDestSurface);
    tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
    CopyableCanvasLayer::PaintWithOpacity(tmpCtx, 1.0f, aMaskLayer);
    return;
  }

  if (mGLContext) {
    nsRefPtr<gfxImageSurface> readSurf;
    nsRefPtr<gfxASurface> resultSurf;

    SharedSurface* sharedSurf = mGLContext->RequestFrame();
    if (!sharedSurf) {
      NS_WARNING("Null frame received.");
      return;
    }

    gfxIntSize readSize(sharedSurf->Size());
    gfxImageFormat format = (GetContentFlags() & CONTENT_OPAQUE)
                            ? gfxImageFormatRGB24
                            : gfxImageFormatARGB32;

    if (aDestSurface) {
      resultSurf = aDestSurface;
    } else {
      resultSurf = GetTempSurface(readSize, format);
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
      readSurf = sharedSurf_Basic->GetData();
    } else {
      if (resultSurf->GetSize() != readSize ||
          !(readSurf = resultSurf->GetAsImageSurface()) ||
          readSurf->Format() != format)
      {
        readSurf = GetTempSurface(readSize, format);
      }

      
      mGLContext->Screen()->Readback(surfGL, readSurf);
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
      mSurface = resultSurf;
    }
  }
}

void
CopyableCanvasLayer::PaintWithOpacity(gfxContext* aContext,
                                      float aOpacity,
                                      Layer* aMaskLayer,
                                      gfxContext::GraphicsOperator aOperator)
{
  if (!mSurface) {
    NS_WARNING("No valid surface to draw!");
    return;
  }

  nsRefPtr<gfxPattern> pat = new gfxPattern(mSurface);

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

}
}
