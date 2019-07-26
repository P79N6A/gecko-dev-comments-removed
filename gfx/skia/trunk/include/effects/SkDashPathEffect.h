






#ifndef SkDashPathEffect_DEFINED
#define SkDashPathEffect_DEFINED

#include "SkPathEffect.h"





class SK_API SkDashPathEffect : public SkPathEffect {
public:
    



















    static SkDashPathEffect* Create(const SkScalar intervals[], int count,
                                    SkScalar phase, bool scaleToFit = false) {
        return SkNEW_ARGS(SkDashPathEffect, (intervals, count, phase, scaleToFit));
    }
    virtual ~SkDashPathEffect();

    virtual bool filterPath(SkPath* dst, const SkPath& src,
                            SkStrokeRec*, const SkRect*) const SK_OVERRIDE;

    virtual bool asPoints(PointData* results, const SkPath& src,
                          const SkStrokeRec&, const SkMatrix&,
                          const SkRect*) const SK_OVERRIDE;

    virtual Factory getFactory() const SK_OVERRIDE;

    static SkFlattenable* CreateProc(SkReadBuffer&);

protected:
    SkDashPathEffect(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkDashPathEffect(const SkScalar intervals[], int count, SkScalar phase,
                     bool scaleToFit = false);

private:
    SkScalar*   fIntervals;
    int32_t     fCount;
    
    SkScalar    fInitialDashLength;
    int32_t     fInitialDashIndex;
    SkScalar    fIntervalLength;
    bool        fScaleToFit;

    typedef SkPathEffect INHERITED;
};

#endif
