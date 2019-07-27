




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
                           const gfxRect& aNativeRect);

    


    CGContextRef BeginNativeDrawing();

    
    void EndNativeDrawing();

private:
    
    gfxQuartzNativeDrawing(const gfxQuartzNativeDrawing&) MOZ_DELETE;
    const gfxQuartzNativeDrawing& operator=(const gfxQuartzNativeDrawing&) MOZ_DELETE;

    
    nsRefPtr<gfxContext> mContext;
    mozilla::RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;
    mozilla::gfx::BorrowedCGContext mBorrowedContext;
    mozilla::gfx::Rect mNativeRect;

    
    CGContextRef mCGContext;
};

#endif
