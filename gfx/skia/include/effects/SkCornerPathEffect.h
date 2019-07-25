








#ifndef SkCornerPathEffect_DEFINED
#define SkCornerPathEffect_DEFINED

#include "SkPathEffect.h"






class SK_API SkCornerPathEffect : public SkPathEffect {
public:
    


    SkCornerPathEffect(SkScalar radius);
    virtual ~SkCornerPathEffect();

    
    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

    
    
    virtual Factory getFactory();
    
    virtual void flatten(SkFlattenableWriteBuffer&);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);

protected:
    SkCornerPathEffect(SkFlattenableReadBuffer&);

private:
    SkScalar    fRadius;
    
    typedef SkPathEffect INHERITED;
};

#endif

