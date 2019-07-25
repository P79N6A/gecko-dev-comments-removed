






#ifndef SkRasterClip_DEFINED
#define SkRasterClip_DEFINED

#include "SkRegion.h"
#include "SkAAClip.h"

class SkRasterClip {
public:
    SkRasterClip();
    SkRasterClip(const SkIRect&);
    SkRasterClip(const SkRasterClip&);
    ~SkRasterClip();

    bool isBW() const { return fIsBW; }
    bool isAA() const { return !fIsBW; }
    const SkRegion& bwRgn() const { SkASSERT(fIsBW); return fBW; }
    const SkAAClip& aaRgn() const { SkASSERT(!fIsBW); return fAA; }

    bool isEmpty() const;
    bool isRect() const;
    bool isComplex() const;
    const SkIRect& getBounds() const;

    bool setEmpty();
    bool setRect(const SkIRect&);

    bool setPath(const SkPath& path, const SkRegion& clip, bool doAA);
    bool setPath(const SkPath& path, const SkIRect& clip, bool doAA);
    bool setPath(const SkPath& path, const SkRasterClip&, bool doAA);

    bool op(const SkIRect&, SkRegion::Op);
    bool op(const SkRegion&, SkRegion::Op);
    bool op(const SkRasterClip&, SkRegion::Op);
    bool op(const SkRect&, SkRegion::Op, bool doAA);

    void translate(int dx, int dy, SkRasterClip* dst) const;
    void translate(int dx, int dy) {
        this->translate(dx, dy, this);
    }

    bool quickContains(const SkIRect& rect) const;
    bool quickContains(int left, int top, int right, int bottom) const {
        return quickContains(SkIRect::MakeLTRB(left, top, right, bottom));
    }
    
    




    bool quickReject(const SkIRect& rect) const {
        return this->isEmpty() || rect.isEmpty() ||
               !SkIRect::Intersects(this->getBounds(), rect);
    }
    
    
    const SkRegion& forceGetBW();

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif

private:
    SkRegion    fBW;
    SkAAClip    fAA;
    bool        fIsBW;

    void convertToAA();
};

class SkAutoRasterClipValidate : SkNoncopyable {
public:
    SkAutoRasterClipValidate(const SkRasterClip& rc) : fRC(rc) {
        fRC.validate();
    }
    ~SkAutoRasterClipValidate() {
        fRC.validate();
    }
private:
    const SkRasterClip& fRC;
};

#ifdef SK_DEBUG
    #define AUTO_RASTERCLIP_VALIDATE(rc)    SkAutoRasterClipValidate arcv(rc)
#else
    #define AUTO_RASTERCLIP_VALIDATE(rc)
#endif












class SkAAClipBlitterWrapper {
public:
    SkAAClipBlitterWrapper();
    SkAAClipBlitterWrapper(const SkRasterClip&, SkBlitter*);
    SkAAClipBlitterWrapper(const SkAAClip*, SkBlitter*);
    
    void init(const SkRasterClip&, SkBlitter*);

    const SkIRect& getBounds() const {
        SkASSERT(fClipRgn);
        return fClipRgn->getBounds();
    }
    const SkRegion& getRgn() const {
        SkASSERT(fClipRgn);
        return *fClipRgn;
    }
    SkBlitter* getBlitter() {
        SkASSERT(fBlitter);
        return fBlitter;
    }
    
private:
    const SkAAClip* fAAClip;
    SkRegion        fBWRgn;
    SkAAClipBlitter fAABlitter;
    
    const SkRegion* fClipRgn;
    SkBlitter* fBlitter;
};

#endif
