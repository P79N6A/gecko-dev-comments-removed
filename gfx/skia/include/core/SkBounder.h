








#ifndef SkBounder_DEFINED
#define SkBounder_DEFINED

#include "SkTypes.h"
#include "SkRefCnt.h"
#include "SkPoint.h"

struct SkGlyph;
struct SkIRect;
struct SkPoint;
struct SkRect;
class SkPaint;
class SkPath;
class SkRegion;






class SkBounder : public SkRefCnt {
public:
    SkBounder();

    


    bool doIRect(const SkIRect&);
    bool doIRectGlyph(const SkIRect& , int x, int y, const SkGlyph&);

protected:
    





    virtual bool onIRect(const SkIRect&) {
        return false;
    }

    



    struct GlyphRec {
        SkIPoint    fLSB;   
        SkIPoint    fRSB;   
        uint16_t    fGlyphID;
        uint16_t    fFlags; 
    };

    


    virtual bool onIRectGlyph(const SkIRect& r, const GlyphRec&) {
        return onIRect(r);
    }

    



    virtual void commit();

private:
    bool doHairline(const SkPoint&, const SkPoint&, const SkPaint&);
    bool doRect(const SkRect&, const SkPaint&);
    bool doPath(const SkPath&, const SkPaint&, bool doFill);
    void setClip(const SkRegion* clip) { fClip = clip; }

    const SkRegion* fClip;
    friend class SkAutoBounderCommit;
    friend class SkDraw;
    friend class SkDrawIter;
    friend struct Draw1Glyph;
    friend class SkMaskFilter;
};

#endif

