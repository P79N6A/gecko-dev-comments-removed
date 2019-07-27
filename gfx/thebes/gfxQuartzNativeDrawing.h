




#ifndef _GFXQUARTZNATIVEDRAWING_H_
#define _GFXQUARTZNATIVEDRAWING_H_

#include "mozilla/Attributes.h"

#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/BorrowedContext.h"
#include "mozilla/RefPtr.h"

class gfxQuartzNativeDrawing {
    typedef mozilla::gfx::DrawTarget DrawTarget;
    typedef mozilla::gfx::Rect Rect;
public:

    
























    gfxQuartzNativeDrawing(DrawTarget& aDrawTarget,
                           const Rect& aNativeRect);

    


    CGContextRef BeginNativeDrawing();

    
    void EndNativeDrawing();

private:
    
    gfxQuartzNativeDrawing(const gfxQuartzNativeDrawing&) = delete;
    const gfxQuartzNativeDrawing& operator=(const gfxQuartzNativeDrawing&) = delete;

    
    mozilla::RefPtr<DrawTarget> mDrawTarget;
    mozilla::RefPtr<DrawTarget> mTempDrawTarget;
    mozilla::gfx::BorrowedCGContext mBorrowedContext;
    mozilla::gfx::Rect mNativeRect;

    
    CGContextRef mCGContext;
};

#endif
