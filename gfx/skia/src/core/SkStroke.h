






#ifndef SkStroke_DEFINED
#define SkStroke_DEFINED

#include "SkPath.h"
#include "SkPoint.h"
#include "SkPaint.h"







class SkStroke {
public:
    SkStroke();
    SkStroke(const SkPaint&);
    SkStroke(const SkPaint&, SkScalar width);   

    SkPaint::Cap    getCap() const { return (SkPaint::Cap)fCap; }
    void        setCap(SkPaint::Cap);

    SkPaint::Join   getJoin() const { return (SkPaint::Join)fJoin; }
    void        setJoin(SkPaint::Join);

    void    setMiterLimit(SkScalar);
    void    setWidth(SkScalar);

    bool    getDoFill() const { return SkToBool(fDoFill); }
    void    setDoFill(bool doFill) { fDoFill = SkToU8(doFill); }

    


    void    strokeRect(const SkRect& rect, SkPath* result,
                       SkPath::Direction = SkPath::kCW_Direction) const;
    void    strokePath(const SkPath& path, SkPath*) const;

    

private:
    SkScalar    fWidth, fMiterLimit;
    uint8_t     fCap, fJoin;
    SkBool8     fDoFill;

    friend class SkPaint;
};

#endif
