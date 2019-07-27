




#include <windows.h>

#include "nsMathUtils.h"

#include "gfxWindowsNativeDrawing.h"
#include "gfxWindowsSurface.h"
#include "gfxAlphaRecovery.h"
#include "gfxPattern.h"
#include "mozilla/gfx/2D.h"
#include "gfx2DGlue.h"

using namespace mozilla;
using namespace mozilla::gfx;

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
                                                 uint32_t nativeDrawFlags)
    : mContext(ctx), mNativeRect(nativeRect), mNativeDrawFlags(nativeDrawFlags), mRenderState(RENDER_STATE_INIT)
{
}

HDC
gfxWindowsNativeDrawing::BeginNativeDrawing()
{
    if (mRenderState == RENDER_STATE_INIT) {
        nsRefPtr<gfxASurface> surf;
        
        if (mContext->GetCairo()) {
          surf = mContext->CurrentSurface(&mDeviceOffset.x, &mDeviceOffset.y);
        }

        if (surf && surf->CairoStatus())
            return nullptr;

        gfxMatrix m = mContext->CurrentMatrix();
        if (!m.HasNonTranslation())
            mTransformType = TRANSLATION_ONLY;
        else if (m.HasNonAxisAlignedTransform())
            mTransformType = COMPLEX;
        else
            mTransformType = AXIS_ALIGNED_SCALE;

        
        
        
        if (surf &&
            ((surf->GetType() == gfxSurfaceType::Win32 ||
              surf->GetType() == gfxSurfaceType::Win32Printing) &&
              (surf->GetContentType() == gfxContentType::COLOR ||
               (surf->GetContentType() == gfxContentType::COLOR_ALPHA &&
               (mNativeDrawFlags & CAN_DRAW_TO_COLOR_ALPHA)))))
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
                    mWorldTransform.eM11 = (FLOAT) m._11;
                    mWorldTransform.eM12 = (FLOAT) m._12;
                    mWorldTransform.eM21 = (FLOAT) m._21;
                    mWorldTransform.eM22 = (FLOAT) m._22;
                    mWorldTransform.eDx  = (FLOAT) m._31;
                    mWorldTransform.eDy  = (FLOAT) m._32;

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
                    gfxIntSize((int32_t) ceil(mNativeRect.Width() + 1),
                               (int32_t) ceil(mNativeRect.Height() + 1));
            } else {
                
                mScale = m.ScaleFactors(true);

                mWorldTransform.eM11 = (FLOAT) mScale.width;
                mWorldTransform.eM12 = 0.0f;
                mWorldTransform.eM21 = 0.0f;
                mWorldTransform.eM22 = (FLOAT) mScale.height;
                mWorldTransform.eDx  = 0.0f;
                mWorldTransform.eDy  = 0.0f;

                
                mTempSurfaceSize =
                    gfxIntSize((int32_t) ceil(mNativeRect.Width() * mScale.width + 1),
                               (int32_t) ceil(mNativeRect.Height() * mScale.height + 1));
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
                         nullptr);

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
        return nullptr;
    }
}

bool
gfxWindowsNativeDrawing::IsDoublePass()
{
    if (mContext->GetDrawTarget()->GetBackendType() != mozilla::gfx::BackendType::CAIRO ||
        mContext->GetDrawTarget()->IsDualDrawTarget()) {
      return true;
    }

    nsRefPtr<gfxASurface> surf = mContext->CurrentSurface(&mDeviceOffset.x, &mDeviceOffset.y);
    if (!surf || surf->CairoStatus())
        return false;
    if (surf->GetType() != gfxSurfaceType::Win32 &&
        surf->GetType() != gfxSurfaceType::Win32Printing) {
        return true;
    }
    if ((surf->GetContentType() != gfxContentType::COLOR ||
         (surf->GetContentType() == gfxContentType::COLOR_ALPHA &&
          !(mNativeDrawFlags & CAN_DRAW_TO_COLOR_ALPHA))))
        return true;
    return false;
}

bool
gfxWindowsNativeDrawing::ShouldRenderAgain()
{
    switch (mRenderState) {
        case RENDER_STATE_NATIVE_DRAWING_DONE:
            return false;

        case RENDER_STATE_ALPHA_RECOVERY_BLACK_DONE:
            mRenderState = RENDER_STATE_ALPHA_RECOVERY_WHITE;
            return true;

        case RENDER_STATE_ALPHA_RECOVERY_WHITE_DONE:
            return false;

        default:
            NS_ERROR("Invalid RenderState in gfxWindowsNativeDrawing::ShouldRenderAgain");
            break;
    }

    return false;
}

void
gfxWindowsNativeDrawing::EndNativeDrawing()
{
    if (mRenderState == RENDER_STATE_NATIVE_DRAWING) {
        
        SetViewportOrgEx(mDC, mOrigViewportOrigin.x, mOrigViewportOrigin.y, nullptr);

        if (mTransformType != TRANSLATION_ONLY)
            SetWorldTransform(mDC, &mOldWorldTransform);

        mWinSurface->MarkDirty();

        mRenderState = RENDER_STATE_NATIVE_DRAWING_DONE;
    } else if (mRenderState == RENDER_STATE_ALPHA_RECOVERY_BLACK) {
        mBlackSurface = mWinSurface;
        mWinSurface = nullptr;

        mRenderState = RENDER_STATE_ALPHA_RECOVERY_BLACK_DONE;
    } else if (mRenderState == RENDER_STATE_ALPHA_RECOVERY_WHITE) {
        mWhiteSurface = mWinSurface;
        mWinSurface = nullptr;

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
        RefPtr<DataSourceSurface> source =
            Factory::CreateWrappingDataSourceSurface(black->Data(),
                                                     black->Stride(),
                                                     black->GetSize(),
                                                     SurfaceFormat::B8G8R8A8);

        mContext->Save();
        mContext->SetMatrix(
          mContext->CurrentMatrix().Translate(mNativeRect.TopLeft()));
        mContext->NewPath();
        mContext->Rectangle(gfxRect(gfxPoint(0.0, 0.0), mNativeRect.Size()));

        nsRefPtr<gfxPattern> pat = new gfxPattern(source, Matrix());

        gfxMatrix m;
        m.Scale(mScale.width, mScale.height);
        pat->SetMatrix(m);

        if (mNativeDrawFlags & DO_NEAREST_NEIGHBOR_FILTERING)
            pat->SetFilter(GraphicsFilter::FILTER_FAST);

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
