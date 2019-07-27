






#ifndef SkCornerPathEffect_DEFINED
#define SkCornerPathEffect_DEFINED

#include "SkPathEffect.h"






class SK_API SkCornerPathEffect : public SkPathEffect {
public:
    


    static SkCornerPathEffect* Create(SkScalar radius) {
        return SkNEW_ARGS(SkCornerPathEffect, (radius));
    }
    virtual ~SkCornerPathEffect();

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkCornerPathEffect)

protected:
    explicit SkCornerPathEffect(SkScalar radius);
    explicit SkCornerPathEffect(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    SkScalar    fRadius;

    typedef SkPathEffect INHERITED;
};

#endif
