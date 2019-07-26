






#ifndef SkDiscretePathEffect_DEFINED
#define SkDiscretePathEffect_DEFINED

#include "SkPathEffect.h"





class SK_API SkDiscretePathEffect : public SkPathEffect {
public:
    



    static SkDiscretePathEffect* Create(SkScalar segLength, SkScalar deviation) {
        return SkNEW_ARGS(SkDiscretePathEffect, (segLength, deviation));
    }

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkDiscretePathEffect)

protected:
    SkDiscretePathEffect(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkDiscretePathEffect(SkScalar segLength, SkScalar deviation);

private:
    SkScalar fSegLength, fPerterb;

    typedef SkPathEffect INHERITED;
};

#endif
