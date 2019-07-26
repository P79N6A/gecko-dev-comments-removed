




#ifndef _GFXQUARTZNATIVEDRAWING_H_
#define _GFXQUARTZNATIVEDRAWING_H_

#include "mozilla/Attributes.h"

#include "gfxContext.h"
#include "gfxQuartzSurface.h"
#include "mozilla/gfx/BorrowedContext.h"
#include "nsAutoPtr.h"

class gfxQuartzNativeDrawing {
public:

    
























    gfxQuartzNativeDrawing(gfxContext *ctx,
                           const gfxRect& aNativeRect,
                           gfxFloat aBackingScale = 1.0f);

    


    CGContextRef BeginNativeDrawing();

    
    void EndNativeDrawing();

private:
    
    gfxQuartzNativeDrawing(const gfxQuartzNativeDrawing&) MOZ_DELETE;
    const gfxQuartzNativeDrawing& operator=(const gfxQuartzNativeDrawing&) MOZ_DELETE;


    
    nsRefPtr<gfxContext> mContext;
    mozilla::RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;
    mozilla::gfx::BorrowedCGContext mBorrowedContext;
    
    
    nsRefPtr<gfxContext> mSurfaceContext;
    gfxRect mNativeRect;
    gfxFloat mBackingScale;

    
    nsRefPtr<gfxQuartzSurface> mQuartzSurface;
    CGContextRef mCGContext;
};

#endif
