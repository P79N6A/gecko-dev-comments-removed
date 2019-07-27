






#ifndef Sk1DPathEffect_DEFINED
#define Sk1DPathEffect_DEFINED

#include "SkPathEffect.h"
#include "SkPath.h"

class SkPathMeasure;


class SK_API Sk1DPathEffect : public SkPathEffect {
public:
    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

protected:
    


    virtual SkScalar begin(SkScalar contourLength) const = 0;
    




    virtual SkScalar next(SkPath* dst, SkScalar dist, SkPathMeasure&) const = 0;

private:
    typedef SkPathEffect INHERITED;
};

class SK_API SkPath1DPathEffect : public Sk1DPathEffect {
public:
    enum Style {
        kTranslate_Style,   
        kRotate_Style,      
        kMorph_Style,       

        kStyleCount
    };

    






    static SkPath1DPathEffect* Create(const SkPath& path, SkScalar advance, SkScalar phase,
                                      Style style) {
        return SkNEW_ARGS(SkPath1DPathEffect, (path, advance, phase, style));
    }

    virtual bool filterPath(SkPath*, const SkPath&,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPath1DPathEffect)

protected:
    SkPath1DPathEffect(const SkPath& path, SkScalar advance, SkScalar phase, Style);
    explicit SkPath1DPathEffect(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    
    virtual SkScalar begin(SkScalar contourLength) const SK_OVERRIDE;
    virtual SkScalar next(SkPath*, SkScalar, SkPathMeasure&) const SK_OVERRIDE;

private:
    SkPath      fPath;          
    SkScalar    fAdvance;       
    SkScalar    fInitialOffset; 
    Style       fStyle;         

    typedef Sk1DPathEffect INHERITED;
};

#endif
