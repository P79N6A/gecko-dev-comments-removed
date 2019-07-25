








#ifndef SkDiscretePathEffect_DEFINED
#define SkDiscretePathEffect_DEFINED

#include "SkPathEffect.h"





class SkDiscretePathEffect : public SkPathEffect {
public:
    



    SkDiscretePathEffect(SkScalar segLength, SkScalar deviation);

    
    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);

    
    
    virtual Factory getFactory();
    
    virtual void flatten(SkFlattenableWriteBuffer&);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);

protected:
    SkDiscretePathEffect(SkFlattenableReadBuffer&);

private:
    SkScalar fSegLength, fPerterb;
    
    typedef SkPathEffect INHERITED;
};

#endif

