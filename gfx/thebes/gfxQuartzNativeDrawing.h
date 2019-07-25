




































#ifndef _GFXQUARTZNATIVEDRAWING_H_
#define _GFXQUARTZNATIVEDRAWING_H_

#include "mozilla/Attributes.h"

#include "gfxContext.h"
#include "gfxQuartzSurface.h"

class THEBES_API gfxQuartzNativeDrawing {
public:

    













    gfxQuartzNativeDrawing(gfxContext *ctx,
                           const gfxRect& nativeRect);

    


    CGContextRef BeginNativeDrawing();

    
    void EndNativeDrawing();

private:
    
    gfxQuartzNativeDrawing(const gfxQuartzNativeDrawing&) MOZ_DELETE;
    const gfxQuartzNativeDrawing& operator=(const gfxQuartzNativeDrawing&) MOZ_DELETE;

    
    nsRefPtr<gfxContext> mContext;
    
    
    nsRefPtr<gfxContext> mSurfaceContext;
    gfxRect mNativeRect;

    
    nsRefPtr<gfxQuartzSurface> mQuartzSurface;
    CGContextRef mCGContext;
};

#endif
