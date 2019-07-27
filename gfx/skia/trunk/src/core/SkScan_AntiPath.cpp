








#include "SkScanPriv.h"
#include "SkPath.h"
#include "SkMatrix.h"
#include "SkBlitter.h"
#include "SkRegion.h"
#include "SkAntiRun.h"

#define SHIFT   2
#define SCALE   (1 << SHIFT)
#define MASK    (SCALE - 1)

























class BaseSuperBlitter : public SkBlitter {
public:
    BaseSuperBlitter(SkBlitter* realBlitter, const SkIRect& ir,
                     const SkRegion& clip);

    
    virtual void blitAntiH(int x, int y, const SkAlpha antialias[],
                           const int16_t runs[]) SK_OVERRIDE {
        SkDEBUGFAIL("How did I get here?");
    }
    
    virtual void blitV(int x, int y, int height, SkAlpha alpha) SK_OVERRIDE {
        SkDEBUGFAIL("How did I get here?");
    }

protected:
    SkBlitter*  fRealBlitter;
    
    int         fCurrIY;
    
    int         fWidth;
    
    int         fLeft;
    
    int         fSuperLeft;

    SkDEBUGCODE(int fCurrX;)
    
    int fCurrY;
    
    int fTop;
};

BaseSuperBlitter::BaseSuperBlitter(SkBlitter* realBlitter, const SkIRect& ir,
                                   const SkRegion& clip) {
    fRealBlitter = realBlitter;

    



    const int left = clip.getBounds().fLeft;
    const int right = clip.getBounds().fRight;

    fLeft = left;
    fSuperLeft = left << SHIFT;
    fWidth = right - left;
#if 0
    fCurrIY = -1;
    fCurrY = -1;
#else
    fTop = ir.fTop;
    fCurrIY = ir.fTop - 1;
    fCurrY = (ir.fTop << SHIFT) - 1;
#endif
    SkDEBUGCODE(fCurrX = -1;)
}


class SuperBlitter : public BaseSuperBlitter {
public:
    SuperBlitter(SkBlitter* realBlitter, const SkIRect& ir,
                 const SkRegion& clip);

    virtual ~SuperBlitter() {
        this->flush();
    }

    
    
    void flush();

    
    
    virtual void blitH(int x, int y, int width) SK_OVERRIDE;
    
    
    virtual void blitRect(int x, int y, int width, int height) SK_OVERRIDE;

private:
    
    
    
    
    int         fRunsToBuffer;
    void*       fRunsBuffer;
    int         fCurrentRun;
    SkAlphaRuns fRuns;

    
    int getRunsSz() const { return (fWidth + 1 + (fWidth + 2)/2) * sizeof(int16_t); }

    
    
    
    void advanceRuns() {
        const size_t kRunsSz = this->getRunsSz();
        fCurrentRun = (fCurrentRun + 1) % fRunsToBuffer;
        fRuns.fRuns = reinterpret_cast<int16_t*>(
            reinterpret_cast<uint8_t*>(fRunsBuffer) + fCurrentRun * kRunsSz);
        fRuns.fAlpha = reinterpret_cast<SkAlpha*>(fRuns.fRuns + fWidth + 1);
        fRuns.reset(fWidth);
    }

    int         fOffsetX;
};

SuperBlitter::SuperBlitter(SkBlitter* realBlitter, const SkIRect& ir,
                           const SkRegion& clip)
        : BaseSuperBlitter(realBlitter, ir, clip) {
    fRunsToBuffer = realBlitter->requestRowsPreserved();
    fRunsBuffer = realBlitter->allocBlitMemory(fRunsToBuffer * this->getRunsSz());
    fCurrentRun = -1;

    this->advanceRuns();

    fOffsetX = 0;
}

void SuperBlitter::flush() {
    if (fCurrIY >= fTop) {

        SkASSERT(fCurrentRun < fRunsToBuffer);
        if (!fRuns.empty()) {
            
            fRealBlitter->blitAntiH(fLeft, fCurrIY, fRuns.fAlpha, fRuns.fRuns);
            this->advanceRuns();
            fOffsetX = 0;
        }

        fCurrIY = fTop - 1;
        SkDEBUGCODE(fCurrX = -1;)
    }
}







static inline int coverage_to_partial_alpha(int aa) {
    aa <<= 8 - 2*SHIFT;
#ifdef SK_USE_LEGACY_AA_COVERAGE
    aa -= aa >> (8 - SHIFT - 1);
#endif
    return aa;
}




static inline int coverage_to_exact_alpha(int aa) {
    int alpha = (256 >> SHIFT) * aa;
    
    return alpha - (alpha >> 8);
}

void SuperBlitter::blitH(int x, int y, int width) {
    SkASSERT(width > 0);

    int iy = y >> SHIFT;
    SkASSERT(iy >= fCurrIY);

    x -= fSuperLeft;
    
    if (x < 0) {
        width += x;
        x = 0;
    }

#ifdef SK_DEBUG
    SkASSERT(y != fCurrY || x >= fCurrX);
#endif
    SkASSERT(y >= fCurrY);
    if (fCurrY != y) {
        fOffsetX = 0;
        fCurrY = y;
    }

    if (iy != fCurrIY) {  
        this->flush();
        fCurrIY = iy;
    }

    int start = x;
    int stop = x + width;

    SkASSERT(start >= 0 && stop > start);
    
    int fb = start & MASK;
    int fe = stop & MASK;
    int n = (stop >> SHIFT) - (start >> SHIFT) - 1;

    if (n < 0) {
        fb = fe - fb;
        n = 0;
        fe = 0;
    } else {
        if (fb == 0) {
            n += 1;
        } else {
            fb = SCALE - fb;
        }
    }

    fOffsetX = fRuns.add(x >> SHIFT, coverage_to_partial_alpha(fb),
                         n, coverage_to_partial_alpha(fe),
                         (1 << (8 - SHIFT)) - (((y & MASK) + 1) >> SHIFT),
                         fOffsetX);

#ifdef SK_DEBUG
    fRuns.assertValid(y & MASK, (1 << (8 - SHIFT)));
    fCurrX = x + width;
#endif
}

#if 0 
static void set_left_rite_runs(SkAlphaRuns& runs, int ileft, U8CPU leftA,
                               int n, U8CPU riteA) {
    SkASSERT(leftA <= 0xFF);
    SkASSERT(riteA <= 0xFF);

    int16_t* run = runs.fRuns;
    uint8_t* aa = runs.fAlpha;

    if (ileft > 0) {
        run[0] = ileft;
        aa[0] = 0;
        run += ileft;
        aa += ileft;
    }

    SkASSERT(leftA < 0xFF);
    if (leftA > 0) {
        *run++ = 1;
        *aa++ = leftA;
    }

    if (n > 0) {
        run[0] = n;
        aa[0] = 0xFF;
        run += n;
        aa += n;
    }

    SkASSERT(riteA < 0xFF);
    if (riteA > 0) {
        *run++ = 1;
        *aa++ = riteA;
    }
    run[0] = 0;
}
#endif

void SuperBlitter::blitRect(int x, int y, int width, int height) {
    SkASSERT(width > 0);
    SkASSERT(height > 0);

    
    while ((y & MASK)) {
        this->blitH(x, y++, width);
        if (--height <= 0) {
            return;
        }
    }
    SkASSERT(height > 0);

    
    
    
    int start_y = y >> SHIFT;
    int stop_y = (y + height) >> SHIFT;
    int count = stop_y - start_y;
    if (count > 0) {
        y += count << SHIFT;
        height -= count << SHIFT;

        
        int origX = x;

        x -= fSuperLeft;
        
        if (x < 0) {
            width += x;
            x = 0;
        }

        
        
        
        
        int ileft = x >> SHIFT;
        int xleft = x & MASK;
        
        
        
        int irite = (x + width) >> SHIFT;
        int xrite = (x + width) & MASK;
        if (!xrite) {
            xrite = SCALE;
            irite--;
        }

        
        
        SkASSERT(start_y > fCurrIY);
        this->flush();

        int n = irite - ileft - 1;
        if (n < 0) {
            
            
            xleft = xrite - xleft;
            SkASSERT(xleft <= SCALE);
            SkASSERT(xleft > 0);
            xrite = 0;
            fRealBlitter->blitV(ileft + fLeft, start_y, count,
                coverage_to_exact_alpha(xleft));
        } else {
            
            

            xleft = SCALE - xleft;

            
            const int coverageL = coverage_to_exact_alpha(xleft);
            const int coverageR = coverage_to_exact_alpha(xrite);

            SkASSERT(coverageL > 0 || n > 0 || coverageR > 0);
            SkASSERT((coverageL != 0) + n + (coverageR != 0) <= fWidth);

            fRealBlitter->blitAntiRect(ileft + fLeft, start_y, n, count,
                                       coverageL, coverageR);
        }

        
        fCurrIY = stop_y - 1;
        fOffsetX = 0;
        fCurrY = y - 1;
        fRuns.reset(fWidth);
        x = origX;
    }

    
    SkASSERT(height <= MASK);
    while (--height >= 0) {
        this->blitH(x, y++, width);
    }
}




class MaskSuperBlitter : public BaseSuperBlitter {
public:
    MaskSuperBlitter(SkBlitter* realBlitter, const SkIRect& ir,
                     const SkRegion& clip);
    virtual ~MaskSuperBlitter() {
        fRealBlitter->blitMask(fMask, fClipRect);
    }

    virtual void blitH(int x, int y, int width) SK_OVERRIDE;

    static bool CanHandleRect(const SkIRect& bounds) {
#ifdef FORCE_RLE
        return false;
#endif
        int width = bounds.width();
        int64_t rb = SkAlign4(width);
        
        int64_t storage = rb * bounds.height();

        return (width <= MaskSuperBlitter::kMAX_WIDTH) &&
               (storage <= MaskSuperBlitter::kMAX_STORAGE);
    }

private:
    enum {
#ifdef FORCE_SUPERMASK
        kMAX_WIDTH = 2048,
        kMAX_STORAGE = 1024 * 1024 * 2
#else
        kMAX_WIDTH = 32,    
        kMAX_STORAGE = 1024
#endif
    };

    SkMask      fMask;
    SkIRect     fClipRect;
    
    
    uint32_t    fStorage[(kMAX_STORAGE >> 2) + 1];
};

MaskSuperBlitter::MaskSuperBlitter(SkBlitter* realBlitter, const SkIRect& ir,
                                   const SkRegion& clip)
        : BaseSuperBlitter(realBlitter, ir, clip) {
    SkASSERT(CanHandleRect(ir));

    fMask.fImage    = (uint8_t*)fStorage;
    fMask.fBounds   = ir;
    fMask.fRowBytes = ir.width();
    fMask.fFormat   = SkMask::kA8_Format;

    fClipRect = ir;
    fClipRect.intersect(clip.getBounds());

    
    
    memset(fStorage, 0, fMask.fBounds.height() * fMask.fRowBytes + 1);
}

static void add_aa_span(uint8_t* alpha, U8CPU startAlpha) {
    




    unsigned tmp = *alpha + startAlpha;
    SkASSERT(tmp <= 256);
    *alpha = SkToU8(tmp - (tmp >> 8));
}

static inline uint32_t quadplicate_byte(U8CPU value) {
    uint32_t pair = (value << 8) | value;
    return (pair << 16) | pair;
}






static inline void saturated_add(uint8_t* ptr, U8CPU add) {
    unsigned tmp = *ptr + add;
    SkASSERT(tmp <= 256);
    *ptr = SkToU8(tmp - (tmp >> 8));
}


#define MIN_COUNT_FOR_QUAD_LOOP  16

static void add_aa_span(uint8_t* alpha, U8CPU startAlpha, int middleCount,
                        U8CPU stopAlpha, U8CPU maxValue) {
    SkASSERT(middleCount >= 0);

    saturated_add(alpha, startAlpha);
    alpha += 1;

    if (middleCount >= MIN_COUNT_FOR_QUAD_LOOP) {
        
        while (SkTCast<intptr_t>(alpha) & 0x3) {
            alpha[0] = SkToU8(alpha[0] + maxValue);
            alpha += 1;
            middleCount -= 1;
        }

        int bigCount = middleCount >> 2;
        uint32_t* qptr = reinterpret_cast<uint32_t*>(alpha);
        uint32_t qval = quadplicate_byte(maxValue);
        do {
            *qptr++ += qval;
        } while (--bigCount > 0);

        middleCount &= 3;
        alpha = reinterpret_cast<uint8_t*> (qptr);
        
    }

    while (--middleCount >= 0) {
        alpha[0] = SkToU8(alpha[0] + maxValue);
        alpha += 1;
    }

    
    
    
    
    saturated_add(alpha, stopAlpha);
}

void MaskSuperBlitter::blitH(int x, int y, int width) {
    int iy = (y >> SHIFT);

    SkASSERT(iy >= fMask.fBounds.fTop && iy < fMask.fBounds.fBottom);
    iy -= fMask.fBounds.fTop;   

    
    
    
    if (iy < 0) {
        return;
    }

#ifdef SK_DEBUG
    {
        int ix = x >> SHIFT;
        SkASSERT(ix >= fMask.fBounds.fLeft && ix < fMask.fBounds.fRight);
    }
#endif

    x -= (fMask.fBounds.fLeft << SHIFT);

    
    if (x < 0) {
        width += x;
        x = 0;
    }

    uint8_t* row = fMask.fImage + iy * fMask.fRowBytes + (x >> SHIFT);

    int start = x;
    int stop = x + width;

    SkASSERT(start >= 0 && stop > start);
    int fb = start & MASK;
    int fe = stop & MASK;
    int n = (stop >> SHIFT) - (start >> SHIFT) - 1;


    if (n < 0) {
        SkASSERT(row >= fMask.fImage);
        SkASSERT(row < fMask.fImage + kMAX_STORAGE + 1);
        add_aa_span(row, coverage_to_partial_alpha(fe - fb));
    } else {
        fb = SCALE - fb;
        SkASSERT(row >= fMask.fImage);
        SkASSERT(row + n + 1 < fMask.fImage + kMAX_STORAGE + 1);
        add_aa_span(row,  coverage_to_partial_alpha(fb),
                    n, coverage_to_partial_alpha(fe),
                    (1 << (8 - SHIFT)) - (((y & MASK) + 1) >> SHIFT));
    }

#ifdef SK_DEBUG
    fCurrX = x + width;
#endif
}



static bool fitsInsideLimit(const SkRect& r, SkScalar max) {
    const SkScalar min = -max;
    return  r.fLeft > min && r.fTop > min &&
            r.fRight < max && r.fBottom < max;
}

static int overflows_short_shift(int value, int shift) {
    const int s = 16 + shift;
    return (value << s >> s) - value;
}





static int rect_overflows_short_shift(SkIRect rect, int shift) {
    SkASSERT(!overflows_short_shift(8191, SHIFT));
    SkASSERT(overflows_short_shift(8192, SHIFT));
    SkASSERT(!overflows_short_shift(32767, 0));
    SkASSERT(overflows_short_shift(32768, 0));

    
    
    return overflows_short_shift(rect.fLeft, SHIFT) |
           overflows_short_shift(rect.fRight, SHIFT) |
           overflows_short_shift(rect.fTop, SHIFT) |
           overflows_short_shift(rect.fBottom, SHIFT);
}

static bool safeRoundOut(const SkRect& src, SkIRect* dst, int32_t maxInt) {
    const SkScalar maxScalar = SkIntToScalar(maxInt);

    if (fitsInsideLimit(src, maxScalar)) {
        src.roundOut(dst);
        return true;
    }
    return false;
}

void SkScan::AntiFillPath(const SkPath& path, const SkRegion& origClip,
                          SkBlitter* blitter, bool forceRLE) {
    if (origClip.isEmpty()) {
        return;
    }

    SkIRect ir;

    if (!safeRoundOut(path.getBounds(), &ir, SK_MaxS32 >> SHIFT)) {
#if 0
        const SkRect& r = path.getBounds();
        SkDebugf("--- bounds can't fit in SkIRect\n", r.fLeft, r.fTop, r.fRight, r.fBottom);
#endif
        return;
    }
    if (ir.isEmpty()) {
        if (path.isInverseFillType()) {
            blitter->blitRegion(origClip);
        }
        return;
    }

    
    
    
    SkIRect clippedIR;
    if (path.isInverseFillType()) {
       
       
       clippedIR = origClip.getBounds();
    } else {
       if (!clippedIR.intersect(ir, origClip.getBounds())) {
           return;
       }
    }
    if (rect_overflows_short_shift(clippedIR, SHIFT)) {
        SkScan::FillPath(path, origClip, blitter);
        return;
    }

    
    
    
    
    
    
    SkRegion tmpClipStorage;
    const SkRegion* clipRgn = &origClip;
    {
        static const int32_t kMaxClipCoord = 32767;
        const SkIRect& bounds = origClip.getBounds();
        if (bounds.fRight > kMaxClipCoord || bounds.fBottom > kMaxClipCoord) {
            SkIRect limit = { 0, 0, kMaxClipCoord, kMaxClipCoord };
            tmpClipStorage.op(origClip, limit, SkRegion::kIntersect_Op);
            clipRgn = &tmpClipStorage;
        }
    }
    

    SkScanClipper   clipper(blitter, clipRgn, ir);
    const SkIRect*  clipRect = clipper.getClipRect();

    if (clipper.getBlitter() == NULL) { 
        if (path.isInverseFillType()) {
            blitter->blitRegion(*clipRgn);
        }
        return;
    }

    
    blitter = clipper.getBlitter();

    if (path.isInverseFillType()) {
        sk_blit_above(blitter, ir, *clipRgn);
    }

    SkIRect superRect, *superClipRect = NULL;

    if (clipRect) {
        superRect.set(  clipRect->fLeft << SHIFT, clipRect->fTop << SHIFT,
                        clipRect->fRight << SHIFT, clipRect->fBottom << SHIFT);
        superClipRect = &superRect;
    }

    SkASSERT(SkIntToScalar(ir.fTop) <= path.getBounds().fTop);

    
    
    if (!path.isInverseFillType() && MaskSuperBlitter::CanHandleRect(ir) && !forceRLE) {
        MaskSuperBlitter    superBlit(blitter, ir, *clipRgn);
        SkASSERT(SkIntToScalar(ir.fTop) <= path.getBounds().fTop);
        sk_fill_path(path, superClipRect, &superBlit, ir.fTop, ir.fBottom, SHIFT, *clipRgn);
    } else {
        SuperBlitter    superBlit(blitter, ir, *clipRgn);
        sk_fill_path(path, superClipRect, &superBlit, ir.fTop, ir.fBottom, SHIFT, *clipRgn);
    }

    if (path.isInverseFillType()) {
        sk_blit_below(blitter, ir, *clipRgn);
    }
}



#include "SkRasterClip.h"

void SkScan::FillPath(const SkPath& path, const SkRasterClip& clip,
                          SkBlitter* blitter) {
    if (clip.isEmpty()) {
        return;
    }

    if (clip.isBW()) {
        FillPath(path, clip.bwRgn(), blitter);
    } else {
        SkRegion        tmp;
        SkAAClipBlitter aaBlitter;

        tmp.setRect(clip.getBounds());
        aaBlitter.init(blitter, &clip.aaRgn());
        SkScan::FillPath(path, tmp, &aaBlitter);
    }
}

void SkScan::AntiFillPath(const SkPath& path, const SkRasterClip& clip,
                          SkBlitter* blitter) {
    if (clip.isEmpty()) {
        return;
    }

    if (clip.isBW()) {
        AntiFillPath(path, clip.bwRgn(), blitter);
    } else {
        SkRegion        tmp;
        SkAAClipBlitter aaBlitter;

        tmp.setRect(clip.getBounds());
        aaBlitter.init(blitter, &clip.aaRgn());
        SkScan::AntiFillPath(path, tmp, &aaBlitter, true);
    }
}
