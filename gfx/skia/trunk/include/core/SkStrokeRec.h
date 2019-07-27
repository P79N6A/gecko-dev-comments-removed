






#ifndef SkStrokeRec_DEFINED
#define SkStrokeRec_DEFINED

#include "SkPaint.h"

class SkPath;

class SkStrokeRec {
public:
    enum InitStyle {
        kHairline_InitStyle,
        kFill_InitStyle
    };
    SkStrokeRec(InitStyle style);

    SkStrokeRec(const SkStrokeRec&);
    SkStrokeRec(const SkPaint&, SkPaint::Style);
    explicit SkStrokeRec(const SkPaint&);

    enum Style {
        kHairline_Style,
        kFill_Style,
        kStroke_Style,
        kStrokeAndFill_Style
    };
    enum {
        kStyleCount = kStrokeAndFill_Style + 1
    };

    Style getStyle() const;
    SkScalar getWidth() const { return fWidth; }
    SkScalar getMiter() const { return fMiterLimit; }
    SkPaint::Cap getCap() const { return fCap; }
    SkPaint::Join getJoin() const { return fJoin; }

    bool isHairlineStyle() const {
        return kHairline_Style == this->getStyle();
    }

    bool isFillStyle() const {
        return kFill_Style == this->getStyle();
    }

    void setFillStyle();
    void setHairlineStyle();
    





    void setStrokeStyle(SkScalar width, bool strokeAndFill = false);

    void setStrokeParams(SkPaint::Cap cap, SkPaint::Join join, SkScalar miterLimit) {
        fCap = cap;
        fJoin = join;
        fMiterLimit = miterLimit;
    }

    



    bool needToApply() const {
        Style style = this->getStyle();
        return (kStroke_Style == style) || (kStrokeAndFill_Style == style);
    }

    









    bool applyToPath(SkPath* dst, const SkPath& src) const;

    bool operator==(const SkStrokeRec& other) const {
            return fWidth == other.fWidth &&
                   fMiterLimit == other.fMiterLimit &&
                   fCap == other.fCap &&
                   fJoin == other.fJoin &&
                   fStrokeAndFill == other.fStrokeAndFill;
    }

private:
    void init(const SkPaint& paint, SkPaint::Style style);


    SkScalar        fWidth;
    SkScalar        fMiterLimit;
    SkPaint::Cap    fCap;
    SkPaint::Join   fJoin;
    bool            fStrokeAndFill;
};

#endif
