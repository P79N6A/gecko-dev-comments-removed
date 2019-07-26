








#ifndef SkDiscretePathEffect_DEFINED
#define SkDiscretePathEffect_DEFINED

#include "SkPathEffect.h"





class SkDiscretePathEffect : public SkPathEffect {
public:
    



    SkDiscretePathEffect(SkScalar segLength, SkScalar deviation);

    virtual bool filterPath(SkPath* dst, const SkPath& src, SkStrokeRec*) SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkDiscretePathEffect)

protected:
    SkDiscretePathEffect(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    SkScalar fSegLength, fPerterb;

    typedef SkPathEffect INHERITED;
};

#endif

