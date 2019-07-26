




#include "nsMathUtils.h"

#include "gfxQuartzNativeDrawing.h"
#include "gfxQuartzSurface.h"
#include "cairo-quartz.h"
#include "mozilla/gfx/2D.h"

enum {
    kPrivateCGCompositeSourceOver = 2
};

using namespace mozilla::gfx;
using namespace mozilla;


extern "C" {
    CG_EXTERN void CGContextSetCompositeOperation(CGContextRef, int);
}

gfxQuartzNativeDrawing::gfxQuartzNativeDrawing(gfxContext* ctx,
                                               const gfxRect& nativeRect,
                                               gfxFloat aBackingScale)
    : mContext(ctx)
    , mNativeRect(nativeRect)
    , mBackingScale(aBackingScale)
{
    mNativeRect.RoundOut();
}

CGContextRef
gfxQuartzNativeDrawing::BeginNativeDrawing()
{
    NS_ASSERTION(!mQuartzSurface, "BeginNativeDrawing called when drawing already in progress");

    if (!mContext->IsCairo()) {
      DrawTarget *dt = mContext->GetDrawTarget();
      if (mContext->GetDrawTarget()->IsDualDrawTarget()) {
        IntSize backingSize(NSToIntFloor(mNativeRect.width * mBackingScale),
                            NSToIntFloor(mNativeRect.height * mBackingScale));

       if (backingSize.IsEmpty())
          return nullptr;

        mDrawTarget = Factory::CreateDrawTarget(BACKEND_COREGRAPHICS, backingSize, FORMAT_B8G8R8A8);

        Matrix transform;
        transform.Scale(mBackingScale, mBackingScale);
        transform.Translate(-mNativeRect.x, -mNativeRect.y);

        mDrawTarget->SetTransform(transform);
        dt = mDrawTarget;
      }

      mCGContext = mBorrowedContext.Init(dt);
      MOZ_ASSERT(mCGContext);
      return mCGContext;
    }

    gfxPoint deviceOffset;
    nsRefPtr<gfxASurface> surf = mContext->CurrentSurface(&deviceOffset.x, &deviceOffset.y);
    if (!surf || surf->CairoStatus())
        return nullptr;

    
    
    
    
    if (surf->GetType() == gfxASurface::SurfaceTypeQuartz &&
        (surf->GetContentType() == gfxASurface::CONTENT_COLOR ||
         (surf->GetContentType() == gfxASurface::CONTENT_COLOR_ALPHA))) {
        mQuartzSurface = static_cast<gfxQuartzSurface*>(surf.get());
        mSurfaceContext = mContext;

        
        mCGContext = cairo_quartz_get_cg_context_with_clip(mSurfaceContext->GetCairo());
        if (!mCGContext)
            return nullptr;

        gfxMatrix m = mContext->CurrentMatrix();
        CGContextTranslateCTM(mCGContext, deviceOffset.x, deviceOffset.y);

        
        
        

        gfxFloat x0 = m.x0;
        gfxFloat y0 = m.y0;

        
        
        
        if (!m.HasNonTranslationOrFlip()) {
            x0 = floor(x0 + 0.5);
            y0 = floor(y0 + 0.5);
        }

        CGContextConcatCTM(mCGContext, CGAffineTransformMake(m.xx, m.yx,
                                                             m.xy, m.yy,
                                                             x0, y0));

        
        CGContextSetCompositeOperation(mCGContext, kPrivateCGCompositeSourceOver);
    } else {
        nsIntSize backingSize(NSToIntFloor(mNativeRect.width * mBackingScale),
                              NSToIntFloor(mNativeRect.height * mBackingScale));
        mQuartzSurface = new gfxQuartzSurface(backingSize,
                                              gfxASurface::ImageFormatARGB32);
        if (mQuartzSurface->CairoStatus())
            return nullptr;
        mSurfaceContext = new gfxContext(mQuartzSurface);

        
        mCGContext = cairo_quartz_get_cg_context_with_clip(mSurfaceContext->GetCairo());
        CGContextScaleCTM(mCGContext, mBackingScale, mBackingScale);
        CGContextTranslateCTM(mCGContext, -mNativeRect.X(), -mNativeRect.Y());
    }

    return mCGContext;
}

void
gfxQuartzNativeDrawing::EndNativeDrawing()
{
    NS_ASSERTION(mCGContext, "EndNativeDrawing called without BeginNativeDrawing");

    if (mBorrowedContext.cg) {
        MOZ_ASSERT(!mContext->IsCairo());
        mBorrowedContext.Finish();
        if (mDrawTarget) {
          DrawTarget *dest = mContext->GetDrawTarget();
          RefPtr<SourceSurface> source = mDrawTarget->Snapshot();

          IntSize backingSize(NSToIntFloor(mNativeRect.width * mBackingScale),
                              NSToIntFloor(mNativeRect.height * mBackingScale));

          Matrix oldTransform = dest->GetTransform();
          Matrix newTransform = oldTransform;
          newTransform.Translate(mNativeRect.x, mNativeRect.y);
          newTransform.Scale(1.0f / mBackingScale, 1.0f / mBackingScale);

          dest->SetTransform(newTransform);

          dest->DrawSurface(source,
                            gfx::Rect(0, 0, backingSize.width, backingSize.height),
                            gfx::Rect(0, 0, backingSize.width, backingSize.height));


          dest->SetTransform(oldTransform);
        }
        return;
    }

    cairo_quartz_finish_cg_context_with_clip(mSurfaceContext->GetCairo());
    mQuartzSurface->MarkDirty();
    if (mSurfaceContext != mContext) {
        gfxContextMatrixAutoSaveRestore save(mContext);

        
        mContext->Translate(mNativeRect.TopLeft());
        mContext->Scale(1.0f / mBackingScale, 1.0f / mBackingScale);
        mContext->DrawSurface(mQuartzSurface, mQuartzSurface->GetSize());
    }
}
