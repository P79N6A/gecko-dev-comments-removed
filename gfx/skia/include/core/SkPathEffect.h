








#ifndef SkPathEffect_DEFINED
#define SkPathEffect_DEFINED

#include "SkFlattenable.h"
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
    explicit SkStrokeRec(const SkPaint&);

    enum Style {
        kHairline_Style,
        kFill_Style,
        kStroke_Style,
        kStrokeAndFill_Style
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

private:
    SkScalar        fWidth;
    SkScalar        fMiterLimit;
    SkPaint::Cap    fCap;
    SkPaint::Join   fJoin;
    bool            fStrokeAndFill;
};









class SK_API SkPathEffect : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkPathEffect)

    SkPathEffect() {}

    














    virtual bool filterPath(SkPath* dst, const SkPath& src, SkStrokeRec*) = 0;

    



    virtual void computeFastBounds(SkRect* dst, const SkRect& src);

protected:
    SkPathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

private:
    
    SkPathEffect(const SkPathEffect&);
    SkPathEffect& operator=(const SkPathEffect&);

    typedef SkFlattenable INHERITED;
};







class SkPairPathEffect : public SkPathEffect {
public:
    SkPairPathEffect(SkPathEffect* pe0, SkPathEffect* pe1);
    virtual ~SkPairPathEffect();

protected:
    SkPairPathEffect(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

    
    SkPathEffect* fPE0, *fPE1;

private:
    typedef SkPathEffect INHERITED;
};






class SkComposePathEffect : public SkPairPathEffect {
public:
    




    SkComposePathEffect(SkPathEffect* outer, SkPathEffect* inner)
        : INHERITED(outer, inner) {}

    virtual bool filterPath(SkPath* dst, const SkPath& src, SkStrokeRec*) SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkComposePathEffect)

protected:
    SkComposePathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

private:
    
    SkComposePathEffect(const SkComposePathEffect&);
    SkComposePathEffect& operator=(const SkComposePathEffect&);

    typedef SkPairPathEffect INHERITED;
};






class SkSumPathEffect : public SkPairPathEffect {
public:
    




    SkSumPathEffect(SkPathEffect* first, SkPathEffect* second)
        : INHERITED(first, second) {}

    virtual bool filterPath(SkPath* dst, const SkPath& src, SkStrokeRec*) SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkSumPathEffect)

protected:
    SkSumPathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

private:
    
    SkSumPathEffect(const SkSumPathEffect&);
    SkSumPathEffect& operator=(const SkSumPathEffect&);

    typedef SkPairPathEffect INHERITED;
};

#endif

