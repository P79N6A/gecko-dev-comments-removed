






#include "SkDraw.h"
#include "SkBlitter.h"
#include "SkCanvas.h"
#include "SkColorPriv.h"
#include "SkDevice.h"
#include "SkDeviceLooper.h"
#include "SkFixed.h"
#include "SkMaskFilter.h"
#include "SkPaint.h"
#include "SkPathEffect.h"
#include "SkRasterClip.h"
#include "SkRasterizer.h"
#include "SkRRect.h"
#include "SkScan.h"
#include "SkShader.h"
#include "SkSmallAllocator.h"
#include "SkString.h"
#include "SkStroke.h"
#include "SkTextMapStateProc.h"
#include "SkTLazy.h"
#include "SkUtils.h"
#include "SkVertState.h"

#include "SkAutoKern.h"
#include "SkBitmapProcShader.h"
#include "SkDrawProcs.h"
#include "SkMatrixUtils.h"







class SkAutoBlitterChoose : SkNoncopyable {
public:
    SkAutoBlitterChoose() {
        fBlitter = NULL;
    }
    SkAutoBlitterChoose(const SkBitmap& device, const SkMatrix& matrix,
                        const SkPaint& paint, bool drawCoverage = false) {
        fBlitter = SkBlitter::Choose(device, matrix, paint, &fAllocator,
                                     drawCoverage);
    }

    SkBlitter*  operator->() { return fBlitter; }
    SkBlitter*  get() const { return fBlitter; }

    void choose(const SkBitmap& device, const SkMatrix& matrix,
                const SkPaint& paint) {
        SkASSERT(!fBlitter);
        fBlitter = SkBlitter::Choose(device, matrix, paint, &fAllocator);
    }

private:
    
    SkBlitter*          fBlitter;
    SkTBlitterAllocator fAllocator;
};
#define SkAutoBlitterChoose(...) SK_REQUIRE_LOCAL_VAR(SkAutoBlitterChoose)






class SkAutoBitmapShaderInstall : SkNoncopyable {
public:
    SkAutoBitmapShaderInstall(const SkBitmap& src, const SkPaint& paint,
                              const SkMatrix* localMatrix = NULL)
            : fPaint(paint)  {
        fPaint.setShader(CreateBitmapShader(src, SkShader::kClamp_TileMode,
                                            SkShader::kClamp_TileMode,
                                            localMatrix, &fAllocator));
        
        SkASSERT(2 == fPaint.getShader()->getRefCnt());
    }

    ~SkAutoBitmapShaderInstall() {
        
        SkASSERT(2 == fPaint.getShader()->getRefCnt());

        fPaint.setShader(NULL); 

    }

    
    const SkPaint& paintWithShader() const { return fPaint; }

private:
    
    SkPaint             fPaint;
    
    SkTBlitterAllocator fAllocator;
};
#define SkAutoBitmapShaderInstall(...) SK_REQUIRE_LOCAL_VAR(SkAutoBitmapShaderInstall)



SkDraw::SkDraw() {
    sk_bzero(this, sizeof(*this));
}

SkDraw::SkDraw(const SkDraw& src) {
    memcpy(this, &src, sizeof(*this));
}

bool SkDraw::computeConservativeLocalClipBounds(SkRect* localBounds) const {
    if (fRC->isEmpty()) {
        return false;
    }

    SkMatrix inverse;
    if (!fMatrix->invert(&inverse)) {
        return false;
    }

    SkIRect devBounds = fRC->getBounds();
    
    devBounds.outset(1, 1);
    inverse.mapRect(localBounds, SkRect::Make(devBounds));
    return true;
}



typedef void (*BitmapXferProc)(void* pixels, size_t bytes, uint32_t data);

static void D_Clear_BitmapXferProc(void* pixels, size_t bytes, uint32_t) {
    sk_bzero(pixels, bytes);
}

static void D_Dst_BitmapXferProc(void*, size_t, uint32_t data) {}

static void D32_Src_BitmapXferProc(void* pixels, size_t bytes, uint32_t data) {
    sk_memset32((uint32_t*)pixels, data, SkToInt(bytes >> 2));
}

static void D16_Src_BitmapXferProc(void* pixels, size_t bytes, uint32_t data) {
    sk_memset16((uint16_t*)pixels, data, SkToInt(bytes >> 1));
}

static void DA8_Src_BitmapXferProc(void* pixels, size_t bytes, uint32_t data) {
    memset(pixels, data, bytes);
}

static BitmapXferProc ChooseBitmapXferProc(const SkBitmap& bitmap,
                                           const SkPaint& paint,
                                           uint32_t* data) {
    
    
    if (paint.getShader() || paint.getColorFilter()) {
        return NULL;
    }

    SkXfermode::Mode mode;
    if (!SkXfermode::AsMode(paint.getXfermode(), &mode)) {
        return NULL;
    }

    SkColor color = paint.getColor();

    
    if (SkXfermode::kSrcOver_Mode == mode) {
        unsigned alpha = SkColorGetA(color);
        if (0 == alpha) {
            mode = SkXfermode::kDst_Mode;
        } else if (0xFF == alpha) {
            mode = SkXfermode::kSrc_Mode;
        }
    }

    switch (mode) {
        case SkXfermode::kClear_Mode:

            return D_Clear_BitmapXferProc;  
        case SkXfermode::kDst_Mode:

            return D_Dst_BitmapXferProc;    
        case SkXfermode::kSrc_Mode: {
            


            SkPMColor pmc = SkPreMultiplyColor(color);
            switch (bitmap.colorType()) {
                case kN32_SkColorType:
                    if (data) {
                        *data = pmc;
                    }

                    return D32_Src_BitmapXferProc;
                case kRGB_565_SkColorType:
                    if (data) {
                        *data = SkPixel32ToPixel16(pmc);
                    }

                    return D16_Src_BitmapXferProc;
                case kAlpha_8_SkColorType:
                    if (data) {
                        *data = SkGetPackedA32(pmc);
                    }

                    return DA8_Src_BitmapXferProc;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    return NULL;
}

static void CallBitmapXferProc(const SkBitmap& bitmap, const SkIRect& rect,
                               BitmapXferProc proc, uint32_t procData) {
    int shiftPerPixel;
    switch (bitmap.colorType()) {
        case kN32_SkColorType:
            shiftPerPixel = 2;
            break;
        case kRGB_565_SkColorType:
            shiftPerPixel = 1;
            break;
        case kAlpha_8_SkColorType:
            shiftPerPixel = 0;
            break;
        default:
            SkDEBUGFAIL("Can't use xferproc on this config");
            return;
    }

    uint8_t* pixels = (uint8_t*)bitmap.getPixels();
    SkASSERT(pixels);
    const size_t rowBytes = bitmap.rowBytes();
    const int widthBytes = rect.width() << shiftPerPixel;

    
    pixels += rect.fTop * rowBytes + (rect.fLeft << shiftPerPixel);
    for (int scans = rect.height() - 1; scans >= 0; --scans) {
        proc(pixels, widthBytes, procData);
        pixels += rowBytes;
    }
}

void SkDraw::drawPaint(const SkPaint& paint) const {
    SkDEBUGCODE(this->validate();)

    if (fRC->isEmpty()) {
        return;
    }

    SkIRect    devRect;
    devRect.set(0, 0, fBitmap->width(), fBitmap->height());

    if (fRC->isBW()) {
        





        uint32_t procData = 0;  
        BitmapXferProc proc = ChooseBitmapXferProc(*fBitmap, paint, &procData);
        if (proc) {
            if (D_Dst_BitmapXferProc == proc) { 
                return;
            }

            SkRegion::Iterator iter(fRC->bwRgn());
            while (!iter.done()) {
                CallBitmapXferProc(*fBitmap, iter.rect(), proc, procData);
                iter.next();
            }
            return;
        }
    }

    
    SkAutoBlitterChoose blitter(*fBitmap, *fMatrix, paint);
    SkScan::FillIRect(devRect, *fRC, blitter.get());
}



struct PtProcRec {
    SkCanvas::PointMode fMode;
    const SkPaint*  fPaint;
    const SkRegion* fClip;
    const SkRasterClip* fRC;

    
    SkFixed fRadius;

    typedef void (*Proc)(const PtProcRec&, const SkPoint devPts[], int count,
                         SkBlitter*);

    bool init(SkCanvas::PointMode, const SkPaint&, const SkMatrix* matrix,
              const SkRasterClip*);
    Proc chooseProc(SkBlitter** blitter);

private:
    SkAAClipBlitterWrapper fWrapper;
};

static void bw_pt_rect_hair_proc(const PtProcRec& rec, const SkPoint devPts[],
                                 int count, SkBlitter* blitter) {
    SkASSERT(rec.fClip->isRect());
    const SkIRect& r = rec.fClip->getBounds();

    for (int i = 0; i < count; i++) {
        int x = SkScalarFloorToInt(devPts[i].fX);
        int y = SkScalarFloorToInt(devPts[i].fY);
        if (r.contains(x, y)) {
            blitter->blitH(x, y, 1);
        }
    }
}

static void bw_pt_rect_16_hair_proc(const PtProcRec& rec,
                                    const SkPoint devPts[], int count,
                                    SkBlitter* blitter) {
    SkASSERT(rec.fRC->isRect());
    const SkIRect& r = rec.fRC->getBounds();
    uint32_t value;
    const SkBitmap* bitmap = blitter->justAnOpaqueColor(&value);
    SkASSERT(bitmap);

    uint16_t* addr = bitmap->getAddr16(0, 0);
    size_t    rb = bitmap->rowBytes();

    for (int i = 0; i < count; i++) {
        int x = SkScalarFloorToInt(devPts[i].fX);
        int y = SkScalarFloorToInt(devPts[i].fY);
        if (r.contains(x, y)) {
            ((uint16_t*)((char*)addr + y * rb))[x] = SkToU16(value);
        }
    }
}

static void bw_pt_rect_32_hair_proc(const PtProcRec& rec,
                                    const SkPoint devPts[], int count,
                                    SkBlitter* blitter) {
    SkASSERT(rec.fRC->isRect());
    const SkIRect& r = rec.fRC->getBounds();
    uint32_t value;
    const SkBitmap* bitmap = blitter->justAnOpaqueColor(&value);
    SkASSERT(bitmap);

    SkPMColor* addr = bitmap->getAddr32(0, 0);
    size_t     rb = bitmap->rowBytes();

    for (int i = 0; i < count; i++) {
        int x = SkScalarFloorToInt(devPts[i].fX);
        int y = SkScalarFloorToInt(devPts[i].fY);
        if (r.contains(x, y)) {
            ((SkPMColor*)((char*)addr + y * rb))[x] = value;
        }
    }
}

static void bw_pt_hair_proc(const PtProcRec& rec, const SkPoint devPts[],
                            int count, SkBlitter* blitter) {
    for (int i = 0; i < count; i++) {
        int x = SkScalarFloorToInt(devPts[i].fX);
        int y = SkScalarFloorToInt(devPts[i].fY);
        if (rec.fClip->contains(x, y)) {
            blitter->blitH(x, y, 1);
        }
    }
}

static void bw_line_hair_proc(const PtProcRec& rec, const SkPoint devPts[],
                              int count, SkBlitter* blitter) {
    for (int i = 0; i < count; i += 2) {
        SkScan::HairLine(devPts[i], devPts[i+1], *rec.fRC, blitter);
    }
}

static void bw_poly_hair_proc(const PtProcRec& rec, const SkPoint devPts[],
                              int count, SkBlitter* blitter) {
    for (int i = 0; i < count - 1; i++) {
        SkScan::HairLine(devPts[i], devPts[i+1], *rec.fRC, blitter);
    }
}



static void aa_line_hair_proc(const PtProcRec& rec, const SkPoint devPts[],
                              int count, SkBlitter* blitter) {
    for (int i = 0; i < count; i += 2) {
        SkScan::AntiHairLine(devPts[i], devPts[i+1], *rec.fRC, blitter);
    }
}

static void aa_poly_hair_proc(const PtProcRec& rec, const SkPoint devPts[],
                              int count, SkBlitter* blitter) {
    for (int i = 0; i < count - 1; i++) {
        SkScan::AntiHairLine(devPts[i], devPts[i+1], *rec.fRC, blitter);
    }
}



static void bw_square_proc(const PtProcRec& rec, const SkPoint devPts[],
                           int count, SkBlitter* blitter) {
    const SkFixed radius = rec.fRadius;
    for (int i = 0; i < count; i++) {
        SkFixed x = SkScalarToFixed(devPts[i].fX);
        SkFixed y = SkScalarToFixed(devPts[i].fY);

        SkXRect r;
        r.fLeft = x - radius;
        r.fTop = y - radius;
        r.fRight = x + radius;
        r.fBottom = y + radius;

        SkScan::FillXRect(r, *rec.fRC, blitter);
    }
}

static void aa_square_proc(const PtProcRec& rec, const SkPoint devPts[],
                           int count, SkBlitter* blitter) {
    const SkFixed radius = rec.fRadius;
    for (int i = 0; i < count; i++) {
        SkFixed x = SkScalarToFixed(devPts[i].fX);
        SkFixed y = SkScalarToFixed(devPts[i].fY);

        SkXRect r;
        r.fLeft = x - radius;
        r.fTop = y - radius;
        r.fRight = x + radius;
        r.fBottom = y + radius;

        SkScan::AntiFillXRect(r, *rec.fRC, blitter);
    }
}


bool PtProcRec::init(SkCanvas::PointMode mode, const SkPaint& paint,
                     const SkMatrix* matrix, const SkRasterClip* rc) {
    if (paint.getPathEffect()) {
        return false;
    }
    SkScalar width = paint.getStrokeWidth();
    if (0 == width) {
        fMode = mode;
        fPaint = &paint;
        fClip = NULL;
        fRC = rc;
        fRadius = SK_FixedHalf;
        return true;
    }
    if (paint.getStrokeCap() != SkPaint::kRound_Cap &&
            matrix->rectStaysRect() && SkCanvas::kPoints_PointMode == mode) {
        SkScalar sx = matrix->get(SkMatrix::kMScaleX);
        SkScalar sy = matrix->get(SkMatrix::kMScaleY);
        if (SkScalarNearlyZero(sx - sy)) {
            if (sx < 0) {
                sx = -sx;
            }

            fMode = mode;
            fPaint = &paint;
            fClip = NULL;
            fRC = rc;
            fRadius = SkScalarToFixed(SkScalarMul(width, sx)) >> 1;
            return true;
        }
    }
    return false;
}

PtProcRec::Proc PtProcRec::chooseProc(SkBlitter** blitterPtr) {
    Proc proc = NULL;

    SkBlitter* blitter = *blitterPtr;
    if (fRC->isBW()) {
        fClip = &fRC->bwRgn();
    } else {
        fWrapper.init(*fRC, blitter);
        fClip = &fWrapper.getRgn();
        blitter = fWrapper.getBlitter();
        *blitterPtr = blitter;
    }

    
    SkASSERT(0 == SkCanvas::kPoints_PointMode);
    SkASSERT(1 == SkCanvas::kLines_PointMode);
    SkASSERT(2 == SkCanvas::kPolygon_PointMode);
    SkASSERT((unsigned)fMode <= (unsigned)SkCanvas::kPolygon_PointMode);

    if (fPaint->isAntiAlias()) {
        if (0 == fPaint->getStrokeWidth()) {
            static const Proc gAAProcs[] = {
                aa_square_proc, aa_line_hair_proc, aa_poly_hair_proc
            };
            proc = gAAProcs[fMode];
        } else if (fPaint->getStrokeCap() != SkPaint::kRound_Cap) {
            SkASSERT(SkCanvas::kPoints_PointMode == fMode);
            proc = aa_square_proc;
        }
    } else {    
        if (fRadius <= SK_FixedHalf) {    
            if (SkCanvas::kPoints_PointMode == fMode && fClip->isRect()) {
                uint32_t value;
                const SkBitmap* bm = blitter->justAnOpaqueColor(&value);
                if (bm && kRGB_565_SkColorType == bm->colorType()) {
                    proc = bw_pt_rect_16_hair_proc;
                } else if (bm && kN32_SkColorType == bm->colorType()) {
                    proc = bw_pt_rect_32_hair_proc;
                } else {
                    proc = bw_pt_rect_hair_proc;
                }
            } else {
                static Proc gBWProcs[] = {
                    bw_pt_hair_proc, bw_line_hair_proc, bw_poly_hair_proc
                };
                proc = gBWProcs[fMode];
            }
        } else {
            proc = bw_square_proc;
        }
    }
    return proc;
}



#define MAX_DEV_PTS     32

void SkDraw::drawPoints(SkCanvas::PointMode mode, size_t count,
                        const SkPoint pts[], const SkPaint& paint,
                        bool forceUseDevice) const {
    
    if (SkCanvas::kLines_PointMode == mode) {
        count &= ~(size_t)1;
    }

    if ((long)count <= 0) {
        return;
    }

    SkASSERT(pts != NULL);
    SkDEBUGCODE(this->validate();)

     
    if (fRC->isEmpty()) {
        return;
    }

    PtProcRec rec;
    if (!forceUseDevice && rec.init(mode, paint, fMatrix, fRC)) {
        SkAutoBlitterChoose blitter(*fBitmap, *fMatrix, paint);

        SkPoint             devPts[MAX_DEV_PTS];
        const SkMatrix*     matrix = fMatrix;
        SkBlitter*          bltr = blitter.get();
        PtProcRec::Proc     proc = rec.chooseProc(&bltr);
        
        const size_t backup = (SkCanvas::kPolygon_PointMode == mode);

        do {
            int n = SkToInt(count);
            if (n > MAX_DEV_PTS) {
                n = MAX_DEV_PTS;
            }
            matrix->mapPoints(devPts, pts, n);
            proc(rec, devPts, n, bltr);
            pts += n - backup;
            SkASSERT(SkToInt(count) >= n);
            count -= n;
            if (count > 0) {
                count += backup;
            }
        } while (count != 0);
    } else {
        switch (mode) {
            case SkCanvas::kPoints_PointMode: {
                
                SkPaint newPaint(paint);
                newPaint.setStyle(SkPaint::kFill_Style);

                SkScalar width = newPaint.getStrokeWidth();
                SkScalar radius = SkScalarHalf(width);

                if (newPaint.getStrokeCap() == SkPaint::kRound_Cap) {
                    SkPath      path;
                    SkMatrix    preMatrix;

                    path.addCircle(0, 0, radius);
                    for (size_t i = 0; i < count; i++) {
                        preMatrix.setTranslate(pts[i].fX, pts[i].fY);
                        
                        
                        if (fDevice) {
                            fDevice->drawPath(*this, path, newPaint, &preMatrix,
                                              (count-1) == i);
                        } else {
                            this->drawPath(path, newPaint, &preMatrix,
                                           (count-1) == i);
                        }
                    }
                } else {
                    SkRect  r;

                    for (size_t i = 0; i < count; i++) {
                        r.fLeft = pts[i].fX - radius;
                        r.fTop = pts[i].fY - radius;
                        r.fRight = r.fLeft + width;
                        r.fBottom = r.fTop + width;
                        if (fDevice) {
                            fDevice->drawRect(*this, r, newPaint);
                        } else {
                            this->drawRect(r, newPaint);
                        }
                    }
                }
                break;
            }
            case SkCanvas::kLines_PointMode:
#ifndef SK_DISABLE_DASHING_OPTIMIZATION
                if (2 == count && NULL != paint.getPathEffect()) {
                    
                    
                    SkStrokeRec rec(paint);
                    SkPathEffect::PointData pointData;

                    SkPath path;
                    path.moveTo(pts[0]);
                    path.lineTo(pts[1]);

                    SkRect cullRect = SkRect::Make(fRC->getBounds());

                    if (paint.getPathEffect()->asPoints(&pointData, path, rec,
                                                        *fMatrix, &cullRect)) {
                        

                        SkPaint newP(paint);
                        newP.setPathEffect(NULL);
                        newP.setStyle(SkPaint::kFill_Style);

                        if (!pointData.fFirst.isEmpty()) {
                            if (fDevice) {
                                fDevice->drawPath(*this, pointData.fFirst, newP);
                            } else {
                                this->drawPath(pointData.fFirst, newP);
                            }
                        }

                        if (!pointData.fLast.isEmpty()) {
                            if (fDevice) {
                                fDevice->drawPath(*this, pointData.fLast, newP);
                            } else {
                                this->drawPath(pointData.fLast, newP);
                            }
                        }

                        if (pointData.fSize.fX == pointData.fSize.fY) {
                            
                            SkASSERT(pointData.fSize.fX == SkScalarHalf(newP.getStrokeWidth()));

                            if (SkPathEffect::PointData::kCircles_PointFlag & pointData.fFlags) {
                                newP.setStrokeCap(SkPaint::kRound_Cap);
                            } else {
                                newP.setStrokeCap(SkPaint::kButt_Cap);
                            }

                            if (fDevice) {
                                fDevice->drawPoints(*this,
                                                    SkCanvas::kPoints_PointMode,
                                                    pointData.fNumPoints,
                                                    pointData.fPoints,
                                                    newP);
                            } else {
                                this->drawPoints(SkCanvas::kPoints_PointMode,
                                                 pointData.fNumPoints,
                                                 pointData.fPoints,
                                                 newP,
                                                 forceUseDevice);
                            }
                            break;
                        } else {
                            
                            SkASSERT(!(SkPathEffect::PointData::kCircles_PointFlag &
                                      pointData.fFlags));

                            SkRect r;

                            for (int i = 0; i < pointData.fNumPoints; ++i) {
                                r.set(pointData.fPoints[i].fX - pointData.fSize.fX,
                                      pointData.fPoints[i].fY - pointData.fSize.fY,
                                      pointData.fPoints[i].fX + pointData.fSize.fX,
                                      pointData.fPoints[i].fY + pointData.fSize.fY);
                                if (fDevice) {
                                    fDevice->drawRect(*this, r, newP);
                                } else {
                                    this->drawRect(r, newP);
                                }
                            }
                        }

                        break;
                    }
                }
#endif 
                
            case SkCanvas::kPolygon_PointMode: {
                count -= 1;
                SkPath path;
                SkPaint p(paint);
                p.setStyle(SkPaint::kStroke_Style);
                size_t inc = (SkCanvas::kLines_PointMode == mode) ? 2 : 1;
                for (size_t i = 0; i < count; i += inc) {
                    path.moveTo(pts[i]);
                    path.lineTo(pts[i+1]);
                    if (fDevice) {
                        fDevice->drawPath(*this, path, p, NULL, true);
                    } else {
                        this->drawPath(path, p, NULL, true);
                    }
                    path.rewind();
                }
                break;
            }
        }
    }
}

static bool easy_rect_join(const SkPaint& paint, const SkMatrix& matrix,
                           SkPoint* strokeSize) {
    if (SkPaint::kMiter_Join != paint.getStrokeJoin() ||
        paint.getStrokeMiter() < SK_ScalarSqrt2) {
        return false;
    }

    SkASSERT(matrix.rectStaysRect());
    SkPoint pt = { paint.getStrokeWidth(), paint.getStrokeWidth() };
    matrix.mapVectors(strokeSize, &pt, 1);
    strokeSize->fX = SkScalarAbs(strokeSize->fX);
    strokeSize->fY = SkScalarAbs(strokeSize->fY);
    return true;
}

SkDraw::RectType SkDraw::ComputeRectType(const SkPaint& paint,
                                         const SkMatrix& matrix,
                                         SkPoint* strokeSize) {
    RectType rtype;
    const SkScalar width = paint.getStrokeWidth();
    const bool zeroWidth = (0 == width);
    SkPaint::Style style = paint.getStyle();

    if ((SkPaint::kStrokeAndFill_Style == style) && zeroWidth) {
        style = SkPaint::kFill_Style;
    }

    if (paint.getPathEffect() || paint.getMaskFilter() ||
        paint.getRasterizer() || !matrix.rectStaysRect() ||
        SkPaint::kStrokeAndFill_Style == style) {
        rtype = kPath_RectType;
    } else if (SkPaint::kFill_Style == style) {
        rtype = kFill_RectType;
    } else if (zeroWidth) {
        rtype = kHair_RectType;
    } else if (easy_rect_join(paint, matrix, strokeSize)) {
        rtype = kStroke_RectType;
    } else {
        rtype = kPath_RectType;
    }
    return rtype;
}

static const SkPoint* rect_points(const SkRect& r) {
    return SkTCast<const SkPoint*>(&r);
}

static SkPoint* rect_points(SkRect& r) {
    return SkTCast<SkPoint*>(&r);
}

void SkDraw::drawRect(const SkRect& rect, const SkPaint& paint) const {
    SkDEBUGCODE(this->validate();)

    
    if (fRC->isEmpty()) {
        return;
    }

    SkPoint strokeSize;
    RectType rtype = ComputeRectType(paint, *fMatrix, &strokeSize);

    if (kPath_RectType == rtype) {
        SkPath  tmp;
        tmp.addRect(rect);
        tmp.setFillType(SkPath::kWinding_FillType);
        this->drawPath(tmp, paint, NULL, true);
        return;
    }

    const SkMatrix& matrix = *fMatrix;
    SkRect          devRect;

    
    matrix.mapPoints(rect_points(devRect), rect_points(rect), 2);
    devRect.sort();

    
    SkIRect ir;
    devRect.roundOut(&ir);
    if (paint.getStyle() != SkPaint::kFill_Style) {
        
        ir.inset(-1, -1);
    }
    if (fRC->quickReject(ir)) {
        return;
    }

    SkDeviceLooper looper(*fBitmap, *fRC, ir, paint.isAntiAlias());
    while (looper.next()) {
        SkRect localDevRect;
        looper.mapRect(&localDevRect, devRect);
        SkMatrix localMatrix;
        looper.mapMatrix(&localMatrix, matrix);

        SkAutoBlitterChoose blitterStorage(looper.getBitmap(), localMatrix,
                                           paint);
        const SkRasterClip& clip = looper.getRC();
        SkBlitter*          blitter = blitterStorage.get();

        
        
        
        switch (rtype) {
            case kFill_RectType:
                if (paint.isAntiAlias()) {
                    SkScan::AntiFillRect(localDevRect, clip, blitter);
                } else {
                    SkScan::FillRect(localDevRect, clip, blitter);
                }
                break;
            case kStroke_RectType:
                if (paint.isAntiAlias()) {
                    SkScan::AntiFrameRect(localDevRect, strokeSize, clip, blitter);
                } else {
                    SkScan::FrameRect(localDevRect, strokeSize, clip, blitter);
                }
                break;
            case kHair_RectType:
                if (paint.isAntiAlias()) {
                    SkScan::AntiHairRect(localDevRect, clip, blitter);
                } else {
                    SkScan::HairRect(localDevRect, clip, blitter);
                }
                break;
            default:
                SkDEBUGFAIL("bad rtype");
        }
    }
}

void SkDraw::drawDevMask(const SkMask& srcM, const SkPaint& paint) const {
    if (srcM.fBounds.isEmpty()) {
        return;
    }

    const SkMask* mask = &srcM;

    SkMask dstM;
    if (paint.getMaskFilter() &&
            paint.getMaskFilter()->filterMask(&dstM, srcM, *fMatrix, NULL)) {
        mask = &dstM;
    } else {
        dstM.fImage = NULL;
    }
    SkAutoMaskFreeImage ami(dstM.fImage);

    SkAutoBlitterChoose blitterChooser(*fBitmap, *fMatrix, paint);
    SkBlitter* blitter = blitterChooser.get();

    SkAAClipBlitterWrapper wrapper;
    const SkRegion* clipRgn;

    if (fRC->isBW()) {
        clipRgn = &fRC->bwRgn();
    } else {
        wrapper.init(*fRC, blitter);
        clipRgn = &wrapper.getRgn();
        blitter = wrapper.getBlitter();
    }
    blitter->blitMaskRegion(*mask, *clipRgn);
}

static SkScalar fast_len(const SkVector& vec) {
    SkScalar x = SkScalarAbs(vec.fX);
    SkScalar y = SkScalarAbs(vec.fY);
    if (x < y) {
        SkTSwap(x, y);
    }
    return x + SkScalarHalf(y);
}

static bool xfermodeSupportsCoverageAsAlpha(SkXfermode* xfer) {
    SkXfermode::Coeff dc;
    if (!SkXfermode::AsCoeff(xfer, NULL, &dc)) {
        return false;
    }

    switch (dc) {
        case SkXfermode::kOne_Coeff:
        case SkXfermode::kISA_Coeff:
        case SkXfermode::kISC_Coeff:
            return true;
        default:
            return false;
    }
}

bool SkDrawTreatAAStrokeAsHairline(SkScalar strokeWidth, const SkMatrix& matrix,
                                   SkScalar* coverage) {
    SkASSERT(strokeWidth > 0);
    

    if (matrix.hasPerspective()) {
        return false;
    }

    SkVector src[2], dst[2];
    src[0].set(strokeWidth, 0);
    src[1].set(0, strokeWidth);
    matrix.mapVectors(dst, src, 2);
    SkScalar len0 = fast_len(dst[0]);
    SkScalar len1 = fast_len(dst[1]);
    if (len0 <= SK_Scalar1 && len1 <= SK_Scalar1) {
        if (NULL != coverage) {
            *coverage = SkScalarAve(len0, len1);
        }
        return true;
    }
    return false;
}

void SkDraw::drawRRect(const SkRRect& rrect, const SkPaint& paint) const {
    SkDEBUGCODE(this->validate());

    if (fRC->isEmpty()) {
        return;
    }

    {
        
        
        
        SkScalar coverage;
        if (SkDrawTreatAsHairline(paint, *fMatrix, &coverage)) {
            goto DRAW_PATH;
        }

        if (paint.getPathEffect() || paint.getStyle() != SkPaint::kFill_Style) {
            goto DRAW_PATH;
        }

        if (paint.getRasterizer()) {
            goto DRAW_PATH;
        }
    }

    if (paint.getMaskFilter()) {
        
        SkRRect devRRect;
        if (rrect.transform(*fMatrix, &devRRect)) {
            SkAutoBlitterChoose blitter(*fBitmap, *fMatrix, paint);
            if (paint.getMaskFilter()->filterRRect(devRRect, *fMatrix, *fRC, blitter.get(),
                                                   SkPaint::kFill_Style)) {
                return; 
            }
        }
    }

DRAW_PATH:
    
    SkPath path;
    path.addRRect(rrect);
    this->drawPath(path, paint, NULL, true);
}

void SkDraw::drawPath(const SkPath& origSrcPath, const SkPaint& origPaint,
                      const SkMatrix* prePathMatrix, bool pathIsMutable,
                      bool drawCoverage) const {
    SkDEBUGCODE(this->validate();)

    
    if (fRC->isEmpty()) {
        return;
    }

    SkPath*         pathPtr = (SkPath*)&origSrcPath;
    bool            doFill = true;
    SkPath          tmpPath;
    SkMatrix        tmpMatrix;
    const SkMatrix* matrix = fMatrix;

    if (prePathMatrix) {
        if (origPaint.getPathEffect() || origPaint.getStyle() != SkPaint::kFill_Style ||
                origPaint.getRasterizer()) {
            SkPath* result = pathPtr;

            if (!pathIsMutable) {
                result = &tmpPath;
                pathIsMutable = true;
            }
            pathPtr->transform(*prePathMatrix, result);
            pathPtr = result;
        } else {
            tmpMatrix.setConcat(*matrix, *prePathMatrix);
            matrix = &tmpMatrix;
        }
    }
    
    SkDEBUGCODE(prePathMatrix = (const SkMatrix*)0x50FF8001;)

    SkTCopyOnFirstWrite<SkPaint> paint(origPaint);

    {
        SkScalar coverage;
        if (SkDrawTreatAsHairline(origPaint, *matrix, &coverage)) {
            if (SK_Scalar1 == coverage) {
                paint.writable()->setStrokeWidth(0);
            } else if (xfermodeSupportsCoverageAsAlpha(origPaint.getXfermode())) {
                U8CPU newAlpha;
#if 0
                newAlpha = SkToU8(SkScalarRoundToInt(coverage *
                                                     origPaint.getAlpha()));
#else
                
                
                
                int scale = (int)SkScalarMul(coverage, 256);
                newAlpha = origPaint.getAlpha() * scale >> 8;
#endif
                SkPaint* writablePaint = paint.writable();
                writablePaint->setStrokeWidth(0);
                writablePaint->setAlpha(newAlpha);
            }
        }
    }

    if (paint->getPathEffect() || paint->getStyle() != SkPaint::kFill_Style) {
        SkRect cullRect;
        const SkRect* cullRectPtr = NULL;
        if (this->computeConservativeLocalClipBounds(&cullRect)) {
            cullRectPtr = &cullRect;
        }
        doFill = paint->getFillPath(*pathPtr, &tmpPath, cullRectPtr);
        pathPtr = &tmpPath;
    }

    if (paint->getRasterizer()) {
        SkMask  mask;
        if (paint->getRasterizer()->rasterize(*pathPtr, *matrix,
                            &fRC->getBounds(), paint->getMaskFilter(), &mask,
                            SkMask::kComputeBoundsAndRenderImage_CreateMode)) {
            this->drawDevMask(mask, *paint);
            SkMask::FreeImage(mask.fImage);
        }
        return;
    }

    
    SkPath* devPathPtr = pathIsMutable ? pathPtr : &tmpPath;

    
    pathPtr->transform(*matrix, devPathPtr);

    SkAutoBlitterChoose blitter(*fBitmap, *fMatrix, *paint, drawCoverage);

    if (paint->getMaskFilter()) {
        SkPaint::Style style = doFill ? SkPaint::kFill_Style :
            SkPaint::kStroke_Style;
        if (paint->getMaskFilter()->filterPath(*devPathPtr, *fMatrix, *fRC, blitter.get(), style)) {
            return; 
        }
    }

    void (*proc)(const SkPath&, const SkRasterClip&, SkBlitter*);
    if (doFill) {
        if (paint->isAntiAlias()) {
            proc = SkScan::AntiFillPath;
        } else {
            proc = SkScan::FillPath;
        }
    } else {    
        if (paint->isAntiAlias()) {
            proc = SkScan::AntiHairPath;
        } else {
            proc = SkScan::HairPath;
        }
    }
    proc(*devPathPtr, *fRC, blitter.get());
}




static bool just_translate(const SkMatrix& matrix, const SkBitmap& bitmap) {
    unsigned bits = 0;  
                        
    return SkTreatAsSprite(matrix, bitmap.width(), bitmap.height(), bits);
}

void SkDraw::drawBitmapAsMask(const SkBitmap& bitmap,
                              const SkPaint& paint) const {
    SkASSERT(bitmap.colorType() == kAlpha_8_SkColorType);

    if (just_translate(*fMatrix, bitmap)) {
        int ix = SkScalarRoundToInt(fMatrix->getTranslateX());
        int iy = SkScalarRoundToInt(fMatrix->getTranslateY());

        SkAutoLockPixels alp(bitmap);
        if (!bitmap.readyToDraw()) {
            return;
        }

        SkMask  mask;
        mask.fBounds.set(ix, iy, ix + bitmap.width(), iy + bitmap.height());
        mask.fFormat = SkMask::kA8_Format;
        mask.fRowBytes = SkToU32(bitmap.rowBytes());
        mask.fImage = bitmap.getAddr8(0, 0);

        this->drawDevMask(mask, paint);
    } else {    
        SkRect  r;
        SkMask  mask;

        r.set(0, 0,
              SkIntToScalar(bitmap.width()), SkIntToScalar(bitmap.height()));
        fMatrix->mapRect(&r);
        r.round(&mask.fBounds);

        
        
        {
            SkIRect    devBounds;
            devBounds.set(0, 0, fBitmap->width(), fBitmap->height());
            
            if (!mask.fBounds.intersect(devBounds)) {
                return;
            }
        }

        mask.fFormat = SkMask::kA8_Format;
        mask.fRowBytes = SkAlign4(mask.fBounds.width());
        size_t size = mask.computeImageSize();
        if (0 == size) {
            
            return;
        }

        
        SkAutoMalloc    storage(size);
        mask.fImage = (uint8_t*)storage.get();
        memset(mask.fImage, 0, size);

        
        {
            SkBitmap    device;
            device.installPixels(SkImageInfo::MakeA8(mask.fBounds.width(), mask.fBounds.height()),
                                 mask.fImage, mask.fRowBytes);

            SkCanvas c(device);
            
            c.translate(-SkIntToScalar(mask.fBounds.fLeft),
                        -SkIntToScalar(mask.fBounds.fTop));
            c.concat(*fMatrix);

            
            
            SkPaint tmpPaint;
            tmpPaint.setFlags(paint.getFlags());
            SkAutoBitmapShaderInstall install(bitmap, tmpPaint);
            SkRect rr;
            rr.set(0, 0, SkIntToScalar(bitmap.width()),
                   SkIntToScalar(bitmap.height()));
            c.drawRect(rr, install.paintWithShader());
        }
        this->drawDevMask(mask, paint);
    }
}

static bool clipped_out(const SkMatrix& m, const SkRasterClip& c,
                        const SkRect& srcR) {
    SkRect  dstR;
    SkIRect devIR;

    m.mapRect(&dstR, srcR);
    dstR.roundOut(&devIR);
    return c.quickReject(devIR);
}

static bool clipped_out(const SkMatrix& matrix, const SkRasterClip& clip,
                        int width, int height) {
    SkRect  r;
    r.set(0, 0, SkIntToScalar(width), SkIntToScalar(height));
    return clipped_out(matrix, clip, r);
}

static bool clipHandlesSprite(const SkRasterClip& clip, int x, int y,
                              const SkBitmap& bitmap) {
    return clip.isBW() ||
           clip.quickContains(x, y, x + bitmap.width(), y + bitmap.height());
}

void SkDraw::drawBitmap(const SkBitmap& bitmap, const SkMatrix& prematrix,
                        const SkPaint& origPaint) const {
    SkDEBUGCODE(this->validate();)

    
    if (fRC->isEmpty() ||
            bitmap.width() == 0 || bitmap.height() == 0 ||
            bitmap.colorType() == kUnknown_SkColorType) {
        return;
    }

    SkPaint paint(origPaint);
    paint.setStyle(SkPaint::kFill_Style);

    SkMatrix matrix;
    matrix.setConcat(*fMatrix, prematrix);

    if (clipped_out(matrix, *fRC, bitmap.width(), bitmap.height())) {
        return;
    }

    if (bitmap.colorType() != kAlpha_8_SkColorType && just_translate(matrix, bitmap)) {
        
        
        
        
        SkAutoLockPixels alp(bitmap);
        if (!bitmap.readyToDraw()) {
            return;
        }
        int ix = SkScalarRoundToInt(matrix.getTranslateX());
        int iy = SkScalarRoundToInt(matrix.getTranslateY());
        if (clipHandlesSprite(*fRC, ix, iy, bitmap)) {
            SkTBlitterAllocator allocator;
            
            SkBlitter* blitter = SkBlitter::ChooseSprite(*fBitmap, paint, bitmap,
                                                         ix, iy, &allocator);
            if (blitter) {
                SkIRect    ir;
                ir.set(ix, iy, ix + bitmap.width(), iy + bitmap.height());

                SkScan::FillIRect(ir, *fRC, blitter);
                return;
            }
        }
    }

    
    
    SkDraw draw(*this);
    draw.fMatrix = &matrix;

    if (bitmap.colorType() == kAlpha_8_SkColorType) {
        draw.drawBitmapAsMask(bitmap, paint);
    } else {
        SkAutoBitmapShaderInstall install(bitmap, paint);

        SkRect  r;
        r.set(0, 0, SkIntToScalar(bitmap.width()),
              SkIntToScalar(bitmap.height()));
        
        draw.drawRect(r, install.paintWithShader());
    }
}

void SkDraw::drawSprite(const SkBitmap& bitmap, int x, int y,
                        const SkPaint& origPaint) const {
    SkDEBUGCODE(this->validate();)

    
    if (fRC->isEmpty() ||
            bitmap.width() == 0 || bitmap.height() == 0 ||
            bitmap.colorType() == kUnknown_SkColorType) {
        return;
    }

    SkIRect    bounds;
    bounds.set(x, y, x + bitmap.width(), y + bitmap.height());

    if (fRC->quickReject(bounds)) {
        return; 
    }

    SkPaint paint(origPaint);
    paint.setStyle(SkPaint::kFill_Style);

    if (NULL == paint.getColorFilter() && clipHandlesSprite(*fRC, x, y, bitmap)) {
        SkTBlitterAllocator allocator;
        
        SkBlitter* blitter = SkBlitter::ChooseSprite(*fBitmap, paint, bitmap,
                                                     x, y, &allocator);

        if (blitter) {
            SkScan::FillIRect(bounds, *fRC, blitter);
            return;
        }
    }

    SkMatrix        matrix;
    SkRect          r;

    
    r.set(bounds);

    
    matrix.setTranslate(r.fLeft, r.fTop);
    SkAutoBitmapShaderInstall install(bitmap, paint, &matrix);
    const SkPaint& shaderPaint = install.paintWithShader();

    SkDraw draw(*this);
    matrix.reset();
    draw.fMatrix = &matrix;
    
    
    draw.drawRect(r, shaderPaint);
}



#include "SkScalerContext.h"
#include "SkGlyphCache.h"
#include "SkTextToPathIter.h"
#include "SkUtils.h"

static void measure_text(SkGlyphCache* cache, SkDrawCacheProc glyphCacheProc,
                const char text[], size_t byteLength, SkVector* stopVector) {
    SkFixed     x = 0, y = 0;
    const char* stop = text + byteLength;

    SkAutoKern  autokern;

    while (text < stop) {
        
        
        const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

        x += autokern.adjust(glyph) + glyph.fAdvanceX;
        y += glyph.fAdvanceY;
    }
    stopVector->set(SkFixedToScalar(x), SkFixedToScalar(y));

    SkASSERT(text == stop);
}

bool SkDraw::ShouldDrawTextAsPaths(const SkPaint& paint, const SkMatrix& ctm) {
    
    if (SkPaint::kStroke_Style == paint.getStyle() && 0 == paint.getStrokeWidth()) {
        return true;
    }

    
    if (ctm.hasPerspective()) {
        return true;
    }

    SkMatrix textM;
    return SkPaint::TooBigToUseCache(ctm, *paint.setTextMatrix(&textM));
}

void SkDraw::drawText_asPaths(const char text[], size_t byteLength,
                              SkScalar x, SkScalar y,
                              const SkPaint& paint) const {
    SkDEBUGCODE(this->validate();)

    SkTextToPathIter iter(text, byteLength, paint, true);

    SkMatrix    matrix;
    matrix.setScale(iter.getPathScale(), iter.getPathScale());
    matrix.postTranslate(x, y);

    const SkPath* iterPath;
    SkScalar xpos, prevXPos = 0;

    while (iter.next(&iterPath, &xpos)) {
        matrix.postTranslate(xpos - prevXPos, 0);
        if (iterPath) {
            const SkPaint& pnt = iter.getPaint();
            if (fDevice) {
                fDevice->drawPath(*this, *iterPath, pnt, &matrix, false);
            } else {
                this->drawPath(*iterPath, pnt, &matrix, false);
            }
        }
        prevXPos = xpos;
    }
}


#if defined _WIN32 && _MSC_VER >= 1300
#pragma warning ( push )
#pragma warning ( disable : 4701 )
#endif



static void D1G_RectClip(const SkDraw1Glyph& state, SkFixed fx, SkFixed fy, const SkGlyph& glyph) {
    int left = SkFixedFloorToInt(fx);
    int top = SkFixedFloorToInt(fy);
    SkASSERT(glyph.fWidth > 0 && glyph.fHeight > 0);
    SkASSERT((NULL == state.fClip && state.fAAClip) ||
             (state.fClip && NULL == state.fAAClip && state.fClip->isRect()));

    left += glyph.fLeft;
    top  += glyph.fTop;

    int right   = left + glyph.fWidth;
    int bottom  = top + glyph.fHeight;

    SkMask        mask;
    SkIRect        storage;
    SkIRect*    bounds = &mask.fBounds;

    mask.fBounds.set(left, top, right, bottom);

    
    
    if (!state.fClipBounds.containsNoEmptyCheck(left, top, right, bottom)) {
        if (!storage.intersectNoEmptyCheck(mask.fBounds, state.fClipBounds))
            return;
        bounds = &storage;
    }

    uint8_t* aa = (uint8_t*)glyph.fImage;
    if (NULL == aa) {
        aa = (uint8_t*)state.fCache->findImage(glyph);
        if (NULL == aa) {
            return; 
        }
    }

    mask.fRowBytes = glyph.rowBytes();
    mask.fFormat = static_cast<SkMask::Format>(glyph.fMaskFormat);
    mask.fImage = aa;
    state.blitMask(mask, *bounds);
}

static void D1G_RgnClip(const SkDraw1Glyph& state, SkFixed fx, SkFixed fy, const SkGlyph& glyph) {
    int left = SkFixedFloorToInt(fx);
    int top = SkFixedFloorToInt(fy);
    SkASSERT(glyph.fWidth > 0 && glyph.fHeight > 0);
    SkASSERT(!state.fClip->isRect());

    SkMask  mask;

    left += glyph.fLeft;
    top  += glyph.fTop;

    mask.fBounds.set(left, top, left + glyph.fWidth, top + glyph.fHeight);
    SkRegion::Cliperator clipper(*state.fClip, mask.fBounds);

    if (!clipper.done()) {
        const SkIRect&  cr = clipper.rect();
        const uint8_t*  aa = (const uint8_t*)glyph.fImage;
        if (NULL == aa) {
            aa = (uint8_t*)state.fCache->findImage(glyph);
            if (NULL == aa) {
                return;
            }
        }

        mask.fRowBytes = glyph.rowBytes();
        mask.fFormat = static_cast<SkMask::Format>(glyph.fMaskFormat);
        mask.fImage = (uint8_t*)aa;
        do {
            state.blitMask(mask, cr);
            clipper.next();
        } while (!clipper.done());
    }
}

static bool hasCustomD1GProc(const SkDraw& draw) {
    return draw.fProcs && draw.fProcs->fD1GProc;
}

static bool needsRasterTextBlit(const SkDraw& draw) {
    return !hasCustomD1GProc(draw);
}

SkDraw1Glyph::Proc SkDraw1Glyph::init(const SkDraw* draw, SkBlitter* blitter, SkGlyphCache* cache,
                                      const SkPaint& pnt) {
    fDraw = draw;
    fBlitter = blitter;
    fCache = cache;
    fPaint = &pnt;

    if (cache->isSubpixel()) {
        fHalfSampleX = fHalfSampleY = (SK_FixedHalf >> SkGlyph::kSubBits);
    } else {
        fHalfSampleX = fHalfSampleY = SK_FixedHalf;
    }

    if (hasCustomD1GProc(*draw)) {
        
        fClip = draw->fClip;
        fClipBounds = fClip->getBounds();
        return draw->fProcs->fD1GProc;
    }

    if (draw->fRC->isBW()) {
        fAAClip = NULL;
        fClip = &draw->fRC->bwRgn();
        fClipBounds = fClip->getBounds();
        if (fClip->isRect()) {
            return D1G_RectClip;
        } else {
            return D1G_RgnClip;
        }
    } else {    
        fAAClip = &draw->fRC->aaRgn();
        fClip = NULL;
        fClipBounds = fAAClip->getBounds();
        return D1G_RectClip;
    }
}

void SkDraw1Glyph::blitMaskAsSprite(const SkMask& mask) const {
    SkASSERT(SkMask::kARGB32_Format == mask.fFormat);

    SkBitmap bm;
    bm.installPixels(SkImageInfo::MakeN32Premul(mask.fBounds.width(), mask.fBounds.height()),
                     (SkPMColor*)mask.fImage, mask.fRowBytes);

    fDraw->drawSprite(bm, mask.fBounds.x(), mask.fBounds.y(), *fPaint);
}



void SkDraw::drawText(const char text[], size_t byteLength,
                      SkScalar x, SkScalar y, const SkPaint& paint) const {
    SkASSERT(byteLength == 0 || text != NULL);

    SkDEBUGCODE(this->validate();)

    
    if (text == NULL || byteLength == 0 || fRC->isEmpty()) {
        return;
    }

    
    
    if (ShouldDrawTextAsPaths(paint, *fMatrix)) {
        this->drawText_asPaths(text, byteLength, x, y, paint);
        return;
    }

    SkDrawCacheProc glyphCacheProc = paint.getDrawCacheProc();

    SkAutoGlyphCache    autoCache(paint, &fDevice->fLeakyProperties, fMatrix);
    SkGlyphCache*       cache = autoCache.getCache();

    
    {
        SkPoint loc;
        fMatrix->mapXY(x, y, &loc);
        x = loc.fX;
        y = loc.fY;
    }

    
    if (paint.getTextAlign() != SkPaint::kLeft_Align) {
        SkVector    stop;

        measure_text(cache, glyphCacheProc, text, byteLength, &stop);

        SkScalar    stopX = stop.fX;
        SkScalar    stopY = stop.fY;

        if (paint.getTextAlign() == SkPaint::kCenter_Align) {
            stopX = SkScalarHalf(stopX);
            stopY = SkScalarHalf(stopY);
        }
        x -= stopX;
        y -= stopY;
    }

    const char* stop = text + byteLength;

    SkAAClipBlitter     aaBlitter;
    SkAutoBlitterChoose blitterChooser;
    SkBlitter*          blitter = NULL;
    if (needsRasterTextBlit(*this)) {
        blitterChooser.choose(*fBitmap, *fMatrix, paint);
        blitter = blitterChooser.get();
        if (fRC->isAA()) {
            aaBlitter.init(blitter, &fRC->aaRgn());
            blitter = &aaBlitter;
        }
    }

    SkAutoKern          autokern;
    SkDraw1Glyph        d1g;
    SkDraw1Glyph::Proc  proc = d1g.init(this, blitter, cache, paint);

    SkFixed fxMask = ~0;
    SkFixed fyMask = ~0;
    if (cache->isSubpixel()) {
        SkAxisAlignment baseline = SkComputeAxisAlignmentForHText(*fMatrix);
        if (kX_SkAxisAlignment == baseline) {
            fyMask = 0;
            d1g.fHalfSampleY = SK_FixedHalf;
        } else if (kY_SkAxisAlignment == baseline) {
            fxMask = 0;
            d1g.fHalfSampleX = SK_FixedHalf;
        }
    }

    SkFixed fx = SkScalarToFixed(x) + d1g.fHalfSampleX;
    SkFixed fy = SkScalarToFixed(y) + d1g.fHalfSampleY;

    while (text < stop) {
        const SkGlyph& glyph = glyphCacheProc(cache, &text, fx & fxMask, fy & fyMask);

        fx += autokern.adjust(glyph);

        if (glyph.fWidth) {
            proc(d1g, fx, fy, glyph);
        }

        fx += glyph.fAdvanceX;
        fy += glyph.fAdvanceY;
    }
}



void SkDraw::drawPosText_asPaths(const char text[], size_t byteLength,
                                 const SkScalar pos[], SkScalar constY,
                                 int scalarsPerPosition,
                                 const SkPaint& origPaint) const {
    
    SkPaint paint(origPaint);
    SkScalar matrixScale = paint.setupForAsPaths();

    SkMatrix matrix;
    matrix.setScale(matrixScale, matrixScale);

    
    paint.setStyle(SkPaint::kFill_Style);
    paint.setPathEffect(NULL);

    SkDrawCacheProc     glyphCacheProc = paint.getDrawCacheProc();
    SkAutoGlyphCache    autoCache(paint, NULL, NULL);
    SkGlyphCache*       cache = autoCache.getCache();

    const char*        stop = text + byteLength;
    SkTextAlignProcScalar alignProc(paint.getTextAlign());
    SkTextMapStateProc tmsProc(SkMatrix::I(), constY, scalarsPerPosition);

    
    paint.setStyle(origPaint.getStyle());
    paint.setPathEffect(origPaint.getPathEffect());

    while (text < stop) {
        const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);
        if (glyph.fWidth) {
            const SkPath* path = cache->findPath(glyph);
            if (path) {
                SkPoint tmsLoc;
                tmsProc(pos, &tmsLoc);
                SkPoint loc;
                alignProc(tmsLoc, glyph, &loc);

                matrix[SkMatrix::kMTransX] = loc.fX;
                matrix[SkMatrix::kMTransY] = loc.fY;
                if (fDevice) {
                    fDevice->drawPath(*this, *path, paint, &matrix, false);
                } else {
                    this->drawPath(*path, paint, &matrix, false);
                }
            }
        }
        pos += scalarsPerPosition;
    }
}

void SkDraw::drawPosText(const char text[], size_t byteLength,
                         const SkScalar pos[], SkScalar constY,
                         int scalarsPerPosition, const SkPaint& paint) const {
    SkASSERT(byteLength == 0 || text != NULL);
    SkASSERT(1 == scalarsPerPosition || 2 == scalarsPerPosition);

    SkDEBUGCODE(this->validate();)

    
    if (text == NULL || byteLength == 0 || fRC->isEmpty()) {
        return;
    }

    if (ShouldDrawTextAsPaths(paint, *fMatrix)) {
        this->drawPosText_asPaths(text, byteLength, pos, constY,
                                  scalarsPerPosition, paint);
        return;
    }

    SkDrawCacheProc     glyphCacheProc = paint.getDrawCacheProc();
    SkAutoGlyphCache    autoCache(paint, &fDevice->fLeakyProperties, fMatrix);
    SkGlyphCache*       cache = autoCache.getCache();

    SkAAClipBlitterWrapper wrapper;
    SkAutoBlitterChoose blitterChooser;
    SkBlitter* blitter = NULL;
    if (needsRasterTextBlit(*this)) {
        blitterChooser.choose(*fBitmap, *fMatrix, paint);
        blitter = blitterChooser.get();
        if (fRC->isAA()) {
            wrapper.init(*fRC, blitter);
            blitter = wrapper.getBlitter();
        }
    }

    const char*        stop = text + byteLength;
    SkTextAlignProc    alignProc(paint.getTextAlign());
    SkDraw1Glyph       d1g;
    SkDraw1Glyph::Proc proc = d1g.init(this, blitter, cache, paint);
    SkTextMapStateProc tmsProc(*fMatrix, constY, scalarsPerPosition);

    if (cache->isSubpixel()) {
        
        SkAxisAlignment baseline = SkComputeAxisAlignmentForHText(*fMatrix);

        SkFixed fxMask = ~0;
        SkFixed fyMask = ~0;
        if (kX_SkAxisAlignment == baseline) {
            fyMask = 0;
#ifndef SK_IGNORE_SUBPIXEL_AXIS_ALIGN_FIX
            d1g.fHalfSampleY = SK_FixedHalf;
#endif
        } else if (kY_SkAxisAlignment == baseline) {
            fxMask = 0;
#ifndef SK_IGNORE_SUBPIXEL_AXIS_ALIGN_FIX
            d1g.fHalfSampleX = SK_FixedHalf;
#endif
        }

        if (SkPaint::kLeft_Align == paint.getTextAlign()) {
            while (text < stop) {
                SkPoint tmsLoc;
                tmsProc(pos, &tmsLoc);
                SkFixed fx = SkScalarToFixed(tmsLoc.fX) + d1g.fHalfSampleX;
                SkFixed fy = SkScalarToFixed(tmsLoc.fY) + d1g.fHalfSampleY;

                const SkGlyph& glyph = glyphCacheProc(cache, &text,
                                                      fx & fxMask, fy & fyMask);

                if (glyph.fWidth) {
                    proc(d1g, fx, fy, glyph);
                }
                pos += scalarsPerPosition;
            }
        } else {
            while (text < stop) {
                const char* currentText = text;
                const SkGlyph& metricGlyph = glyphCacheProc(cache, &text, 0, 0);

                if (metricGlyph.fWidth) {
                    SkDEBUGCODE(SkFixed prevAdvX = metricGlyph.fAdvanceX;)
                    SkDEBUGCODE(SkFixed prevAdvY = metricGlyph.fAdvanceY;)
                    SkPoint tmsLoc;
                    tmsProc(pos, &tmsLoc);
                    SkIPoint fixedLoc;
                    alignProc(tmsLoc, metricGlyph, &fixedLoc);

                    SkFixed fx = fixedLoc.fX + d1g.fHalfSampleX;
                    SkFixed fy = fixedLoc.fY + d1g.fHalfSampleY;

                    
                    const SkGlyph& glyph = glyphCacheProc(cache, &currentText,
                                                          fx & fxMask, fy & fyMask);
                    
                    SkASSERT(prevAdvX == glyph.fAdvanceX);
                    SkASSERT(prevAdvY == glyph.fAdvanceY);
                    SkASSERT(glyph.fWidth);

                    proc(d1g, fx, fy, glyph);
                }
                pos += scalarsPerPosition;
            }
        }
    } else {    
        if (SkPaint::kLeft_Align == paint.getTextAlign()) {
            while (text < stop) {
                
                const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

                if (glyph.fWidth) {
                    SkPoint tmsLoc;
                    tmsProc(pos, &tmsLoc);

                    proc(d1g,
                         SkScalarToFixed(tmsLoc.fX) + SK_FixedHalf, 
                         SkScalarToFixed(tmsLoc.fY) + SK_FixedHalf, 
                         glyph);
                }
                pos += scalarsPerPosition;
            }
        } else {
            while (text < stop) {
                
                const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

                if (glyph.fWidth) {
                    SkPoint tmsLoc;
                    tmsProc(pos, &tmsLoc);

                    SkIPoint fixedLoc;
                    alignProc(tmsLoc, glyph, &fixedLoc);

                    proc(d1g,
                         fixedLoc.fX + SK_FixedHalf, 
                         fixedLoc.fY + SK_FixedHalf, 
                         glyph);
                }
                pos += scalarsPerPosition;
            }
        }
    }
}

#if defined _WIN32 && _MSC_VER >= 1300
#pragma warning ( pop )
#endif



#include "SkPathMeasure.h"

static void morphpoints(SkPoint dst[], const SkPoint src[], int count,
                        SkPathMeasure& meas, const SkMatrix& matrix) {
    SkMatrix::MapXYProc proc = matrix.getMapXYProc();

    for (int i = 0; i < count; i++) {
        SkPoint pos;
        SkVector tangent;

        proc(matrix, src[i].fX, src[i].fY, &pos);
        SkScalar sx = pos.fX;
        SkScalar sy = pos.fY;

        if (!meas.getPosTan(sx, &pos, &tangent)) {
            
            tangent.set(0, 0);
        }

        









        dst[i].set(pos.fX - SkScalarMul(tangent.fY, sy),
                   pos.fY + SkScalarMul(tangent.fX, sy));
    }
}







static void morphpath(SkPath* dst, const SkPath& src, SkPathMeasure& meas,
                      const SkMatrix& matrix) {
    SkPath::Iter    iter(src, false);
    SkPoint         srcP[4], dstP[3];
    SkPath::Verb    verb;

    while ((verb = iter.next(srcP)) != SkPath::kDone_Verb) {
        switch (verb) {
            case SkPath::kMove_Verb:
                morphpoints(dstP, srcP, 1, meas, matrix);
                dst->moveTo(dstP[0]);
                break;
            case SkPath::kLine_Verb:
                
                srcP[0].fX = SkScalarAve(srcP[0].fX, srcP[1].fX);
                srcP[0].fY = SkScalarAve(srcP[0].fY, srcP[1].fY);
                morphpoints(dstP, srcP, 2, meas, matrix);
                dst->quadTo(dstP[0], dstP[1]);
                break;
            case SkPath::kQuad_Verb:
                morphpoints(dstP, &srcP[1], 2, meas, matrix);
                dst->quadTo(dstP[0], dstP[1]);
                break;
            case SkPath::kCubic_Verb:
                morphpoints(dstP, &srcP[1], 3, meas, matrix);
                dst->cubicTo(dstP[0], dstP[1], dstP[2]);
                break;
            case SkPath::kClose_Verb:
                dst->close();
                break;
            default:
                SkDEBUGFAIL("unknown verb");
                break;
        }
    }
}

void SkDraw::drawTextOnPath(const char text[], size_t byteLength,
                            const SkPath& follow, const SkMatrix* matrix,
                            const SkPaint& paint) const {
    SkASSERT(byteLength == 0 || text != NULL);

    
    if (text == NULL || byteLength == 0 || fRC->isEmpty()) {
        return;
    }

    SkTextToPathIter    iter(text, byteLength, paint, true);
    SkPathMeasure       meas(follow, false);
    SkScalar            hOffset = 0;

    
    if (paint.getTextAlign() != SkPaint::kLeft_Align) {
        SkScalar pathLen = meas.getLength();
        if (paint.getTextAlign() == SkPaint::kCenter_Align) {
            pathLen = SkScalarHalf(pathLen);
        }
        hOffset += pathLen;
    }

    const SkPath*   iterPath;
    SkScalar        xpos;
    SkMatrix        scaledMatrix;
    SkScalar        scale = iter.getPathScale();

    scaledMatrix.setScale(scale, scale);

    while (iter.next(&iterPath, &xpos)) {
        if (iterPath) {
            SkPath      tmp;
            SkMatrix    m(scaledMatrix);

            m.postTranslate(xpos + hOffset, 0);
            if (matrix) {
                m.postConcat(*matrix);
            }
            morphpath(&tmp, *iterPath, meas, m);
            if (fDevice) {
                fDevice->drawPath(*this, tmp, iter.getPaint(), NULL, true);
            } else {
                this->drawPath(tmp, iter.getPaint(), NULL, true);
            }
        }
    }
}



typedef void (*HairProc)(const SkPoint&, const SkPoint&, const SkRasterClip&,
                         SkBlitter*);

static HairProc ChooseHairProc(bool doAntiAlias) {
    return doAntiAlias ? SkScan::AntiHairLine : SkScan::HairLine;
}

static bool texture_to_matrix(const VertState& state, const SkPoint verts[],
                              const SkPoint texs[], SkMatrix* matrix) {
    SkPoint src[3], dst[3];

    src[0] = texs[state.f0];
    src[1] = texs[state.f1];
    src[2] = texs[state.f2];
    dst[0] = verts[state.f0];
    dst[1] = verts[state.f1];
    dst[2] = verts[state.f2];
    return matrix->setPolyToPoly(src, dst, 3);
}

class SkTriColorShader : public SkShader {
public:
    SkTriColorShader() {}

    virtual size_t contextSize() const SK_OVERRIDE;

    class TriColorShaderContext : public SkShader::Context {
    public:
        TriColorShaderContext(const SkTriColorShader& shader, const ContextRec&);
        virtual ~TriColorShaderContext();

        bool setup(const SkPoint pts[], const SkColor colors[], int, int, int);

        virtual void shadeSpan(int x, int y, SkPMColor dstC[], int count) SK_OVERRIDE;

    private:
        SkMatrix    fDstToUnit;
        SkPMColor   fColors[3];

        typedef SkShader::Context INHERITED;
    };

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkTriColorShader)

protected:
    SkTriColorShader(SkReadBuffer& buffer) : SkShader(buffer) {}

    virtual Context* onCreateContext(const ContextRec& rec, void* storage) const SK_OVERRIDE {
        return SkNEW_PLACEMENT_ARGS(storage, TriColorShaderContext, (*this, rec));
    }

private:
    typedef SkShader INHERITED;
};

bool SkTriColorShader::TriColorShaderContext::setup(const SkPoint pts[], const SkColor colors[],
                                                    int index0, int index1, int index2) {

    fColors[0] = SkPreMultiplyColor(colors[index0]);
    fColors[1] = SkPreMultiplyColor(colors[index1]);
    fColors[2] = SkPreMultiplyColor(colors[index2]);

    SkMatrix m, im;
    m.reset();
    m.set(0, pts[index1].fX - pts[index0].fX);
    m.set(1, pts[index2].fX - pts[index0].fX);
    m.set(2, pts[index0].fX);
    m.set(3, pts[index1].fY - pts[index0].fY);
    m.set(4, pts[index2].fY - pts[index0].fY);
    m.set(5, pts[index0].fY);
    if (!m.invert(&im)) {
        return false;
    }
    
    
    SkMatrix ctmInv;
    if (!this->getCTM().invert(&ctmInv)) {
        return false;
    }
    fDstToUnit.setConcat(im, ctmInv);
    return true;
}

#include "SkColorPriv.h"
#include "SkComposeShader.h"

static int ScalarTo256(SkScalar v) {
    int scale = SkScalarToFixed(v) >> 8;
    if (scale < 0) {
        scale = 0;
    }
    if (scale > 255) {
        scale = 255;
    }
    return SkAlpha255To256(scale);
}


SkTriColorShader::TriColorShaderContext::TriColorShaderContext(const SkTriColorShader& shader,
                                                               const ContextRec& rec)
    : INHERITED(shader, rec) {}

SkTriColorShader::TriColorShaderContext::~TriColorShaderContext() {}

size_t SkTriColorShader::contextSize() const {
    return sizeof(TriColorShaderContext);
}
void SkTriColorShader::TriColorShaderContext::shadeSpan(int x, int y, SkPMColor dstC[], int count) {
    const int alphaScale = Sk255To256(this->getPaintAlpha());

    SkPoint src;

    for (int i = 0; i < count; i++) {
        fDstToUnit.mapXY(SkIntToScalar(x), SkIntToScalar(y), &src);
        x += 1;

        int scale1 = ScalarTo256(src.fX);
        int scale2 = ScalarTo256(src.fY);
        int scale0 = 256 - scale1 - scale2;
        if (scale0 < 0) {
            if (scale1 > scale2) {
                scale2 = 256 - scale1;
            } else {
                scale1 = 256 - scale2;
            }
            scale0 = 0;
        }

        if (256 != alphaScale) {
            scale0 = SkAlphaMul(scale0, alphaScale);
            scale1 = SkAlphaMul(scale1, alphaScale);
            scale2 = SkAlphaMul(scale2, alphaScale);
        }

        dstC[i] = SkAlphaMulQ(fColors[0], scale0) +
                  SkAlphaMulQ(fColors[1], scale1) +
                  SkAlphaMulQ(fColors[2], scale2);
    }
}

#ifndef SK_IGNORE_TO_STRING
void SkTriColorShader::toString(SkString* str) const {
    str->append("SkTriColorShader: (");

    this->INHERITED::toString(str);

    str->append(")");
}
#endif

void SkDraw::drawVertices(SkCanvas::VertexMode vmode, int count,
                          const SkPoint vertices[], const SkPoint textures[],
                          const SkColor colors[], SkXfermode* xmode,
                          const uint16_t indices[], int indexCount,
                          const SkPaint& paint) const {
    SkASSERT(0 == count || NULL != vertices);

    
    if (count < 3 || (indices && indexCount < 3) || fRC->isEmpty()) {
        return;
    }

    
    SkAutoSTMalloc<16, SkPoint> storage(count);
    SkPoint* devVerts = storage.get();
    fMatrix->mapPoints(devVerts, vertices, count);

    










    SkTriColorShader triShader; 
    SkPaint p(paint);

    SkShader* shader = p.getShader();
    if (NULL == shader) {
        
        textures = NULL;
    } else if (NULL == textures) {
        
        p.setShader(NULL);
        shader = NULL;
    }

    
    SkAutoTUnref<SkComposeShader> composeShader;
    if (NULL != colors) {
        if (NULL == textures) {
            
            shader = p.setShader(&triShader);
        } else {
            
            SkASSERT(shader);
            bool releaseMode = false;
            if (NULL == xmode) {
                xmode = SkXfermode::Create(SkXfermode::kModulate_Mode);
                releaseMode = true;
            }
            composeShader.reset(SkNEW_ARGS(SkComposeShader, (&triShader, shader, xmode)));
            p.setShader(composeShader);
            if (releaseMode) {
                xmode->unref();
            }
        }
    }

    SkAutoBlitterChoose blitter(*fBitmap, *fMatrix, p);
    
    if (blitter->isNullBlitter()) {
        return;
    }

    
    VertState       state(count, indices, indexCount);
    VertState::Proc vertProc = state.chooseProc(vmode);

    if (NULL != textures || NULL != colors) {
        while (vertProc(&state)) {
            if (NULL != textures) {
                SkMatrix tempM;
                if (texture_to_matrix(state, vertices, textures, &tempM)) {
                    SkShader::ContextRec rec(*fBitmap, p, *fMatrix);
                    rec.fLocalMatrix = &tempM;
                    if (!blitter->resetShaderContext(rec)) {
                        continue;
                    }
                }
            }
            if (NULL != colors) {
                
                SkTriColorShader::TriColorShaderContext* triColorShaderContext;

                SkShader::Context* shaderContext = blitter->getShaderContext();
                SkASSERT(shaderContext);
                if (p.getShader() == &triShader) {
                    triColorShaderContext =
                            static_cast<SkTriColorShader::TriColorShaderContext*>(shaderContext);
                } else {
                    
                    SkASSERT(p.getShader() == composeShader);
                    SkASSERT(composeShader->getShaderA() == &triShader);
                    SkComposeShader::ComposeShaderContext* composeShaderContext =
                            static_cast<SkComposeShader::ComposeShaderContext*>(shaderContext);
                    SkShader::Context* shaderContextA = composeShaderContext->getShaderContextA();
                    triColorShaderContext =
                            static_cast<SkTriColorShader::TriColorShaderContext*>(shaderContextA);
                }

                if (!triColorShaderContext->setup(vertices, colors,
                                                  state.f0, state.f1, state.f2)) {
                    continue;
                }
            }

            SkPoint tmp[] = {
                devVerts[state.f0], devVerts[state.f1], devVerts[state.f2]
            };
            SkScan::FillTriangle(tmp, *fRC, blitter.get());
        }
    } else {
        
        HairProc hairProc = ChooseHairProc(paint.isAntiAlias());
        const SkRasterClip& clip = *fRC;
        while (vertProc(&state)) {
            hairProc(devVerts[state.f0], devVerts[state.f1], clip, blitter.get());
            hairProc(devVerts[state.f1], devVerts[state.f2], clip, blitter.get());
            hairProc(devVerts[state.f2], devVerts[state.f0], clip, blitter.get());
        }
    }
}




#ifdef SK_DEBUG

void SkDraw::validate() const {
    SkASSERT(fBitmap != NULL);
    SkASSERT(fMatrix != NULL);
    SkASSERT(fClip != NULL);
    SkASSERT(fRC != NULL);

    const SkIRect&  cr = fRC->getBounds();
    SkIRect         br;

    br.set(0, 0, fBitmap->width(), fBitmap->height());
    SkASSERT(cr.isEmpty() || br.contains(cr));
}

#endif



#include "SkPath.h"
#include "SkDraw.h"
#include "SkRegion.h"
#include "SkBlitter.h"

static bool compute_bounds(const SkPath& devPath, const SkIRect* clipBounds,
                       const SkMaskFilter* filter, const SkMatrix* filterMatrix,
                           SkIRect* bounds) {
    if (devPath.isEmpty()) {
        return false;
    }

    
    {
        SkRect pathBounds = devPath.getBounds();
        pathBounds.inset(-SK_ScalarHalf, -SK_ScalarHalf);
        pathBounds.roundOut(bounds);
    }

    SkIPoint margin = SkIPoint::Make(0, 0);
    if (filter) {
        SkASSERT(filterMatrix);

        SkMask srcM, dstM;

        srcM.fBounds = *bounds;
        srcM.fFormat = SkMask::kA8_Format;
        srcM.fImage = NULL;
        if (!filter->filterMask(&dstM, srcM, *filterMatrix, &margin)) {
            return false;
        }
    }

    
    
    if (clipBounds) {
        SkIRect tmp = *clipBounds;
        
        
        
        
        
        
        static const int MAX_MARGIN = 128;
        tmp.inset(-SkMin32(margin.fX, MAX_MARGIN),
                  -SkMin32(margin.fY, MAX_MARGIN));
        if (!bounds->intersect(tmp)) {
            return false;
        }
    }

    return true;
}

static void draw_into_mask(const SkMask& mask, const SkPath& devPath,
                           SkPaint::Style style) {
    SkBitmap        bm;
    SkDraw          draw;
    SkRasterClip    clip;
    SkMatrix        matrix;
    SkPaint         paint;

    bm.installPixels(SkImageInfo::MakeA8(mask.fBounds.width(), mask.fBounds.height()),
                     mask.fImage, mask.fRowBytes);

    clip.setRect(SkIRect::MakeWH(mask.fBounds.width(), mask.fBounds.height()));
    matrix.setTranslate(-SkIntToScalar(mask.fBounds.fLeft),
                        -SkIntToScalar(mask.fBounds.fTop));

    draw.fBitmap    = &bm;
    draw.fRC        = &clip;
    draw.fClip      = &clip.bwRgn();
    draw.fMatrix    = &matrix;
    paint.setAntiAlias(true);
    paint.setStyle(style);
    draw.drawPath(devPath, paint);
}

bool SkDraw::DrawToMask(const SkPath& devPath, const SkIRect* clipBounds,
                        const SkMaskFilter* filter, const SkMatrix* filterMatrix,
                        SkMask* mask, SkMask::CreateMode mode,
                        SkPaint::Style style) {
    if (SkMask::kJustRenderImage_CreateMode != mode) {
        if (!compute_bounds(devPath, clipBounds, filter, filterMatrix, &mask->fBounds))
            return false;
    }

    if (SkMask::kComputeBoundsAndRenderImage_CreateMode == mode) {
        mask->fFormat = SkMask::kA8_Format;
        mask->fRowBytes = mask->fBounds.width();
        size_t size = mask->computeImageSize();
        if (0 == size) {
            
            return false;
        }
        mask->fImage = SkMask::AllocImage(size);
        memset(mask->fImage, 0, mask->computeImageSize());
    }

    if (SkMask::kJustComputeBounds_CreateMode != mode) {
        draw_into_mask(*mask, devPath, style);
    }

    return true;
}
