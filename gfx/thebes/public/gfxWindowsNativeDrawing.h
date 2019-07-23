




































#ifndef _GFXWINDOWSNATIVEDRAWING_H_
#define _GFXWINDOWSNATIVEDRAWING_H_

#include <windows.h>

#include "gfxContext.h"
#include "gfxWindowsSurface.h"

class THEBES_API gfxWindowsNativeDrawing {
public:

    



    enum {
        
        CAN_DRAW_TO_COLOR_ALPHA    = 1 << 0,
        CANNOT_DRAW_TO_COLOR_ALPHA = 0 << 0,

        
        CAN_AXIS_ALIGNED_SCALE     = 1 << 1,
        CANNOT_AXIS_ALIGNED_SCALE  = 0 << 1,

        
        CAN_COMPLEX_TRANSFORM      = 1 << 2,
        CANNOT_COMPLEX_TRANSFORM   = 0 << 2,

        
        DO_NEAREST_NEIGHBOR_FILTERING = 1 << 3,
        DO_BILINEAR_FILTERING         = 0 << 3
    };

    





























    gfxWindowsNativeDrawing(gfxContext *ctx,
                            const gfxRect& nativeRect,
                            PRUint32 nativeDrawFlags = CANNOT_DRAW_TO_COLOR_ALPHA |
                                                       CANNOT_AXIS_ALIGNED_SCALE |
                                                       CANNOT_COMPLEX_TRANSFORM |
                                                       DO_BILINEAR_FILTERING);

    


    HDC BeginNativeDrawing();

    


    void TransformToNativeRect(const gfxRect& r, RECT& rout);

    
    void EndNativeDrawing();

    
    PRBool ShouldRenderAgain();

    
    void PaintToContext();

private:

    nsRefPtr<gfxContext> mContext;
    gfxRect mNativeRect;
    PRUint32 mNativeDrawFlags;

    
    PRUint8 mRenderState;

    gfxPoint mDeviceOffset;
    nsRefPtr<gfxPattern> mBlackPattern, mWhitePattern;

    enum TransformType {
        TRANSLATION_ONLY,
        AXIS_ALIGNED_SCALE,
        COMPLEX
    };

    TransformType mTransformType;
    gfxPoint mTranslation;
    gfxSize mScale;
    XFORM mWorldTransform;

    
    nsRefPtr<gfxWindowsSurface> mWinSurface, mBlackSurface, mWhiteSurface;
    HDC mDC;
    XFORM mOldWorldTransform;
    POINT mOrigViewportOrigin;
    gfxIntSize mTempSurfaceSize;
};

#endif
