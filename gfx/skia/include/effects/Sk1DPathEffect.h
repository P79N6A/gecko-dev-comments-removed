








#ifndef Sk1DPathEffect_DEFINED
#define Sk1DPathEffect_DEFINED

#include "SkPathEffect.h"
#include "SkPath.h"

class SkPathMeasure;


class Sk1DPathEffect : public SkPathEffect {
public:
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkStrokeRec*) SK_OVERRIDE;

protected:
    


    virtual SkScalar begin(SkScalar contourLength) = 0;
    




    virtual SkScalar next(SkPath* dst, SkScalar distance, SkPathMeasure&) = 0;

private:
    typedef SkPathEffect INHERITED;
};

class SkPath1DPathEffect : public Sk1DPathEffect {
public:
    enum Style {
        kTranslate_Style,   
        kRotate_Style,      
        kMorph_Style,       

        kStyleCount
    };

    






    SkPath1DPathEffect(const SkPath& path, SkScalar advance, SkScalar phase, Style);

    virtual bool filterPath(SkPath*, const SkPath&, SkStrokeRec*) SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPath1DPathEffect)

protected:
    SkPath1DPathEffect(SkFlattenableReadBuffer& buffer);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

    
    virtual SkScalar begin(SkScalar contourLength) SK_OVERRIDE;
    virtual SkScalar next(SkPath*, SkScalar distance, SkPathMeasure&) SK_OVERRIDE;

private:
    SkPath      fPath;          
    SkScalar    fAdvance;       
    SkScalar    fInitialOffset; 
    Style       fStyle;         

    typedef Sk1DPathEffect INHERITED;
};


#endif
