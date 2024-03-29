







#ifndef SkAAClip_DEFINED
#define SkAAClip_DEFINED

#include "SkBlitter.h"
#include "SkRegion.h"

class SkAAClip {
public:
    SkAAClip();
    SkAAClip(const SkAAClip&);
    ~SkAAClip();

    SkAAClip& operator=(const SkAAClip&);
    friend bool operator==(const SkAAClip&, const SkAAClip&);
    friend bool operator!=(const SkAAClip& a, const SkAAClip& b) {
        return !(a == b);
    }

    void swap(SkAAClip&);

    bool isEmpty() const { return NULL == fRunHead; }
    const SkIRect& getBounds() const { return fBounds; }

    bool setEmpty();
    bool setRect(const SkIRect&);
    bool setRect(const SkRect&, bool doAA = true);
    bool setPath(const SkPath&, const SkRegion* clip = NULL, bool doAA = true);
    bool setRegion(const SkRegion&);
    bool set(const SkAAClip&);

    bool op(const SkAAClip&, const SkAAClip&, SkRegion::Op);

    
    bool op(const SkIRect&, SkRegion::Op);
    bool op(const SkRect&, SkRegion::Op, bool doAA);
    bool op(const SkAAClip&, SkRegion::Op);

    bool translate(int dx, int dy, SkAAClip* dst) const;
    bool translate(int dx, int dy) {
        return this->translate(dx, dy, this);
    }

    



    void copyToMask(SkMask*) const;

    

    bool quickContains(int left, int top, int right, int bottom) const;
    bool quickContains(const SkIRect& r) const {
        return this->quickContains(r.fLeft, r.fTop, r.fRight, r.fBottom);
    }

    const uint8_t* findRow(int y, int* lastYForRow = NULL) const;
    const uint8_t* findX(const uint8_t data[], int x, int* initialCount = NULL) const;

    class Iter;
    struct RunHead;
    struct YOffset;
    class Builder;

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif

private:
    SkIRect  fBounds;
    RunHead* fRunHead;

    void freeRuns();
    bool trimBounds();
    bool trimTopBottom();
    bool trimLeftRight();

    friend class Builder;
    class BuilderBlitter;
    friend class BuilderBlitter;
};



class SkAAClipBlitter : public SkBlitter {
public:
    SkAAClipBlitter() : fScanlineScratch(NULL) {}
    virtual ~SkAAClipBlitter();

    void init(SkBlitter* blitter, const SkAAClip* aaclip) {
        SkASSERT(aaclip && !aaclip->isEmpty());
        fBlitter = blitter;
        fAAClip = aaclip;
        fAAClipBounds = aaclip->getBounds();
    }

    virtual void blitH(int x, int y, int width) SK_OVERRIDE;
    virtual void blitAntiH(int x, int y, const SkAlpha[],
                           const int16_t runs[]) SK_OVERRIDE;
    virtual void blitV(int x, int y, int height, SkAlpha alpha) SK_OVERRIDE;
    virtual void blitRect(int x, int y, int width, int height) SK_OVERRIDE;
    virtual void blitMask(const SkMask&, const SkIRect& clip) SK_OVERRIDE;
    virtual const SkBitmap* justAnOpaqueColor(uint32_t* value) SK_OVERRIDE;

private:
    SkBlitter*      fBlitter;
    const SkAAClip* fAAClip;
    SkIRect         fAAClipBounds;

    
    int16_t*        fRuns;
    SkAlpha*        fAA;

    enum {
        kSize = 32 * 32
    };
    SkAutoSMalloc<kSize> fGrayMaskScratch;  
    void* fScanlineScratch;  

    void ensureRunsAndAA();
};

#endif
