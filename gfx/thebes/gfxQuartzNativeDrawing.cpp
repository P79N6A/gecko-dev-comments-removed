




































#include "nsMathUtils.h"

#include "gfxQuartzNativeDrawing.h"
#include "gfxQuartzSurface.h"
#include "cairo-quartz.h"

enum {
    kPrivateCGCompositeSourceOver = 2
};


extern "C" {
    CG_EXTERN void CGContextSetCompositeOperation(CGContextRef, int);
}

gfxQuartzNativeDrawing::gfxQuartzNativeDrawing(gfxContext* ctx,
                                               const gfxRect& nativeRect)
    : mContext(ctx), mNativeRect(nativeRect)
{
    mNativeRect.RoundOut();
}

CGContextRef
gfxQuartzNativeDrawing::BeginNativeDrawing()
{
    NS_ASSERTION(!mQuartzSurface, "BeginNativeDrawing called when drawing already in progress");

    gfxPoint deviceOffset;
    nsRefPtr<gfxASurface> surf = mContext->CurrentSurface(&deviceOffset.x, &deviceOffset.y);
    if (!surf || surf->CairoStatus())
        return nsnull;

    
    
    
    
    if (surf->GetType() == gfxASurface::SurfaceTypeQuartz &&
        (surf->GetContentType() == gfxASurface::CONTENT_COLOR ||
         (surf->GetContentType() == gfxASurface::CONTENT_COLOR_ALPHA))) {
        mQuartzSurface = static_cast<gfxQuartzSurface*>(surf.get());
        mSurfaceContext = mContext;

        
        mCGContext = cairo_quartz_get_cg_context_with_clip(mSurfaceContext->GetCairo());
        if (!mCGContext)
            return nsnull;

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
        mQuartzSurface = new gfxQuartzSurface(mNativeRect.Size(),
                                              gfxASurface::ImageFormatARGB32);
        if (mQuartzSurface->CairoStatus())
            return nsnull;
        mSurfaceContext = new gfxContext(mQuartzSurface);

        
        mCGContext = cairo_quartz_get_cg_context_with_clip(mSurfaceContext->GetCairo());
        CGContextTranslateCTM(mCGContext, -mNativeRect.X(), -mNativeRect.Y());
    }

    return mCGContext;
}

void
gfxQuartzNativeDrawing::EndNativeDrawing()
{
    NS_ASSERTION(mQuartzSurface, "EndNativeDrawing called without BeginNativeDrawing");

    cairo_quartz_finish_cg_context_with_clip(mSurfaceContext->GetCairo());
    mQuartzSurface->MarkDirty();
    if (mSurfaceContext != mContext) {
        gfxContextMatrixAutoSaveRestore save(mContext);

        
        mContext->Translate(mNativeRect.TopLeft());
        mContext->DrawSurface(mQuartzSurface, mNativeRect.Size());
    }
}
