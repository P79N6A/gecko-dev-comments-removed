








#ifndef SkCornerPathEffect_DEFINED
#define SkCornerPathEffect_DEFINED

#include "SkPathEffect.h"






class SK_API SkCornerPathEffect : public SkPathEffect {
public:
    


    SkCornerPathEffect(SkScalar radius);
    virtual ~SkCornerPathEffect();

    
    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkStrokeRec*) SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkCornerPathEffect)

protected:
    SkCornerPathEffect(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    SkScalar    fRadius;

    typedef SkPathEffect INHERITED;
};

#endif

