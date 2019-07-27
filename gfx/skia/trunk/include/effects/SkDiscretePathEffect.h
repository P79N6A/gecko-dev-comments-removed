






#ifndef SkDiscretePathEffect_DEFINED
#define SkDiscretePathEffect_DEFINED

#include "SkPathEffect.h"





class SK_API SkDiscretePathEffect : public SkPathEffect {
public:
    












    static SkDiscretePathEffect* Create(SkScalar segLength,
                                        SkScalar deviation,
                                        uint32_t seedAssist=0) {
        return SkNEW_ARGS(SkDiscretePathEffect,
                          (segLength, deviation, seedAssist));
    }

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkDiscretePathEffect)

protected:
    SkDiscretePathEffect(SkScalar segLength,
                         SkScalar deviation,
                         uint32_t seedAssist);
    explicit SkDiscretePathEffect(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    SkScalar fSegLength, fPerterb;

    
    uint32_t fSeedAssist;

    typedef SkPathEffect INHERITED;
};

#endif
