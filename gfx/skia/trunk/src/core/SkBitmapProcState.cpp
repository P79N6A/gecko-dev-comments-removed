






#include "SkBitmapProcState.h"
#include "SkColorPriv.h"
#include "SkFilterProc.h"
#include "SkPaint.h"
#include "SkShader.h"   
#include "SkUtilsArm.h"
#include "SkBitmapScaler.h"
#include "SkMipMap.h"
#include "SkPixelRef.h"
#include "SkScaledImageCache.h"
#include "SkImageEncoder.h"

#if !SK_ARM_NEON_IS_NONE

extern const SkBitmapProcState::SampleProc16 gSkBitmapProcStateSample16_neon[];
extern const SkBitmapProcState::SampleProc32 gSkBitmapProcStateSample32_neon[];
extern void  S16_D16_filter_DX_neon(const SkBitmapProcState&, const uint32_t*, int, uint16_t*);
extern void  Clamp_S16_D16_filter_DX_shaderproc_neon(const SkBitmapProcState&, int, int, uint16_t*, int);
extern void  Repeat_S16_D16_filter_DX_shaderproc_neon(const SkBitmapProcState&, int, int, uint16_t*, int);
extern void  SI8_opaque_D32_filter_DX_neon(const SkBitmapProcState&, const uint32_t*, int, SkPMColor*);
extern void  SI8_opaque_D32_filter_DX_shaderproc_neon(const SkBitmapProcState&, int, int, uint32_t*, int);
extern void  Clamp_SI8_opaque_D32_filter_DX_shaderproc_neon(const SkBitmapProcState&, int, int, uint32_t*, int);
#endif

#define   NAME_WRAP(x)  x
#include "SkBitmapProcState_filter.h"
#include "SkBitmapProcState_procs.h"




static bool matrix_only_scale_translate(const SkMatrix& m) {
    return m.getType() <= (SkMatrix::kScale_Mask | SkMatrix::kTranslate_Mask);
}





static bool just_trans_clamp(const SkMatrix& matrix, const SkBitmap& bitmap) {
    SkASSERT(matrix_only_scale_translate(matrix));

    if (matrix.getType() & SkMatrix::kScale_Mask) {
        SkRect src, dst;
        bitmap.getBounds(&src);

        
        
        
        matrix.mapPoints(SkTCast<SkPoint*>(&dst),
                         SkTCast<const SkPoint*>(&src),
                         2);

        
        
        
        
        SkIRect idst;
        dst.round(&idst);
        return idst.width() == bitmap.width() && idst.height() == bitmap.height();
    }
    
    return true;
}

static bool just_trans_general(const SkMatrix& matrix) {
    SkASSERT(matrix_only_scale_translate(matrix));

    if (matrix.getType() & SkMatrix::kScale_Mask) {
        const SkScalar tol = SK_Scalar1 / 32768;

        if (!SkScalarNearlyZero(matrix[SkMatrix::kMScaleX] - SK_Scalar1, tol)) {
            return false;
        }
        if (!SkScalarNearlyZero(matrix[SkMatrix::kMScaleY] - SK_Scalar1, tol)) {
            return false;
        }
    }
    
    return true;
}



static bool valid_for_filtering(unsigned dimension) {
    
    
    return (dimension & ~0x3FFF) == 0;
}

static SkScalar effective_matrix_scale_sqrd(const SkMatrix& mat) {
    SkPoint v1, v2;

    v1.fX = mat.getScaleX();
    v1.fY = mat.getSkewY();

    v2.fX = mat.getSkewX();
    v2.fY = mat.getScaleY();

    return SkMaxScalar(v1.lengthSqd(), v2.lengthSqd());
}

class AutoScaledCacheUnlocker {
public:
    AutoScaledCacheUnlocker(SkScaledImageCache::ID** idPtr) : fIDPtr(idPtr) {}
    ~AutoScaledCacheUnlocker() {
        if (fIDPtr && *fIDPtr) {
            SkScaledImageCache::Unlock(*fIDPtr);
            *fIDPtr = NULL;
        }
    }

    
    void release() {
        fIDPtr = NULL;
    }

private:
    SkScaledImageCache::ID** fIDPtr;
};
#define AutoScaledCacheUnlocker(...) SK_REQUIRE_LOCAL_VAR(AutoScaledCacheUnlocker)



static inline bool cache_size_okay(const SkBitmap& bm, const SkMatrix& invMat) {
    size_t maximumAllocation
        = SkScaledImageCache::GetSingleAllocationByteLimit();
    if (0 == maximumAllocation) {
        return true;
    }
    
    
    
    return bm.info().getSafeSize(bm.info().minRowBytes())
        < (maximumAllocation * invMat.getScaleX() * invMat.getScaleY());
}





bool SkBitmapProcState::possiblyScaleImage() {
    AutoScaledCacheUnlocker unlocker(&fScaledCacheID);

    SkASSERT(NULL == fBitmap);
    SkASSERT(NULL == fScaledCacheID);

    if (fFilterLevel <= SkPaint::kLow_FilterLevel) {
        return false;
    }
    
    
    

    if (SkPaint::kHigh_FilterLevel == fFilterLevel &&
        fInvMatrix.getType() <= (SkMatrix::kScale_Mask | SkMatrix::kTranslate_Mask) &&
        kN32_SkColorType == fOrigBitmap.colorType() &&
        cache_size_okay(fOrigBitmap, fInvMatrix)) {

        SkScalar invScaleX = fInvMatrix.getScaleX();
        SkScalar invScaleY = fInvMatrix.getScaleY();

        fScaledCacheID = SkScaledImageCache::FindAndLock(fOrigBitmap,
                                                         invScaleX, invScaleY,
                                                         &fScaledBitmap);
        if (fScaledCacheID) {
            fScaledBitmap.lockPixels();
            if (!fScaledBitmap.getPixels()) {
                fScaledBitmap.unlockPixels();
                
                SkScaledImageCache::Unlock(fScaledCacheID);
                fScaledCacheID = NULL;
                
            }
        }

        if (NULL == fScaledCacheID) {
            float dest_width  = fOrigBitmap.width() / invScaleX;
            float dest_height = fOrigBitmap.height() / invScaleY;

            

            if (!SkBitmapScaler::Resize(&fScaledBitmap,
                                        fOrigBitmap,
                                        SkBitmapScaler::RESIZE_BEST,
                                        dest_width,
                                        dest_height,
                                        SkScaledImageCache::GetAllocator())) {
                
                
                return false;

            }

            SkASSERT(NULL != fScaledBitmap.getPixels());
            fScaledCacheID = SkScaledImageCache::AddAndLock(fOrigBitmap,
                                                            invScaleX,
                                                            invScaleY,
                                                            fScaledBitmap);
            if (!fScaledCacheID) {
                fScaledBitmap.reset();
                return false;
            }
            SkASSERT(NULL != fScaledBitmap.getPixels());
        }

        SkASSERT(NULL != fScaledBitmap.getPixels());
        fBitmap = &fScaledBitmap;

        
        fInvMatrix.setTranslate(fInvMatrix.getTranslateX() / fInvMatrix.getScaleX(),
                                fInvMatrix.getTranslateY() / fInvMatrix.getScaleY());

        
        fFilterLevel = SkPaint::kNone_FilterLevel;
        unlocker.release();
        return true;
    }

    













    SkScalar scaleSqd = effective_matrix_scale_sqrd(fInvMatrix);

    if (SkPaint::kHigh_FilterLevel == fFilterLevel) {
        
        
        
        
        
        const SkScalar bicubicLimit = 4.0f;
        const SkScalar bicubicLimitSqd = bicubicLimit * bicubicLimit;
        if (scaleSqd < bicubicLimitSqd) {  
            return false;
        }

        
        
        fFilterLevel = SkPaint::kMedium_FilterLevel;
    }

    SkASSERT(SkPaint::kMedium_FilterLevel == fFilterLevel);

    




    if (scaleSqd > SK_Scalar1) {
        const SkMipMap* mip = NULL;

        SkASSERT(NULL == fScaledCacheID);
        fScaledCacheID = SkScaledImageCache::FindAndLockMip(fOrigBitmap, &mip);
        if (!fScaledCacheID) {
            SkASSERT(NULL == mip);
            mip = SkMipMap::Build(fOrigBitmap);
            if (mip) {
                fScaledCacheID = SkScaledImageCache::AddAndLockMip(fOrigBitmap,
                                                                   mip);
                SkASSERT(mip->getRefCnt() > 1);
                mip->unref();   
                SkASSERT(fScaledCacheID);
            }
        } else {
            SkASSERT(mip);
        }

        if (mip) {
            SkScalar levelScale = SkScalarInvert(SkScalarSqrt(scaleSqd));
            SkMipMap::Level level;
            if (mip->extractLevel(levelScale, &level)) {
                SkScalar invScaleFixup = level.fScale;
                fInvMatrix.postScale(invScaleFixup, invScaleFixup);

                SkImageInfo info = fOrigBitmap.info();
                info.fWidth = level.fWidth;
                info.fHeight = level.fHeight;
                fScaledBitmap.installPixels(info, level.fPixels, level.fRowBytes);
                fBitmap = &fScaledBitmap;
                fFilterLevel = SkPaint::kLow_FilterLevel;
                unlocker.release();
                return true;
            }
        }
    }

    return false;
}

static bool get_locked_pixels(const SkBitmap& src, int pow2, SkBitmap* dst) {
    SkPixelRef* pr = src.pixelRef();
    if (pr && pr->decodeInto(pow2, dst)) {
        return true;
    }

    




    *dst = src;
    dst->lockPixels();
    return SkToBool(dst->getPixels());
}

bool SkBitmapProcState::lockBaseBitmap() {
    AutoScaledCacheUnlocker unlocker(&fScaledCacheID);

    SkPixelRef* pr = fOrigBitmap.pixelRef();

    SkASSERT(NULL == fScaledCacheID);

    if (pr->isLocked() || !pr->implementsDecodeInto()) {
        
        fScaledBitmap = fOrigBitmap;
        fScaledBitmap.lockPixels();
        if (NULL == fScaledBitmap.getPixels()) {
            return false;
        }
    } else {
        fScaledCacheID = SkScaledImageCache::FindAndLock(fOrigBitmap,
                                                         SK_Scalar1, SK_Scalar1,
                                                         &fScaledBitmap);
        if (fScaledCacheID) {
            fScaledBitmap.lockPixels();
            if (!fScaledBitmap.getPixels()) {
                fScaledBitmap.unlockPixels();
                
                SkScaledImageCache::Unlock(fScaledCacheID);
                fScaledCacheID = NULL;
                
            }
        }

        if (NULL == fScaledCacheID) {
            if (!get_locked_pixels(fOrigBitmap, 0, &fScaledBitmap)) {
                return false;
            }

            
            

            fScaledCacheID = SkScaledImageCache::AddAndLock(fOrigBitmap,
                                                            SK_Scalar1, SK_Scalar1,
                                                            fScaledBitmap);
            if (!fScaledCacheID) {
                fScaledBitmap.reset();
                return false;
            }
        }
    }
    fBitmap = &fScaledBitmap;
    unlocker.release();
    return true;
}

SkBitmapProcState::~SkBitmapProcState() {
    if (fScaledCacheID) {
        SkScaledImageCache::Unlock(fScaledCacheID);
    }
    SkDELETE(fBitmapFilter);
}

bool SkBitmapProcState::chooseProcs(const SkMatrix& inv, const SkPaint& paint) {
    SkASSERT(fOrigBitmap.width() && fOrigBitmap.height());

    fBitmap = NULL;
    fInvMatrix = inv;
    fFilterLevel = paint.getFilterLevel();

    SkASSERT(NULL == fScaledCacheID);

    
    
    
    
    
    if (!this->possiblyScaleImage()) {
        if (!this->lockBaseBitmap()) {
            return false;
        }
    }
    
    
    
    if (NULL == fBitmap) {
        return false;
    }

    
    
    if (SkPaint::kMedium_FilterLevel == fFilterLevel) {
        fFilterLevel = SkPaint::kLow_FilterLevel;
    }

    bool trivialMatrix = (fInvMatrix.getType() & ~SkMatrix::kTranslate_Mask) == 0;
    bool clampClamp = SkShader::kClamp_TileMode == fTileModeX &&
                      SkShader::kClamp_TileMode == fTileModeY;

    if (!(clampClamp || trivialMatrix)) {
        fInvMatrix.postIDiv(fOrigBitmap.width(), fOrigBitmap.height());
    }

    
    
    
    

    
    

    if (matrix_only_scale_translate(fInvMatrix)) {
        SkMatrix forward;
        if (fInvMatrix.invert(&forward)) {
            if (clampClamp ? just_trans_clamp(forward, *fBitmap)
                            : just_trans_general(forward)) {
                SkScalar tx = -SkScalarRoundToScalar(forward.getTranslateX());
                SkScalar ty = -SkScalarRoundToScalar(forward.getTranslateY());
                fInvMatrix.setTranslate(tx, ty);
            }
        }
    }

    fInvProc        = fInvMatrix.getMapXYProc();
    fInvType        = fInvMatrix.getType();
    fInvSx          = SkScalarToFixed(fInvMatrix.getScaleX());
    fInvSxFractionalInt = SkScalarToFractionalInt(fInvMatrix.getScaleX());
    fInvKy          = SkScalarToFixed(fInvMatrix.getSkewY());
    fInvKyFractionalInt = SkScalarToFractionalInt(fInvMatrix.getSkewY());

    fAlphaScale = SkAlpha255To256(paint.getAlpha());

    fShaderProc32 = NULL;
    fShaderProc16 = NULL;
    fSampleProc32 = NULL;
    fSampleProc16 = NULL;

    
    

    trivialMatrix = (fInvMatrix.getType() & ~SkMatrix::kTranslate_Mask) == 0;

    if (SkPaint::kHigh_FilterLevel == fFilterLevel) {
        
        
        
        

        
        
        
        
        

        if (!this->setBitmapFilterProcs()) {
            fFilterLevel = SkPaint::kLow_FilterLevel;
        }
    }

    if (SkPaint::kLow_FilterLevel == fFilterLevel) {
        
        

        if (fInvType <= SkMatrix::kTranslate_Mask ||
                !valid_for_filtering(fBitmap->width() | fBitmap->height())) {
            fFilterLevel = SkPaint::kNone_FilterLevel;
        }
    }

    
    

    fMatrixProc = this->chooseMatrixProc(trivialMatrix);
    
    if (NULL == fMatrixProc) {
        return false;
    }

    

    const SkAlphaType at = fBitmap->alphaType();

    
    
    

    if (fFilterLevel < SkPaint::kHigh_FilterLevel) {

        int index = 0;
        if (fAlphaScale < 256) {  
            index |= 1;
        }
        if (fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask)) {
            index |= 2;
        }
        if (fFilterLevel > SkPaint::kNone_FilterLevel) {
            index |= 4;
        }
        
        switch (fBitmap->colorType()) {
            case kN32_SkColorType:
                if (kPremul_SkAlphaType != at && kOpaque_SkAlphaType != at) {
                    return false;
                }
                index |= 0;
                break;
            case kRGB_565_SkColorType:
                index |= 8;
                break;
            case kIndex_8_SkColorType:
                if (kPremul_SkAlphaType != at && kOpaque_SkAlphaType != at) {
                    return false;
                }
                index |= 16;
                break;
            case kARGB_4444_SkColorType:
                if (kPremul_SkAlphaType != at && kOpaque_SkAlphaType != at) {
                    return false;
                }
                index |= 24;
                break;
            case kAlpha_8_SkColorType:
                index |= 32;
                fPaintPMColor = SkPreMultiplyColor(paint.getColor());
                break;
            default:
                
                return false;
        }

    #if !SK_ARM_NEON_IS_ALWAYS
        static const SampleProc32 gSkBitmapProcStateSample32[] = {
            S32_opaque_D32_nofilter_DXDY,
            S32_alpha_D32_nofilter_DXDY,
            S32_opaque_D32_nofilter_DX,
            S32_alpha_D32_nofilter_DX,
            S32_opaque_D32_filter_DXDY,
            S32_alpha_D32_filter_DXDY,
            S32_opaque_D32_filter_DX,
            S32_alpha_D32_filter_DX,

            S16_opaque_D32_nofilter_DXDY,
            S16_alpha_D32_nofilter_DXDY,
            S16_opaque_D32_nofilter_DX,
            S16_alpha_D32_nofilter_DX,
            S16_opaque_D32_filter_DXDY,
            S16_alpha_D32_filter_DXDY,
            S16_opaque_D32_filter_DX,
            S16_alpha_D32_filter_DX,

            SI8_opaque_D32_nofilter_DXDY,
            SI8_alpha_D32_nofilter_DXDY,
            SI8_opaque_D32_nofilter_DX,
            SI8_alpha_D32_nofilter_DX,
            SI8_opaque_D32_filter_DXDY,
            SI8_alpha_D32_filter_DXDY,
            SI8_opaque_D32_filter_DX,
            SI8_alpha_D32_filter_DX,

            S4444_opaque_D32_nofilter_DXDY,
            S4444_alpha_D32_nofilter_DXDY,
            S4444_opaque_D32_nofilter_DX,
            S4444_alpha_D32_nofilter_DX,
            S4444_opaque_D32_filter_DXDY,
            S4444_alpha_D32_filter_DXDY,
            S4444_opaque_D32_filter_DX,
            S4444_alpha_D32_filter_DX,

            
            SA8_alpha_D32_nofilter_DXDY,
            SA8_alpha_D32_nofilter_DXDY,
            SA8_alpha_D32_nofilter_DX,
            SA8_alpha_D32_nofilter_DX,
            SA8_alpha_D32_filter_DXDY,
            SA8_alpha_D32_filter_DXDY,
            SA8_alpha_D32_filter_DX,
            SA8_alpha_D32_filter_DX
        };

        static const SampleProc16 gSkBitmapProcStateSample16[] = {
            S32_D16_nofilter_DXDY,
            S32_D16_nofilter_DX,
            S32_D16_filter_DXDY,
            S32_D16_filter_DX,

            S16_D16_nofilter_DXDY,
            S16_D16_nofilter_DX,
            S16_D16_filter_DXDY,
            S16_D16_filter_DX,

            SI8_D16_nofilter_DXDY,
            SI8_D16_nofilter_DX,
            SI8_D16_filter_DXDY,
            SI8_D16_filter_DX,

            
            NULL, NULL, NULL, NULL,
            
            NULL, NULL, NULL, NULL
        };
    #endif

        fSampleProc32 = SK_ARM_NEON_WRAP(gSkBitmapProcStateSample32)[index];
        index >>= 1;    
        fSampleProc16 = SK_ARM_NEON_WRAP(gSkBitmapProcStateSample16)[index];

        
        if (SK_ARM_NEON_WRAP(S16_D16_filter_DX) == fSampleProc16) {
            if (clampClamp) {
                fShaderProc16 = SK_ARM_NEON_WRAP(Clamp_S16_D16_filter_DX_shaderproc);
            } else if (SkShader::kRepeat_TileMode == fTileModeX &&
                       SkShader::kRepeat_TileMode == fTileModeY) {
                fShaderProc16 = SK_ARM_NEON_WRAP(Repeat_S16_D16_filter_DX_shaderproc);
            }
        } else if (SK_ARM_NEON_WRAP(SI8_opaque_D32_filter_DX) == fSampleProc32 && clampClamp) {
            fShaderProc32 = SK_ARM_NEON_WRAP(Clamp_SI8_opaque_D32_filter_DX_shaderproc);
        }

        if (NULL == fShaderProc32) {
            fShaderProc32 = this->chooseShaderProc32();
        }
    }

    
    this->platformProcs();

    return true;
}

static void Clamp_S32_D32_nofilter_trans_shaderproc(const SkBitmapProcState& s,
                                                    int x, int y,
                                                    SkPMColor* SK_RESTRICT colors,
                                                    int count) {
    SkASSERT(((s.fInvType & ~SkMatrix::kTranslate_Mask)) == 0);
    SkASSERT(s.fInvKy == 0);
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(SkPaint::kNone_FilterLevel == s.fFilterLevel);

    const int maxX = s.fBitmap->width() - 1;
    const int maxY = s.fBitmap->height() - 1;
    int ix = s.fFilterOneX + x;
    int iy = SkClampMax(s.fFilterOneY + y, maxY);
#ifdef SK_DEBUG
    {
        SkPoint pt;
        s.fInvProc(s.fInvMatrix, SkIntToScalar(x) + SK_ScalarHalf,
                   SkIntToScalar(y) + SK_ScalarHalf, &pt);
        int iy2 = SkClampMax(SkScalarFloorToInt(pt.fY), maxY);
        int ix2 = SkScalarFloorToInt(pt.fX);

        SkASSERT(iy == iy2);
        SkASSERT(ix == ix2);
    }
#endif
    const SkPMColor* row = s.fBitmap->getAddr32(0, iy);

    
    if (ix < 0) {
        int n = SkMin32(-ix, count);
        sk_memset32(colors, row[0], n);
        count -= n;
        if (0 == count) {
            return;
        }
        colors += n;
        SkASSERT(-ix == n);
        ix = 0;
    }
    
    if (ix <= maxX) {
        int n = SkMin32(maxX - ix + 1, count);
        memcpy(colors, row + ix, n * sizeof(SkPMColor));
        count -= n;
        if (0 == count) {
            return;
        }
        colors += n;
    }
    SkASSERT(count > 0);
    
    sk_memset32(colors, row[maxX], count);
}

static inline int sk_int_mod(int x, int n) {
    SkASSERT(n > 0);
    if ((unsigned)x >= (unsigned)n) {
        if (x < 0) {
            x = n + ~(~x % n);
        } else {
            x = x % n;
        }
    }
    return x;
}

static inline int sk_int_mirror(int x, int n) {
    x = sk_int_mod(x, 2 * n);
    if (x >= n) {
        x = n + ~(x - n);
    }
    return x;
}

static void Repeat_S32_D32_nofilter_trans_shaderproc(const SkBitmapProcState& s,
                                                     int x, int y,
                                                     SkPMColor* SK_RESTRICT colors,
                                                     int count) {
    SkASSERT(((s.fInvType & ~SkMatrix::kTranslate_Mask)) == 0);
    SkASSERT(s.fInvKy == 0);
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(SkPaint::kNone_FilterLevel == s.fFilterLevel);

    const int stopX = s.fBitmap->width();
    const int stopY = s.fBitmap->height();
    int ix = s.fFilterOneX + x;
    int iy = sk_int_mod(s.fFilterOneY + y, stopY);
#ifdef SK_DEBUG
    {
        SkPoint pt;
        s.fInvProc(s.fInvMatrix, SkIntToScalar(x) + SK_ScalarHalf,
                   SkIntToScalar(y) + SK_ScalarHalf, &pt);
        int iy2 = sk_int_mod(SkScalarFloorToInt(pt.fY), stopY);
        int ix2 = SkScalarFloorToInt(pt.fX);

        SkASSERT(iy == iy2);
        SkASSERT(ix == ix2);
    }
#endif
    const SkPMColor* row = s.fBitmap->getAddr32(0, iy);

    ix = sk_int_mod(ix, stopX);
    for (;;) {
        int n = SkMin32(stopX - ix, count);
        memcpy(colors, row + ix, n * sizeof(SkPMColor));
        count -= n;
        if (0 == count) {
            return;
        }
        colors += n;
        ix = 0;
    }
}

static void S32_D32_constX_shaderproc(const SkBitmapProcState& s,
                                      int x, int y,
                                      SkPMColor* SK_RESTRICT colors,
                                      int count) {
    SkASSERT((s.fInvType & ~(SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask)) == 0);
    SkASSERT(s.fInvKy == 0);
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(1 == s.fBitmap->width());

    int iY0;
    int iY1   SK_INIT_TO_AVOID_WARNING;
    int iSubY SK_INIT_TO_AVOID_WARNING;

    if (SkPaint::kNone_FilterLevel != s.fFilterLevel) {
        SkBitmapProcState::MatrixProc mproc = s.getMatrixProc();
        uint32_t xy[2];

        mproc(s, xy, 1, x, y);

        iY0 = xy[0] >> 18;
        iY1 = xy[0] & 0x3FFF;
        iSubY = (xy[0] >> 14) & 0xF;
    } else {
        int yTemp;

        if (s.fInvType > SkMatrix::kTranslate_Mask) {
            SkPoint pt;
            s.fInvProc(s.fInvMatrix,
                       SkIntToScalar(x) + SK_ScalarHalf,
                       SkIntToScalar(y) + SK_ScalarHalf,
                       &pt);
            
            
            
            
            if (SkShader::kClamp_TileMode != s.fTileModeX ||
                SkShader::kClamp_TileMode != s.fTileModeY) {
                yTemp = SkScalarFloorToInt(pt.fY * s.fBitmap->height());
            } else {
                yTemp = SkScalarFloorToInt(pt.fY);
            }
        } else {
            yTemp = s.fFilterOneY + y;
        }

        const int stopY = s.fBitmap->height();
        switch (s.fTileModeY) {
            case SkShader::kClamp_TileMode:
                iY0 = SkClampMax(yTemp, stopY-1);
                break;
            case SkShader::kRepeat_TileMode:
                iY0 = sk_int_mod(yTemp, stopY);
                break;
            case SkShader::kMirror_TileMode:
            default:
                iY0 = sk_int_mirror(yTemp, stopY);
                break;
        }

#ifdef SK_DEBUG
        {
            SkPoint pt;
            s.fInvProc(s.fInvMatrix,
                       SkIntToScalar(x) + SK_ScalarHalf,
                       SkIntToScalar(y) + SK_ScalarHalf,
                       &pt);
            if (s.fInvType > SkMatrix::kTranslate_Mask &&
                (SkShader::kClamp_TileMode != s.fTileModeX ||
                 SkShader::kClamp_TileMode != s.fTileModeY)) {
                pt.fY *= s.fBitmap->height();
            }
            int iY2;

            switch (s.fTileModeY) {
            case SkShader::kClamp_TileMode:
                iY2 = SkClampMax(SkScalarFloorToInt(pt.fY), stopY-1);
                break;
            case SkShader::kRepeat_TileMode:
                iY2 = sk_int_mod(SkScalarFloorToInt(pt.fY), stopY);
                break;
            case SkShader::kMirror_TileMode:
            default:
                iY2 = sk_int_mirror(SkScalarFloorToInt(pt.fY), stopY);
                break;
            }

            SkASSERT(iY0 == iY2);
        }
#endif
    }

    const SkPMColor* row0 = s.fBitmap->getAddr32(0, iY0);
    SkPMColor color;

    if (SkPaint::kNone_FilterLevel != s.fFilterLevel) {
        const SkPMColor* row1 = s.fBitmap->getAddr32(0, iY1);

        if (s.fAlphaScale < 256) {
            Filter_32_alpha(iSubY, *row0, *row1, &color, s.fAlphaScale);
        } else {
            Filter_32_opaque(iSubY, *row0, *row1, &color);
        }
    } else {
        if (s.fAlphaScale < 256) {
            color = SkAlphaMulQ(*row0, s.fAlphaScale);
        } else {
            color = *row0;
        }
    }

    sk_memset32(colors, color, count);
}

static void DoNothing_shaderproc(const SkBitmapProcState&, int x, int y,
                                 SkPMColor* SK_RESTRICT colors, int count) {
    
    sk_memset32(colors, 0, count);
}

bool SkBitmapProcState::setupForTranslate() {
    SkPoint pt;
    fInvProc(fInvMatrix, SK_ScalarHalf, SK_ScalarHalf, &pt);

    




    const SkScalar too_big = SkIntToScalar(1 << 30);
    if (SkScalarAbs(pt.fX) > too_big || SkScalarAbs(pt.fY) > too_big) {
        return false;
    }

    
    
    
    fFilterOneX = SkScalarFloorToInt(pt.fX);
    fFilterOneY = SkScalarFloorToInt(pt.fY);
    return true;
}

SkBitmapProcState::ShaderProc32 SkBitmapProcState::chooseShaderProc32() {

    if (kN32_SkColorType != fBitmap->colorType()) {
        return NULL;
    }

    static const unsigned kMask = SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask;

    if (1 == fBitmap->width() && 0 == (fInvType & ~kMask)) {
        if (SkPaint::kNone_FilterLevel == fFilterLevel &&
            fInvType <= SkMatrix::kTranslate_Mask &&
            !this->setupForTranslate()) {
            return DoNothing_shaderproc;
        }
        return S32_D32_constX_shaderproc;
    }

    if (fAlphaScale < 256) {
        return NULL;
    }
    if (fInvType > SkMatrix::kTranslate_Mask) {
        return NULL;
    }
    if (SkPaint::kNone_FilterLevel != fFilterLevel) {
        return NULL;
    }

    SkShader::TileMode tx = (SkShader::TileMode)fTileModeX;
    SkShader::TileMode ty = (SkShader::TileMode)fTileModeY;

    if (SkShader::kClamp_TileMode == tx && SkShader::kClamp_TileMode == ty) {
        if (this->setupForTranslate()) {
            return Clamp_S32_D32_nofilter_trans_shaderproc;
        }
        return DoNothing_shaderproc;
    }
    if (SkShader::kRepeat_TileMode == tx && SkShader::kRepeat_TileMode == ty) {
        if (this->setupForTranslate()) {
            return Repeat_S32_D32_nofilter_trans_shaderproc;
        }
        return DoNothing_shaderproc;
    }
    return NULL;
}



#ifdef SK_DEBUG

static void check_scale_nofilter(uint32_t bitmapXY[], int count,
                                 unsigned mx, unsigned my) {
    unsigned y = *bitmapXY++;
    SkASSERT(y < my);

    const uint16_t* xptr = reinterpret_cast<const uint16_t*>(bitmapXY);
    for (int i = 0; i < count; ++i) {
        SkASSERT(xptr[i] < mx);
    }
}

static void check_scale_filter(uint32_t bitmapXY[], int count,
                                 unsigned mx, unsigned my) {
    uint32_t YY = *bitmapXY++;
    unsigned y0 = YY >> 18;
    unsigned y1 = YY & 0x3FFF;
    SkASSERT(y0 < my);
    SkASSERT(y1 < my);

    for (int i = 0; i < count; ++i) {
        uint32_t XX = bitmapXY[i];
        unsigned x0 = XX >> 18;
        unsigned x1 = XX & 0x3FFF;
        SkASSERT(x0 < mx);
        SkASSERT(x1 < mx);
    }
}

static void check_affine_nofilter(uint32_t bitmapXY[], int count,
                                 unsigned mx, unsigned my) {
    for (int i = 0; i < count; ++i) {
        uint32_t XY = bitmapXY[i];
        unsigned x = XY & 0xFFFF;
        unsigned y = XY >> 16;
        SkASSERT(x < mx);
        SkASSERT(y < my);
    }
}

static void check_affine_filter(uint32_t bitmapXY[], int count,
                                 unsigned mx, unsigned my) {
    for (int i = 0; i < count; ++i) {
        uint32_t YY = *bitmapXY++;
        unsigned y0 = YY >> 18;
        unsigned y1 = YY & 0x3FFF;
        SkASSERT(y0 < my);
        SkASSERT(y1 < my);

        uint32_t XX = *bitmapXY++;
        unsigned x0 = XX >> 18;
        unsigned x1 = XX & 0x3FFF;
        SkASSERT(x0 < mx);
        SkASSERT(x1 < mx);
    }
}

void SkBitmapProcState::DebugMatrixProc(const SkBitmapProcState& state,
                                        uint32_t bitmapXY[], int count,
                                        int x, int y) {
    SkASSERT(bitmapXY);
    SkASSERT(count > 0);

    state.fMatrixProc(state, bitmapXY, count, x, y);

    void (*proc)(uint32_t bitmapXY[], int count, unsigned mx, unsigned my);

    
    
    
    if (state.fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask)) {
        proc = state.fFilterLevel != SkPaint::kNone_FilterLevel ? check_scale_filter : check_scale_nofilter;
    } else {
        proc = state.fFilterLevel != SkPaint::kNone_FilterLevel ? check_affine_filter : check_affine_nofilter;
    }
    proc(bitmapXY, count, state.fBitmap->width(), state.fBitmap->height());
}

SkBitmapProcState::MatrixProc SkBitmapProcState::getMatrixProc() const {
    return DebugMatrixProc;
}

#endif











int SkBitmapProcState::maxCountForBufferSize(size_t bufferSize) const {
    int32_t size = static_cast<int32_t>(bufferSize);

    size &= ~3; 
    if (fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask)) {
        size -= 4;   
        if (size < 0) {
            size = 0;
        }
        size >>= 1;
    } else {
        size >>= 2;
    }

    if (fFilterLevel != SkPaint::kNone_FilterLevel) {
        size >>= 1;
    }

    return size;
}
