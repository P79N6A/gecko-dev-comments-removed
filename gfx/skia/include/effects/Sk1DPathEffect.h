








#ifndef Sk1DPathEffect_DEFINED
#define Sk1DPathEffect_DEFINED

#include "SkPathEffect.h"
#include "SkPath.h"

class SkPathMeasure;


class Sk1DPathEffect : public SkPathEffect {
public:
    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

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

    
    virtual bool filterPath(SkPath*, const SkPath&, SkScalar* width) SK_OVERRIDE;

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkPath1DPathEffect, (buffer));
    }

    SK_DECLARE_FLATTENABLE_REGISTRAR()

protected:
    SkPath1DPathEffect(SkFlattenableReadBuffer& buffer);

    
    virtual SkScalar begin(SkScalar contourLength) SK_OVERRIDE;
    virtual SkScalar next(SkPath*, SkScalar distance, SkPathMeasure&) SK_OVERRIDE;
    
    virtual void flatten(SkFlattenableWriteBuffer&) SK_OVERRIDE;
    virtual Factory getFactory() SK_OVERRIDE { return CreateProc; }
    
private:
    SkPath      fPath;          
    SkScalar    fAdvance;       
    SkScalar    fInitialOffset; 
    Style       fStyle;         

    typedef Sk1DPathEffect INHERITED;
};


#endif
