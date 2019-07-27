








#ifndef SkBlitter_DEFINED
#define SkBlitter_DEFINED

#include "SkBitmap.h"
#include "SkBitmapProcShader.h"
#include "SkMask.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "SkRefCnt.h"
#include "SkRegion.h"
#include "SkShader.h"
#include "SkSmallAllocator.h"




class SkBlitter {
public:
    virtual ~SkBlitter();

    
    virtual void blitH(int x, int y, int width);
    
    
    virtual void blitAntiH(int x, int y, const SkAlpha antialias[],
                           const int16_t runs[]);

    
    virtual void blitV(int x, int y, int height, SkAlpha alpha);
    
    virtual void blitRect(int x, int y, int width, int height);
    




    virtual void blitAntiRect(int x, int y, int width, int height,
                              SkAlpha leftAlpha, SkAlpha rightAlpha);
    
    
    virtual void blitMask(const SkMask&, const SkIRect& clip);

    



    virtual const SkBitmap* justAnOpaqueColor(uint32_t* value);

    




    virtual bool isNullBlitter() const;

    


    virtual bool resetShaderContext(const SkShader::ContextRec&);
    virtual SkShader::Context* getShaderContext() const;

    





    virtual int requestRowsPreserved() const { return 1; }

    





    virtual void* allocBlitMemory(size_t sz) {
        return fBlitMemory.reset(sz, SkAutoMalloc::kReuse_OnShrink);
    }

    
    void blitMaskRegion(const SkMask& mask, const SkRegion& clip);
    void blitRectRegion(const SkIRect& rect, const SkRegion& clip);
    void blitRegion(const SkRegion& clip);
    

    


    static SkBlitter* Choose(const SkBitmap& device,
                             const SkMatrix& matrix,
                             const SkPaint& paint,
                             SkTBlitterAllocator*,
                             bool drawCoverage = false);

    static SkBlitter* ChooseSprite(const SkBitmap& device,
                                   const SkPaint&,
                                   const SkBitmap& src,
                                   int left, int top,
                                   SkTBlitterAllocator*);
    

protected:

    SkAutoMalloc fBlitMemory;
    
private:
};



class SkNullBlitter : public SkBlitter {
public:
    virtual void blitH(int x, int y, int width) SK_OVERRIDE;
    virtual void blitAntiH(int x, int y, const SkAlpha[],
                           const int16_t runs[]) SK_OVERRIDE;
    virtual void blitV(int x, int y, int height, SkAlpha alpha) SK_OVERRIDE;
    virtual void blitRect(int x, int y, int width, int height) SK_OVERRIDE;
    virtual void blitMask(const SkMask&, const SkIRect& clip) SK_OVERRIDE;
    virtual const SkBitmap* justAnOpaqueColor(uint32_t* value) SK_OVERRIDE;
    virtual bool isNullBlitter() const SK_OVERRIDE;
};





class SkRectClipBlitter : public SkBlitter {
public:
    void init(SkBlitter* blitter, const SkIRect& clipRect) {
        SkASSERT(!clipRect.isEmpty());
        fBlitter = blitter;
        fClipRect = clipRect;
    }

    virtual void blitH(int x, int y, int width) SK_OVERRIDE;
    virtual void blitAntiH(int x, int y, const SkAlpha[],
                           const int16_t runs[]) SK_OVERRIDE;
    virtual void blitV(int x, int y, int height, SkAlpha alpha) SK_OVERRIDE;
    virtual void blitRect(int x, int y, int width, int height) SK_OVERRIDE;
    virtual void blitAntiRect(int x, int y, int width, int height,
                     SkAlpha leftAlpha, SkAlpha rightAlpha) SK_OVERRIDE;
    virtual void blitMask(const SkMask&, const SkIRect& clip) SK_OVERRIDE;
    virtual const SkBitmap* justAnOpaqueColor(uint32_t* value) SK_OVERRIDE;

    virtual void* allocBlitMemory(size_t sz) SK_OVERRIDE {
        return fBlitter->allocBlitMemory(sz);
    }

private:
    SkBlitter*  fBlitter;
    SkIRect     fClipRect;
};





class SkRgnClipBlitter : public SkBlitter {
public:
    void init(SkBlitter* blitter, const SkRegion* clipRgn) {
        SkASSERT(clipRgn && !clipRgn->isEmpty());
        fBlitter = blitter;
        fRgn = clipRgn;
    }

    virtual void blitH(int x, int y, int width) SK_OVERRIDE;
    virtual void blitAntiH(int x, int y, const SkAlpha[],
                           const int16_t runs[]) SK_OVERRIDE;
    virtual void blitV(int x, int y, int height, SkAlpha alpha) SK_OVERRIDE;
    virtual void blitRect(int x, int y, int width, int height) SK_OVERRIDE;
    virtual void blitAntiRect(int x, int y, int width, int height,
                     SkAlpha leftAlpha, SkAlpha rightAlpha) SK_OVERRIDE;
    virtual void blitMask(const SkMask&, const SkIRect& clip) SK_OVERRIDE;
    virtual const SkBitmap* justAnOpaqueColor(uint32_t* value) SK_OVERRIDE;

    virtual void* allocBlitMemory(size_t sz) SK_OVERRIDE {
        return fBlitter->allocBlitMemory(sz);
    }

private:
    SkBlitter*      fBlitter;
    const SkRegion* fRgn;
};





class SkBlitterClipper {
public:
    SkBlitter*  apply(SkBlitter* blitter, const SkRegion* clip,
                      const SkIRect* bounds = NULL);

private:
    SkNullBlitter       fNullBlitter;
    SkRectClipBlitter   fRectBlitter;
    SkRgnClipBlitter    fRgnBlitter;
};

#endif
