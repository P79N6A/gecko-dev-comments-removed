




































#include <windows.h>

#include "nsMathUtils.h"

#include "gfxWindowsNativeDrawing.h"
#include "gfxWindowsSurface.h"
#include "gfxAlphaRecovery.h"
#include "gfxPattern.h"

enum {
    RENDER_STATE_INIT,

    RENDER_STATE_NATIVE_DRAWING,
    RENDER_STATE_NATIVE_DRAWING_DONE,

    RENDER_STATE_ALPHA_RECOVERY_BLACK,
    RENDER_STATE_ALPHA_RECOVERY_BLACK_DONE,
    RENDER_STATE_ALPHA_RECOVERY_WHITE,
    RENDER_STATE_ALPHA_RECOVERY_WHITE_DONE,

    RENDER_STATE_DONE
};

gfxWindowsNativeDrawing::gfxWindowsNativeDrawing(gfxContext* ctx,
                                                 const gfxRect& nativeRect,
                                                 PRUint32 nativeDrawFlags)
    : mContext(ctx), mNativeRect(nativeRect), mNativeDrawFlags(nativeDrawFlags), mRenderState(RENDER_STATE_INIT)
{
}

HDC
gfxWindowsNativeDrawing::BeginNativeDrawing()
{
    if (mRenderState == RENDER_STATE_INIT) {
        nsRefPtr<gfxASurface> surf = mContext->CurrentSurface(&mDeviceOffset.x, &mDeviceOffset.y);
        if (!surf || surf->CairoStatus())
            return nsnull;

        gfxMatrix m = mContext->CurrentMatrix();
        if (!m.HasNonTranslation())
            mTransformType = TRANSLATION_ONLY;
        else if (m.HasNonAxisAlignedTransform())
            mTransformType = COMPLEX;
        else
            mTransformType = AXIS_ALIGNED_SCALE;

        
        
        
        if ((surf->GetType() == gfxASurface::SurfaceTypeWin32 ||
             surf->GetType() == gfxASurface::SurfaceTypeWin32Printing) &&
            (surf->GetContentType() == gfxASurface::CONTENT_COLOR ||
             (surf->GetContentType() == gfxASurface::CONTENT_COLOR_ALPHA &&
              (mNativeDrawFlags & CAN_DRAW_TO_COLOR_ALPHA))))
        {
            
            
            mWinSurface = static_cast<gfxWindowsSurface*>(static_cast<gfxASurface*>(surf.get()));
            mDC = mWinSurface->GetDCWithClip(mContext);

            if (mDC) {
                if (mTransformType == TRANSLATION_ONLY) {
                    mRenderState = RENDER_STATE_NATIVE_DRAWING;

                    mTranslation = m.GetTranslation();
                } else if (((mTransformType == AXIS_ALIGNED_SCALE)
                            && (mNativeDrawFlags & CAN_AXIS_ALIGNED_SCALE)) ||
                           (mNativeDrawFlags & CAN_COMPLEX_TRANSFORM))
                {
                    mWorldTransform.eM11 = (FLOAT) m.xx;
                    mWorldTransform.eM12 = (FLOAT) m.yx;
                    mWorldTransform.eM21 = (FLOAT) m.xy;
                    mWorldTransform.eM22 = (FLOAT) m.yy;
                    mWorldTransform.eDx  = (FLOAT) m.x0;
                    mWorldTransform.eDy  = (FLOAT) m.y0;

                    mRenderState = RENDER_STATE_NATIVE_DRAWING;
                }
            }
        }

        
        
        if (mRenderState == RENDER_STATE_INIT) {
            mRenderState = RENDER_STATE_ALPHA_RECOVERY_BLACK;

            
            
            mNativeRect.RoundOut();

            
            
            
            
            
            if (mTransformType == TRANSLATION_ONLY || !(mNativeDrawFlags & CAN_AXIS_ALIGNED_SCALE)) {
                mScale = gfxSize(1.0, 1.0);

                
                
                
                
                mTempSurfaceSize =
                    gfxIntSize((PRInt32) NS_ceil(mNativeRect.Width() + 1),
                               (PRInt32) NS_ceil(mNativeRect.Height() + 1));
            } else {
                
                mScale = m.ScaleFactors(PR_TRUE);

                mWorldTransform.eM11 = (FLOAT) mScale.width;
                mWorldTransform.eM12 = 0.0f;
                mWorldTransform.eM21 = 0.0f;
                mWorldTransform.eM22 = (FLOAT) mScale.height;
                mWorldTransform.eDx  = 0.0f;
                mWorldTransform.eDy  = 0.0f;

                
                mTempSurfaceSize =
                    gfxIntSize((PRInt32) NS_ceil(mNativeRect.Width() * mScale.width + 1),
                               (PRInt32) NS_ceil(mNativeRect.Height() * mScale.height + 1));
            }
        }
    }

    if (mRenderState == RENDER_STATE_NATIVE_DRAWING) {
        

        
        if (mTransformType != TRANSLATION_ONLY) {
            SetGraphicsMode(mDC, GM_ADVANCED);
            GetWorldTransform(mDC, &mOldWorldTransform);
            SetWorldTransform(mDC, &mWorldTransform);
        }
        GetViewportOrgEx(mDC, &mOrigViewportOrigin);
        SetViewportOrgEx(mDC,
                         mOrigViewportOrigin.x + (int)mDeviceOffset.x,
                         mOrigViewportOrigin.y + (int)mDeviceOffset.y,
                         NULL);

        return mDC;
    } else if (mRenderState == RENDER_STATE_ALPHA_RECOVERY_BLACK ||
               mRenderState == RENDER_STATE_ALPHA_RECOVERY_WHITE)
    {
        

        
        
        mWinSurface = new gfxWindowsSurface(mTempSurfaceSize);
        mDC = mWinSurface->GetDC();

        RECT r = { 0, 0, mTempSurfaceSize.width, mTempSurfaceSize.height };
        if (mRenderState == RENDER_STATE_ALPHA_RECOVERY_BLACK)
            FillRect(mDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
        else
            FillRect(mDC, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));

        if ((mTransformType != TRANSLATION_ONLY) &&
            (mNativeDrawFlags & CAN_AXIS_ALIGNED_SCALE))
        {
            SetGraphicsMode(mDC, GM_ADVANCED);
            SetWorldTransform(mDC, &mWorldTransform);
        }

        return mDC;
    } else {
        NS_ERROR("Bogus render state!");
        return nsnull;
    }
}

PRBool
gfxWindowsNativeDrawing::IsDoublePass()
{
    nsRefPtr<gfxASurface> surf = mContext->CurrentSurface(&mDeviceOffset.x, &mDeviceOffset.y);
    if (!surf || surf->CairoStatus())
        return false;
    if (surf->GetType() != gfxASurface::SurfaceTypeWin32 &&
	surf->GetType() != gfxASurface::SurfaceTypeWin32Printing) {
	return PR_TRUE;
    }
    if ((surf->GetContentType() != gfxASurface::CONTENT_COLOR ||
         (surf->GetContentType() == gfxASurface::CONTENT_COLOR_ALPHA &&
          !(mNativeDrawFlags & CAN_DRAW_TO_COLOR_ALPHA))))
        return PR_TRUE;
    return PR_FALSE;
}

PRBool
gfxWindowsNativeDrawing::ShouldRenderAgain()
{
    switch (mRenderState) {
        case RENDER_STATE_NATIVE_DRAWING_DONE:
            return PR_FALSE;

        case RENDER_STATE_ALPHA_RECOVERY_BLACK_DONE:
            mRenderState = RENDER_STATE_ALPHA_RECOVERY_WHITE;
            return PR_TRUE;

        case RENDER_STATE_ALPHA_RECOVERY_WHITE_DONE:
            return PR_FALSE;

        default:
            NS_ERROR("Invalid RenderState in gfxWindowsNativeDrawing::ShouldRenderAgain");
            break;
    }

    return PR_FALSE;
}

void
gfxWindowsNativeDrawing::EndNativeDrawing()
{
    if (mRenderState == RENDER_STATE_NATIVE_DRAWING) {
        
        SetViewportOrgEx(mDC, mOrigViewportOrigin.x, mOrigViewportOrigin.y, NULL);

        if (mTransformType != TRANSLATION_ONLY)
            SetWorldTransform(mDC, &mOldWorldTransform);

        mWinSurface->MarkDirty();

        mRenderState = RENDER_STATE_NATIVE_DRAWING_DONE;
    } else if (mRenderState == RENDER_STATE_ALPHA_RECOVERY_BLACK) {
        mBlackSurface = mWinSurface;
        mWinSurface = nsnull;

        mRenderState = RENDER_STATE_ALPHA_RECOVERY_BLACK_DONE;
    } else if (mRenderState == RENDER_STATE_ALPHA_RECOVERY_WHITE) {
        mWhiteSurface = mWinSurface;
        mWinSurface = nsnull;

        mRenderState = RENDER_STATE_ALPHA_RECOVERY_WHITE_DONE;
    } else {
        NS_ERROR("Invalid RenderState in gfxWindowsNativeDrawing::EndNativeDrawing");
    }
}

void
gfxWindowsNativeDrawing::PaintToContext()
{
    if (mRenderState == RENDER_STATE_NATIVE_DRAWING_DONE) {
        
        mRenderState = RENDER_STATE_DONE;
    } else if (mRenderState == RENDER_STATE_ALPHA_RECOVERY_WHITE_DONE) {
        nsRefPtr<gfxImageSurface> black = mBlackSurface->GetAsImageSurface();
        nsRefPtr<gfxImageSurface> white = mWhiteSurface->GetAsImageSurface();
        if (!gfxAlphaRecovery::RecoverAlpha(black, white)) {
            NS_ERROR("Alpha recovery failure");
            return;
        }
        nsRefPtr<gfxImageSurface> alphaSurface =
            new gfxImageSurface(black->Data(), black->GetSize(),
                                black->Stride(),
                                gfxASurface::ImageFormatARGB32);

        mContext->Save();
        mContext->Translate(mNativeRect.TopLeft());
        mContext->NewPath();
        mContext->Rectangle(gfxRect(gfxPoint(0.0, 0.0), mNativeRect.Size()));

        nsRefPtr<gfxPattern> pat = new gfxPattern(alphaSurface);

        gfxMatrix m;
        m.Scale(mScale.width, mScale.height);
        pat->SetMatrix(m);

        if (mNativeDrawFlags & DO_NEAREST_NEIGHBOR_FILTERING)
            pat->SetFilter(gfxPattern::FILTER_FAST);

        pat->SetExtend(gfxPattern::EXTEND_PAD);
        mContext->SetPattern(pat);
        mContext->Fill();
        mContext->Restore();

        mRenderState = RENDER_STATE_DONE;
    } else {
        NS_ERROR("Invalid RenderState in gfxWindowsNativeDrawing::PaintToContext");
    }
}

void
gfxWindowsNativeDrawing::TransformToNativeRect(const gfxRect& r,
                                               RECT& rout)
{
    




    gfxRect roundedRect(r);

    if (mRenderState == RENDER_STATE_NATIVE_DRAWING) {
        if (mTransformType == TRANSLATION_ONLY) {
            roundedRect.MoveBy(mTranslation);
        }
    } else {
        roundedRect.MoveBy(-mNativeRect.TopLeft());
    }

    roundedRect.Round();

    rout.left   = LONG(roundedRect.X());
    rout.right  = LONG(roundedRect.XMost());
    rout.top    = LONG(roundedRect.Y());
    rout.bottom = LONG(roundedRect.YMost());
}
