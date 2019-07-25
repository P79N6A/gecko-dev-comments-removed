








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

    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkPath1DPathEffect, (buffer));
    }

protected:
    SkPath1DPathEffect(SkFlattenableReadBuffer& buffer);

    
    virtual SkScalar begin(SkScalar contourLength);
    virtual SkScalar next(SkPath* dst, SkScalar distance, SkPathMeasure&);
    
    virtual void flatten(SkFlattenableWriteBuffer& );
    virtual Factory getFactory() { return CreateProc; }
    
private:
    SkPath      fPath;          
    SkScalar    fAdvance;       
    SkScalar    fInitialOffset; 
    Style       fStyle;         

    typedef Sk1DPathEffect INHERITED;
};


#endif
