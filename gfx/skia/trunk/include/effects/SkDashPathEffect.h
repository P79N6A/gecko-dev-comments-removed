






#ifndef SkDashPathEffect_DEFINED
#define SkDashPathEffect_DEFINED

#include "SkPathEffect.h"





class SK_API SkDashPathEffect : public SkPathEffect {
public:
    



















    static SkDashPathEffect* Create(const SkScalar intervals[], int count,
                                    SkScalar phase) {
        return SkNEW_ARGS(SkDashPathEffect, (intervals, count, phase));
    }
    virtual ~SkDashPathEffect();

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    virtual bool asPoints(PointData* results, const SkPath& src,
                          const SkStrokeRec&, const SkMatrix&,
                          const SkRect*) const SK_OVERRIDE;

    virtual DashType asADash(DashInfo* info) const SK_OVERRIDE;

    virtual Factory getFactory() const SK_OVERRIDE;

    static SkFlattenable* CreateProc(SkReadBuffer&);

protected:
    SkDashPathEffect(const SkScalar intervals[], int count, SkScalar phase);
    explicit SkDashPathEffect(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    SkScalar*   fIntervals;
    int32_t     fCount;
    SkScalar    fPhase;
    
    SkScalar    fInitialDashLength;
    int32_t     fInitialDashIndex;
    SkScalar    fIntervalLength;

    typedef SkPathEffect INHERITED;
};

#endif
